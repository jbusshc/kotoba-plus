#include "SrsViewModel.h"
#include "EntryDetailsViewModel.h"
#include "../infrastructure/SrsService.h"

extern "C" {
#include "../../core/include/viewer.h"
}

#include <QStringList>

// ── Constructor ───────────────────────────────────────────────────────────────

SrsViewModel::SrsViewModel(QObject *parent)
    : QObject(parent), m_service(nullptr), m_dict(nullptr), m_detailsVM(nullptr)
{}

void SrsViewModel::initialize(SrsService *service, kotoba_dict *dict, EntryDetailsViewModel *detailsVM)
{
    m_service = service;
    m_dict = dict;
    m_detailsVM = detailsVM;
}

// ── Properties ────────────────────────────────────────────────────────────────

int SrsViewModel::totalCount()       const { return m_service ? static_cast<int>(m_service->totalCount())       : 0; }
int SrsViewModel::dueCount()         const { return m_service ? static_cast<int>(m_service->dueCount())         : 0; }
int SrsViewModel::learningCount()    const { return m_service ? static_cast<int>(m_service->learningCount())    : 0; }
int SrsViewModel::newCount()         const { return m_service ? static_cast<int>(m_service->newCount())         : 0; }
int SrsViewModel::lapsedCount()      const { return m_service ? static_cast<int>(m_service->lapsedCount())      : 0; }
int SrsViewModel::reviewTodayCount() const { return m_service ? static_cast<int>(m_service->reviewTodayCount()) : 0; }

QString SrsViewModel::againInterval() const { return m_hasCard && m_service ? QString::fromStdString(m_service->predictInterval(m_currentEntryId, FSRS_AGAIN)) : QString(); }
QString SrsViewModel::hardInterval()  const { return m_hasCard && m_service ? QString::fromStdString(m_service->predictInterval(m_currentEntryId, FSRS_HARD))  : QString(); }
QString SrsViewModel::goodInterval()  const { return m_hasCard && m_service ? QString::fromStdString(m_service->predictInterval(m_currentEntryId, FSRS_GOOD))  : QString(); }
QString SrsViewModel::easyInterval()  const { return m_hasCard && m_service ? QString::fromStdString(m_service->predictInterval(m_currentEntryId, FSRS_EASY))  : QString(); }

QString     SrsViewModel::currentWord()      const { return m_currentWord; }
QString     SrsViewModel::currentMeaning()   const { return m_currentMeaning; }
bool        SrsViewModel::hasCard()          const { return m_hasCard; }
int         SrsViewModel::currentEntryId()   const { return m_hasCard ? static_cast<int>(m_currentEntryId) : -1; }
QVariantMap SrsViewModel::currentEntryData() const { return m_currentEntryData; }

bool SrsViewModel::canUndo() const { return m_service && m_service->canUndo(); }

// ── Session ───────────────────────────────────────────────────────────────────

void SrsViewModel::startSession()
{
    if (m_service) m_service->startSession();
    loadNext();
}

bool SrsViewModel::saveProfile()
{
    return m_service ? m_service->save() : false;
}

// ── loadNext ──────────────────────────────────────────────────────────────────

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

    m_hasCard        = true;
    m_currentEntryId = card->id;

    const entry_bin *entry = kotoba_dict_get_entry(m_dict, m_currentEntryId);
    if (!entry) {
        m_hasCard = false;
        m_currentEntryData = {};
        finish();
        emit noMoreCards();
        return;
    }

    // word / meaning (para compatibilidad con código existente)
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
    if (word.isEmpty()) word = "[unknown]";

    if (entry->senses_count > 0) {
        const sense_bin *sense = kotoba_sense(m_dict, entry, 0);
        QStringList parts;
        for (uint32_t g = 0; g < sense->gloss_count; ++g) {
            kotoba_str gs = kotoba_gloss(m_dict, sense, g);
            parts << QString::fromUtf8(gs.ptr, gs.len);
        }
        meaning = parts.join("; ");
    }

    m_currentWord    = word;
    m_currentMeaning = meaning;

    // entry data completo via detailsVM
    m_currentEntryData = m_detailsVM
        ? m_detailsVM->mapEntry(static_cast<int>(m_currentEntryId))
        : QVariantMap{};

    finish();
}

// ── Answer ────────────────────────────────────────────────────────────────────

void SrsViewModel::revealAnswer() {}

void SrsViewModel::handleAnswer(int quality)
{
    if (!m_hasCard || !m_service) return;
    m_service->answer(m_currentEntryId, static_cast<fsrs_rating>(quality));
    loadNext();
}

void SrsViewModel::answerAgain() { handleAnswer(FSRS_AGAIN); }
void SrsViewModel::answerHard()  { handleAnswer(FSRS_HARD);  }
void SrsViewModel::answerGood()  { handleAnswer(FSRS_GOOD);  }
void SrsViewModel::answerEasy()  { handleAnswer(FSRS_EASY);  }

// ── Add / Remove / Contains ───────────────────────────────────────────────────

bool SrsViewModel::contains(int entryId)
{
    return m_service ? m_service->contains(static_cast<uint32_t>(entryId)) : false;
}

void SrsViewModel::add(int entryId)
{
    if (!m_service) return;
    if (m_service->add(static_cast<uint32_t>(entryId)))
        updateStats();
    emit containsChanged(entryId);
}

void SrsViewModel::remove(int id)
{
    if (!m_service || !m_service->contains(id)) return;
    m_service->remove(id);
    emit containsChanged(id);
}

// ── Undo ──────────────────────────────────────────────────────────────────────

bool SrsViewModel::undoLastAnswer()
{
    if (!m_service || !m_service->canUndo()) return false;
    if (!m_service->undoLastAnswer()) return false;
    loadNext();
    return true;
}

// ── Stats ─────────────────────────────────────────────────────────────────────

void SrsViewModel::updateStats()
{
    emit statsChanged();
}

// refresh when app is active again (puede haber cambiado la fecha mientras estaba en background)
void SrsViewModel::refresh()
{
    if (!m_service) return;
    loadNext();
}