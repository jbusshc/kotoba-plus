#include "SrsViewModel.h"
#include "../infrastructure/SrsService.h"
#include "../../core/include/viewer.h"

SrsViewModel::SrsViewModel(SrsService *service, kotoba_dict *dict, QObject *parent)
    : QObject(parent), m_service(service), m_dict(dict)
{
}

int SrsViewModel::dueCount() const
{
    return m_service->dueCount(srs_now());
}

int SrsViewModel::learningCount() const
{
    return m_service->learningCount();
}

int SrsViewModel::newCount() const
{
    return m_service->newCount();
}

int SrsViewModel::lapsedCount() const
{
    return m_service->lapsedCount();
}

void SrsViewModel::updateStats()
{
    emit statsChanged();
}

void SrsViewModel::startSession()
{
    loadNext();
}

void SrsViewModel::loadNext()
{
    auto maybe = m_service->nextDue();

    if (!maybe.has_value())
    {
        m_hasCard = false;
        emit noMoreCards();
        updateStats();
        return;
    }

    srs_review rev = *maybe;
    const srs_item *it = rev.item;

    if (!it)
    {
        m_hasCard = false;
        emit noMoreCards();
        updateStats();
        return;
    }

    m_hasCard = true;
    m_currentEntryId = it->entry_id;

    const entry_bin *entry = kotoba_dict_get_entry(m_dict, m_currentEntryId);

    if (!entry)
    {
        emit noMoreCards();
        return;
    }

    QString word;
    QString meaning;

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

    emit showQuestion(word);
}

void SrsViewModel::revealAnswer()
{
    if (!m_hasCard)
        return;

    emit showAnswer(m_currentMeaning);
}

void SrsViewModel::handleAnswer(int quality)
{
    if (!m_hasCard)
        return;

    srs_quality q = (srs_quality)quality;

    m_service->answer(m_currentEntryId, q);

    updateStats();

    loadNext();
}

void SrsViewModel::answerAgain() { handleAnswer(SRS_AGAIN); }
void SrsViewModel::answerHard() { handleAnswer(SRS_HARD); }
void SrsViewModel::answerGood() { handleAnswer(SRS_GOOD); }
void SrsViewModel::answerEasy() { handleAnswer(SRS_EASY); }

bool SrsViewModel::contains(int entryId)
{
    return m_service->contains(entryId);
}

void SrsViewModel::add(int entryId)
{
    if (m_service->add(entryId))
    {
        updateStats();
    }
}