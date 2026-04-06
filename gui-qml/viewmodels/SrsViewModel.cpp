#include "SrsViewModel.h"
#include "EntryDetailsViewModel.h"
#include "../infrastructure/SrsService.h"

extern "C" {
#include "../../core/include/viewer.h"
}

#include <QStringList>

/* ─── Constructor ────────────────────────────────────────────────────────────── */

SrsViewModel::SrsViewModel(QObject *parent)
    : QObject(parent), m_service(nullptr), m_dict(nullptr), m_detailsVM(nullptr)
{}

void SrsViewModel::initialize(SrsService *service, kotoba_dict *dict, EntryDetailsViewModel *detailsVM)
{
    m_service  = service;
    m_dict     = dict;
    m_detailsVM = detailsVM;
}

/* ─── Properties ─────────────────────────────────────────────────────────────── */

int SrsViewModel::totalCount()       const { return m_service ? static_cast<int>(m_service->totalCount())       : 0; }
int SrsViewModel::dueCount()         const { return m_service ? static_cast<int>(m_service->dueCount())         : 0; }
int SrsViewModel::learningCount()    const { return m_service ? static_cast<int>(m_service->learningCount())    : 0; }
int SrsViewModel::newCount()         const { return m_service ? static_cast<int>(m_service->newCount())         : 0; }
int SrsViewModel::lapsedCount()      const { return m_service ? static_cast<int>(m_service->lapsedCount())      : 0; }
int SrsViewModel::reviewTodayCount() const { return m_service ? static_cast<int>(m_service->reviewTodayCount()) : 0; }
bool SrsViewModel::canUndo()         const { return m_service && m_service->canUndo(); }

// Intervalos: usan el fsrsId completo de la carta en curso
QString SrsViewModel::againInterval() const {
    return (m_hasCard && m_service)
        ? QString::fromStdString(m_service->predictInterval(m_currentFsrsId, FSRS_AGAIN))
        : QString();
}
QString SrsViewModel::hardInterval() const {
    return (m_hasCard && m_service)
        ? QString::fromStdString(m_service->predictInterval(m_currentFsrsId, FSRS_HARD))
        : QString();
}
QString SrsViewModel::goodInterval() const {
    return (m_hasCard && m_service)
        ? QString::fromStdString(m_service->predictInterval(m_currentFsrsId, FSRS_GOOD))
        : QString();
}
QString SrsViewModel::easyInterval() const {
    return (m_hasCard && m_service)
        ? QString::fromStdString(m_service->predictInterval(m_currentFsrsId, FSRS_EASY))
        : QString();
}

QString     SrsViewModel::currentWord()      const { return m_currentWord; }
QString     SrsViewModel::currentMeaning()   const { return m_currentMeaning; }
bool        SrsViewModel::hasCard()          const { return m_hasCard; }
QVariantMap SrsViewModel::currentEntryData() const { return m_currentEntryData; }

int SrsViewModel::currentEntryId() const {
    return m_hasCard
        ? static_cast<int>(CardTypeHelper::toEntryId(m_currentFsrsId))
        : -1;
}

QString SrsViewModel::currentCardType() const {
    if (!m_hasCard) return QStringLiteral("Recognition");
    return (CardTypeHelper::toCardType(m_currentFsrsId) == CardType::Recall)
        ? QStringLiteral("Recall")
        : QStringLiteral("Recognition");
}

/* ─── Sesión ─────────────────────────────────────────────────────────────────── */

void SrsViewModel::startSession()
{
    if (m_service) m_service->startSession();
    loadNext();
}

bool SrsViewModel::saveProfile()
{
    return m_service ? m_service->save() : false;
}

/* ─── loadNext ───────────────────────────────────────────────────────────────── */

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

    fsrs_card *card = m_service->nextCard();
    if (!card) {
        m_hasCard = false;
        m_currentEntryData = {};
        finish();
        emit noMoreCards();
        return;
    }

    // card->id es el fsrsId completo (puede tener el bit de Recall)
    m_currentFsrsId = card->id;
    uint32_t entryId = CardTypeHelper::toEntryId(m_currentFsrsId);
    CardType type    = CardTypeHelper::toCardType(m_currentFsrsId);

    const entry_bin *entry = kotoba_dict_get_entry(m_dict, entryId);
    if (!entry) {
        m_hasCard = false;
        m_currentEntryData = {};
        finish();
        emit noMoreCards();
        return;
    }

    m_hasCard = true;

    // Construir word / meaning según el tipo de carta.
    // Recognition: pregunta = headword/lectura;  respuesta = gloss
    // Recall:      pregunta = gloss (primer sense); respuesta = headword
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

    // Para el modo Recall: intercambiamos el rol de "pregunta" y "respuesta".
    // currentWord  → lo que se muestra antes de revelar
    // currentMeaning → lo que se revela
    // EntryView en modo "srs" ya usa entryData completo, pero currentWord y
    // currentMeaning se usan en algunos widgets legacy; los mantenemos coherentes.
    if (type == CardType::Recall) {
        m_currentWord    = meaning;   // se muestra el gloss como pregunta
        m_currentMeaning = word;      // se revela la escritura
    } else {
        m_currentWord    = word;
        m_currentMeaning = meaning;
    }

    // entryData completo — EntryView usará currentCardType para decidir qué ocultar
    m_currentEntryData = m_detailsVM
        ? m_detailsVM->mapEntry(static_cast<int>(entryId))
        : QVariantMap{};

    finish();
}

/* ─── Respuestas ─────────────────────────────────────────────────────────────── */

void SrsViewModel::revealAnswer() {}

void SrsViewModel::handleAnswer(int quality)
{
    if (!m_hasCard || !m_service) return;
    m_service->answer(m_currentFsrsId, static_cast<fsrs_rating>(quality));
    loadNext();
}

void SrsViewModel::answerAgain() { handleAnswer(FSRS_AGAIN); }
void SrsViewModel::answerHard()  { handleAnswer(FSRS_HARD);  }
void SrsViewModel::answerGood()  { handleAnswer(FSRS_GOOD);  }
void SrsViewModel::answerEasy()  { handleAnswer(FSRS_EASY);  }

/* ─── Gestión de cartas ──────────────────────────────────────────────────────── */

bool SrsViewModel::containsRecognition(int entryId) const {
    return m_service && m_service->contains(static_cast<uint32_t>(entryId), CardType::Recognition);
}

bool SrsViewModel::containsRecall(int entryId) const {
    return m_service && m_service->contains(static_cast<uint32_t>(entryId), CardType::Recall);
}

void SrsViewModel::add(int entryId)
{
    if (!m_service) return;
    if (m_service->add(static_cast<uint32_t>(entryId), CardType::Recognition))
        updateStats();
    emit containsChanged(entryId, 1);   // 1 = Recognition
}

void SrsViewModel::addRecall(int entryId)
{
    if (!m_service) return;
    if (m_service->add(static_cast<uint32_t>(entryId), CardType::Recall))
        updateStats();
    emit containsChanged(entryId, 2);   // 2 = Recall
}

void SrsViewModel::addBoth(int entryId)
{
    if (!m_service) return;
    if (m_service->addBoth(static_cast<uint32_t>(entryId)))
        updateStats();
    emit containsChanged(entryId, 3);   // 3 = ambos
}

void SrsViewModel::remove(int entryId)
{
    if (!m_service) return;
    m_service->remove(static_cast<uint32_t>(entryId), CardType::Recognition);
    emit containsChanged(entryId, 1);
}

void SrsViewModel::removeRecall(int entryId)
{
    if (!m_service) return;
    m_service->remove(static_cast<uint32_t>(entryId), CardType::Recall);
    emit containsChanged(entryId, 2);
}

void SrsViewModel::removeBoth(int entryId)
{
    if (!m_service) return;
    m_service->removeBoth(static_cast<uint32_t>(entryId));
    emit containsChanged(entryId, 3);
}

/* ─── Undo ───────────────────────────────────────────────────────────────────── */

bool SrsViewModel::undoLastAnswer()
{
    if (!m_service || !m_service->canUndo()) return false;
    if (!m_service->undoLastAnswer()) return false;
    loadNext();
    return true;
}

/* ─── Stats ──────────────────────────────────────────────────────────────────── */

void SrsViewModel::updateStats() { emit statsChanged(); }

void SrsViewModel::refresh()
{
    if (!m_service) return;
    loadNext();
}