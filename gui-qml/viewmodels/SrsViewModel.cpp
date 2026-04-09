#include "SrsViewModel.h"
#include "EntryDetailsViewModel.h"
#include "../infrastructure/SrsService.h"

extern "C" {
#include "../../core/include/viewer.h"
}

#include <QStringList>

// ─────────────────────────────────────────────────────────────────────────────
// Constructor
// ─────────────────────────────────────────────────────────────────────────────

SrsViewModel::SrsViewModel(QObject *parent)
    : QObject(parent)
{}

void SrsViewModel::initialize(SrsService *service, kotoba_dict *dict, EntryDetailsViewModel *detailsVM)
{
    m_service   = service;
    m_dict      = dict;
    m_detailsVM = detailsVM;
}

// ─────────────────────────────────────────────────────────────────────────────
// Properties
// ─────────────────────────────────────────────────────────────────────────────

int SrsViewModel::totalCount()       const { return m_service ? (int)m_service->totalCount()       : 0; }
int SrsViewModel::dueCount()         const { return m_service ? (int)m_service->dueCount()         : 0; }
int SrsViewModel::learningCount()    const { return m_service ? (int)m_service->learningCount()    : 0; }
int SrsViewModel::newCount()         const { return m_service ? (int)m_service->newCount()         : 0; }
int SrsViewModel::lapsedCount()      const { return m_service ? (int)m_service->lapsedCount()      : 0; }
int SrsViewModel::reviewTodayCount() const { return m_service ? (int)m_service->reviewTodayCount() : 0; }
bool SrsViewModel::canUndo()         const { return m_service && m_service->canUndo(); }

QString SrsViewModel::againInterval() const {
    return (m_hasCard && m_service)
        ? QString::fromStdString(m_service->predictInterval(m_current.entSeq, m_current.type, FSRS_AGAIN))
        : QString();
}
QString SrsViewModel::hardInterval() const {
    return (m_hasCard && m_service)
        ? QString::fromStdString(m_service->predictInterval(m_current.entSeq, m_current.type, FSRS_HARD))
        : QString();
}
QString SrsViewModel::goodInterval() const {
    return (m_hasCard && m_service)
        ? QString::fromStdString(m_service->predictInterval(m_current.entSeq, m_current.type, FSRS_GOOD))
        : QString();
}
QString SrsViewModel::easyInterval() const {
    return (m_hasCard && m_service)
        ? QString::fromStdString(m_service->predictInterval(m_current.entSeq, m_current.type, FSRS_EASY))
        : QString();
}

QString     SrsViewModel::currentWord()      const { return m_currentWord; }
QString     SrsViewModel::currentMeaning()   const { return m_currentMeaning; }
bool        SrsViewModel::hasCard()          const { return m_hasCard; }
QVariantMap SrsViewModel::currentEntryData() const { return m_currentEntryData; }

int SrsViewModel::currentEntryId() const
{
    return m_hasCard ? (int)m_current.entSeq : -1;
}

QString SrsViewModel::currentCardType() const
{
    if (!m_hasCard) return QStringLiteral("Recognition");
    return (m_current.type == CardType::Recall)
        ? QStringLiteral("Recall")
        : QStringLiteral("Recognition");
}

// ─────────────────────────────────────────────────────────────────────────────
// Sesión
// ─────────────────────────────────────────────────────────────────────────────

void SrsViewModel::startSession()
{
    if (m_service) m_service->startSession();
    loadNext();
}

void SrsViewModel::startCustomSession(const QString &customDeckId)
{
    if (m_service) m_service->startCustomSession(customDeckId);
    loadNext();
}

bool SrsViewModel::saveProfile()
{
    return m_service ? m_service->save() : false;
}

// ─────────────────────────────────────────────────────────────────────────────
// loadNext
// ─────────────────────────────────────────────────────────────────────────────

void SrsViewModel::loadNext()
{
    auto finish = [&]() {
        emit currentChanged();
        updateStats();
    };

    if (!m_service) {
        m_hasCard = false;
        finish();
        emit noMoreCards();
        return;
    }

    NextCard nc = m_service->nextCard();
    if (!nc) {
        m_hasCard          = false;
        m_current          = {};
        m_currentEntryData = {};
        finish();
        emit noMoreCards();
        return;
    }

    // Lookup por ent_seq — estable entre regeneraciones del dict
    const entry_bin *entry = kotoba_entry(m_dict, nc.entSeq);
    if (!entry) {
        m_hasCard          = false;
        m_current          = {};
        m_currentEntryData = {};
        finish();
        emit noMoreCards();
        return;
    }

    m_hasCard = true;
    m_current = nc;

    // Construir word / meaning
    QString word, meaning;

    if (entry->k_elements_count > 0) {
        const k_ele_bin *k = kotoba_k_ele(m_dict, entry, 0);
        kotoba_str s = kotoba_keb(m_dict, k);
        word = QString::fromUtf8(s.ptr, s.len);
    } else if (entry->r_elements_count > 0) {
        const r_ele_bin *r = kotoba_r_ele(m_dict, entry, 0);
        kotoba_str s = kotoba_reb(m_dict, r);
        word = QString::fromUtf8(s.ptr, s.len);
    }
    if (word.isEmpty()) word = QStringLiteral("[unknown]");

    if (entry->senses_count > 0) {
        const sense_bin *sense = kotoba_sense(m_dict, entry, 0);
        QStringList parts;
        for (uint32_t g = 0; g < sense->gloss_count; ++g) {
            kotoba_str gs = kotoba_gloss(m_dict, sense, g);
            parts << QString::fromUtf8(gs.ptr, gs.len);
        }
        meaning = parts.join(QStringLiteral("; "));
    }

    // Recognition: pregunta = headword, respuesta = gloss
    // Recall:      pregunta = gloss,    respuesta = headword
    if (nc.type == CardType::Recall) {
        m_currentWord    = meaning;
        m_currentMeaning = word;
    } else {
        m_currentWord    = word;
        m_currentMeaning = meaning;
    }

    m_currentEntryData = m_detailsVM
        ? m_detailsVM->mapEntry((int)nc.entSeq)
        : QVariantMap{};

    finish();
}

// ─────────────────────────────────────────────────────────────────────────────
// Respuestas
// ─────────────────────────────────────────────────────────────────────────────

void SrsViewModel::revealAnswer() {}

void SrsViewModel::handleAnswer(int quality)
{
    if (!m_hasCard || !m_service) return;
    m_service->answer(m_current.entSeq, m_current.type,
                      static_cast<fsrs_rating>(quality));
    loadNext();
}

void SrsViewModel::answerAgain() { handleAnswer(FSRS_AGAIN); }
void SrsViewModel::answerHard()  { handleAnswer(FSRS_HARD);  }
void SrsViewModel::answerGood()  { handleAnswer(FSRS_GOOD);  }
void SrsViewModel::answerEasy()  { handleAnswer(FSRS_EASY);  }

// ─────────────────────────────────────────────────────────────────────────────
// Gestión de cartas
// ─────────────────────────────────────────────────────────────────────────────

bool SrsViewModel::containsRecognition(int entSeq) const {
    return m_service && m_service->contains((uint32_t)entSeq, CardType::Recognition);
}

bool SrsViewModel::containsRecall(int entSeq) const {
    return m_service && m_service->contains((uint32_t)entSeq, CardType::Recall);
}

void SrsViewModel::add(int entSeq)
{
    if (!m_service) return;
    if (m_service->add((uint32_t)entSeq, CardType::Recognition))
        updateStats();
    emit containsChanged(entSeq, 1);
}

void SrsViewModel::addRecall(int entSeq)
{
    if (!m_service) return;
    if (m_service->add((uint32_t)entSeq, CardType::Recall))
        updateStats();
    emit containsChanged(entSeq, 2);
}

void SrsViewModel::addBoth(int entSeq)
{
    if (!m_service) return;
    if (m_service->addBoth((uint32_t)entSeq))
        updateStats();
    emit containsChanged(entSeq, 3);
}

void SrsViewModel::remove(int entSeq)
{
    if (!m_service) return;
    m_service->remove((uint32_t)entSeq, CardType::Recognition);
    emit containsChanged(entSeq, 1);
}

void SrsViewModel::removeRecall(int entSeq)
{
    if (!m_service) return;
    m_service->remove((uint32_t)entSeq, CardType::Recall);
    emit containsChanged(entSeq, 2);
}

void SrsViewModel::removeBoth(int entSeq)
{
    if (!m_service) return;
    m_service->removeBoth((uint32_t)entSeq);
    emit containsChanged(entSeq, 3);
}

// ─────────────────────────────────────────────────────────────────────────────
// Undo
// ─────────────────────────────────────────────────────────────────────────────

bool SrsViewModel::undoLastAnswer()
{
    if (!m_service || !m_service->canUndo()) return false;
    if (!m_service->undoLastAnswer()) return false;
    loadNext();
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// Stats
// ─────────────────────────────────────────────────────────────────────────────

void SrsViewModel::updateStats() { emit statsChanged(); }

void SrsViewModel::refresh()
{
    if (!m_service) return;
    loadNext();
}