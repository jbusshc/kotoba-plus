#include "SrsViewModel.h"
#include "../infrastructure/SrsService.h"

extern "C" {
#include "../../core/include/viewer.h"
}

#include <QStringList>

SrsViewModel::SrsViewModel(SrsService *service, kotoba_dict *dict, QObject *parent)
    : QObject(parent), m_service(service), m_dict(dict)
{
}

/* ---- properties ---- */
int SrsViewModel::dueCount() const        { return m_service ? static_cast<int>(m_service->dueCount()) : 0; }
int SrsViewModel::learningCount() const   { return m_service ? static_cast<int>(m_service->learningCount()) : 0; }
int SrsViewModel::newCount() const        { return m_service ? static_cast<int>(m_service->newCount()) : 0; }
int SrsViewModel::lapsedCount() const     { return m_service ? static_cast<int>(m_service->lapsedCount()) : 0; }

QString SrsViewModel::againInterval() const { return m_hasCard && m_service ? QString::fromStdString(m_service->predictInterval(m_currentEntryId, FSRS_AGAIN)) : ""; }
QString SrsViewModel::hardInterval() const  { return m_hasCard && m_service ? QString::fromStdString(m_service->predictInterval(m_currentEntryId, FSRS_HARD)) : ""; }
QString SrsViewModel::goodInterval() const  { return m_hasCard && m_service ? QString::fromStdString(m_service->predictInterval(m_currentEntryId, FSRS_GOOD)) : ""; }
QString SrsViewModel::easyInterval() const  { return m_hasCard && m_service ? QString::fromStdString(m_service->predictInterval(m_currentEntryId, FSRS_EASY)) : ""; }

/* ---- session control ---- */
void SrsViewModel::startSession()
{
    if (m_service)
        m_service->startSession();
    loadNext();
}

/* ---- main: cargar siguiente carta y emitir señales ---- */
void SrsViewModel::loadNext()
{
    if (!m_service)
    {
        m_hasCard = false;
        emit noMoreCards();
        updateStats();
        return;
    }

    fsrs_card *card = m_service->nextCard();
    if (!card)
    {
        m_hasCard = false;
        emit noMoreCards();
        updateStats();
        return;
    }

    m_hasCard = true;
    m_currentEntryId = card->id;

    const entry_bin *entry = kotoba_dict_get_entry(m_dict, m_currentEntryId);
    if (!entry)
    {
        m_hasCard = false;
        emit noMoreCards();
        updateStats();
        return;
    }

    QString word;
    QString meaning;

    /* WORD: preferir keb (kanji) y si no, reb (reading) */
    if (entry->k_elements_count > 0)
    {
        const k_ele_bin *k = kotoba_k_ele(m_dict, entry, 0);
        kotoba_str s = kotoba_keb(m_dict, k);
        word = QString::fromUtf8(s.ptr, s.len);
    }
    else if (entry->r_elements_count > 0)
    {
        const r_ele_bin *r = kotoba_r_ele(m_dict, entry, 0);
        kotoba_str s = kotoba_reb(m_dict, r);
        word = QString::fromUtf8(s.ptr, s.len);
    }
    if (word.isEmpty()) word = "[unknown]";

    /* MEANING: unir glosses de la primera sense */
    if (entry->senses_count > 0)
    {
        const sense_bin *sense = kotoba_sense(m_dict, entry, 0);
        QStringList parts;
        for (uint32_t g = 0; g < sense->gloss_count; ++g)
        {
            kotoba_str gs = kotoba_gloss(m_dict, sense, g);
            parts << QString::fromUtf8(gs.ptr, gs.len);
        }
        meaning = parts.join("; ");
    }

    m_currentMeaning = meaning;

    // 🔹 Emitir carta nueva
    emit showQuestion(word);

    // 🔹 Actualizar stats e intervalos
    updateStats();
}

/* ---- reveal / answer ---- */
void SrsViewModel::revealAnswer()
{
    if (!m_hasCard) return;

    emit showAnswer(m_currentMeaning);
    updateStats(); // 🔹 refrescar intervalos en QML
}

/* ---- handle answer ---- */
void SrsViewModel::handleAnswer(int quality)
{
    if (!m_hasCard || !m_service) return;

    fsrs_rating r = static_cast<fsrs_rating>(quality);
    m_service->answer(m_currentEntryId, r);

    // cargar siguiente carta
    loadNext();
}

/* ---- helpers para botones ---- */
void SrsViewModel::answerAgain() { handleAnswer(FSRS_AGAIN); }
void SrsViewModel::answerHard()  { handleAnswer(FSRS_HARD);  }
void SrsViewModel::answerGood()  { handleAnswer(FSRS_GOOD);  }
void SrsViewModel::answerEasy()  { handleAnswer(FSRS_EASY);  }

/* ---- add / contains ---- */
bool SrsViewModel::contains(int entryId) { return m_service ? m_service->contains(static_cast<uint32_t>(entryId)) : false; }

void SrsViewModel::add(int entryId)
{
    if (!m_service) return;
    if (m_service->add(static_cast<uint32_t>(entryId)))
        updateStats();
}

/* ---- actualizar stats / intervalos ---- */
void SrsViewModel::updateStats()
{
    emit statsChanged(); // 🔹 esto refresca todos los bindings QML (contadores e intervalos)
}