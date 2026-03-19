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

int SrsViewModel::totalCount()    const { return m_service ? static_cast<int>(m_service->totalCount())      : 0; }
int SrsViewModel::dueCount()      const { return m_service ? static_cast<int>(m_service->dueCount())        : 0; }
int SrsViewModel::learningCount() const { return m_service ? static_cast<int>(m_service->learningCount()) : 0; }
int SrsViewModel::newCount()      const { return m_service ? static_cast<int>(m_service->newCount())      : 0; }
int SrsViewModel::lapsedCount()   const { return m_service ? static_cast<int>(m_service->lapsedCount())   : 0; }

/*
 * reviewTodayCount() — BUG FIX delegado al service.
 * Ahora devuelve cards_done de la sesión actual (respuestas dadas hoy),
 * no due_today que es un conteo de cartas *pendientes*.
 */
int SrsViewModel::reviewTodayCount() const
{
    return m_service ? static_cast<int>(m_service->reviewTodayCount()) : 0;
}

QString SrsViewModel::againInterval() const { return m_hasCard && m_service ? QString::fromStdString(m_service->predictInterval(m_currentEntryId, FSRS_AGAIN)) : QString(); }
QString SrsViewModel::hardInterval()  const { return m_hasCard && m_service ? QString::fromStdString(m_service->predictInterval(m_currentEntryId, FSRS_HARD))  : QString(); }
QString SrsViewModel::goodInterval()  const { return m_hasCard && m_service ? QString::fromStdString(m_service->predictInterval(m_currentEntryId, FSRS_GOOD))  : QString(); }
QString SrsViewModel::easyInterval()  const { return m_hasCard && m_service ? QString::fromStdString(m_service->predictInterval(m_currentEntryId, FSRS_EASY))  : QString(); }

/* ---- session control ---- */

void SrsViewModel::startSession()
{
    if (m_service)
        m_service->startSession();
    loadNext();
}

bool SrsViewModel::saveProfile()
{
    return m_service ? m_service->save() : false;
}

/* ---- main: cargar siguiente carta ---- */
void SrsViewModel::loadNext()
{
    if (!m_service) {
        m_hasCard = false;
        emit currentChanged();
        emit noMoreCards();
        updateStats();
        return;
    }

    fsrs_card *card = m_service->nextCard();
    if (!card) {
        m_hasCard = false;
        emit currentChanged();
        emit noMoreCards();
        updateStats();
        return;
    }

    m_hasCard = true;
    m_currentEntryId = card->id;

    const entry_bin *entry = kotoba_dict_get_entry(m_dict, m_currentEntryId);
    if (!entry) {
        m_hasCard = false;
        emit currentChanged();
        emit noMoreCards();
        updateStats();
        return;
    }

    QString word;
    QString meaning;

    if (entry->k_elements_count > 0) {
        const k_ele_bin *k = kotoba_k_ele(m_dict, entry, 0);
        kotoba_str s = kotoba_keb(m_dict, k);
        word = QString::fromUtf8(s.ptr, s.len);
    } else if (entry->r_elements_count > 0) {
        const r_ele_bin *r = kotoba_r_ele(m_dict, entry, 0);
        kotoba_str s = kotoba_reb(m_dict, r);
        word = QString::fromUtf8(s.ptr, s.len);
    }

    if (word.isEmpty())
        word = "[unknown]";

    if (entry->senses_count > 0) {
        const sense_bin *sense = kotoba_sense(m_dict, entry, 0);
        QStringList parts;
        for (uint32_t g = 0; g < sense->gloss_count; ++g) {
            kotoba_str gs = kotoba_gloss(m_dict, sense, g);
            parts << QString::fromUtf8(gs.ptr, gs.len);
        }
        meaning = parts.join("; ");
    }

    // 🔥 AQUÍ está el cambio clave
    m_currentWord = word;
    m_currentMeaning = meaning;

    emit currentChanged();
    updateStats();
}

/* ---- reveal / answer ---- */

void SrsViewModel::revealAnswer()
{
    if (!m_hasCard) return;
}

void SrsViewModel::handleAnswer(int quality)
{
    if (!m_hasCard || !m_service) return;

    fsrs_rating r = static_cast<fsrs_rating>(quality);
    m_service->answer(m_currentEntryId, r);
    loadNext();
}

void SrsViewModel::answerAgain() { handleAnswer(FSRS_AGAIN); }
void SrsViewModel::answerHard()  { handleAnswer(FSRS_HARD);  }
void SrsViewModel::answerGood()  { handleAnswer(FSRS_GOOD);  }
void SrsViewModel::answerEasy()  { handleAnswer(FSRS_EASY);  }

/* ---- add / contains ---- */

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

/* ---- stats ---- */

void SrsViewModel::updateStats()
{
    emit statsChanged();
}


void SrsViewModel::remove(int id)
{
    if (!m_service->contains(id))
        return;

    m_service->remove(id);
    emit containsChanged(id); // 🔥 misma señal para mantener reactividad
}

bool SrsViewModel::undoLastAnswer()
{
    if (!m_service || !m_service->canUndo())
        return false;

    if (!m_service->undoLastAnswer())
        return false;

    loadNext(); // recargar la carta actualizada
    return true;
}

bool SrsViewModel::canUndo() const
{
    return m_service && m_service->canUndo();
}

QString SrsViewModel::currentWord() const {
    return m_currentWord;
}

QString SrsViewModel::currentMeaning() const {
    return m_currentMeaning;
}

bool SrsViewModel::hasCard() const {
    return m_hasCard;
}