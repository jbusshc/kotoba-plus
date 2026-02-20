#include "srs_presenter.h"

SrsPresenter::SrsPresenter(KotobaAppContext* ctx,
                           QObject* parent)
    : QObject(parent),
      context(ctx),
      dict(&ctx->dictionary)
{
    service = new SrsService(ctx->dictionary.entry_count);
    snapshotPath = "srs_profile.dat";
    service->load(snapshotPath);
}

void SrsPresenter::startSession()
{
    service->resetSessionSeenBitset();

    sessionTotal = service->countDueAndLearningToday();
    sessionRemaining = sessionTotal;

    emit updateProgress(0, sessionTotal);
    loadNext();
}

void SrsPresenter::loadNext()
{
    auto review = service->nextDue();

    if (!review)
    {
        emit noMoreCards();
        return;
    }

    currentEntryId = review->entryId;

    const entry_bin *entry =
        kotoba_dict_get_entry(dict, currentEntryId);

    if (!entry)
    {
        emit noMoreCards();
        return;
    }

    QString word;
    QString meaning;

    if (entry->k_elements_count > 0)
    {
        const k_ele_bin *k_ele =
            kotoba_k_ele(dict, entry, 0);

        kotoba_str keb =
            kotoba_keb(dict, k_ele);

        word = QString::fromUtf8(keb.ptr, keb.len);
    }
    else if (entry->r_elements_count > 0)
    {
        const r_ele_bin *r_ele =
            kotoba_r_ele(dict, entry, 0);

        kotoba_str reb =
            kotoba_reb(dict, r_ele);

        word = QString::fromUtf8(reb.ptr, reb.len);
    }

    if (entry->senses_count > 0)
    {
        const sense_bin *sense =
            kotoba_sense(dict, entry, 0);

        for (uint32_t g = 0; g < sense->gloss_count; ++g)
        {
            kotoba_str gloss =
                kotoba_gloss(dict, sense, g);

            meaning += QString::fromUtf8(gloss.ptr, gloss.len);

            if (g < sense->gloss_count - 1)
                meaning += "; ";
        }
    }

    currentMeaning = meaning;
    emit showQuestion(word);
}

void SrsPresenter::revealAnswer()
{
    emit showAnswer(currentMeaning);
}

void SrsPresenter::handleAnswer(srs_quality grade)
{
    service->answer(currentEntryId, grade);
    autoSave();

    // 🔥 CLAVE: solo baja remaining si ya no es del día
    if (!service->isDueToday(currentEntryId))
    {
        if (sessionRemaining > 0)
            sessionRemaining--;
    }

    emit updateProgress(
        sessionTotal - sessionRemaining,
        sessionTotal
    );

    loadNext();
}

void SrsPresenter::answerAgain() { handleAnswer(SRS_AGAIN); }
void SrsPresenter::answerHard()  { handleAnswer(SRS_HARD); }
void SrsPresenter::answerGood()  { handleAnswer(SRS_GOOD); }
void SrsPresenter::answerEasy()  { handleAnswer(SRS_EASY); }

void SrsPresenter::refreshStats()
{
    uint64_t now = srs_now();

    emit showCounts(
        service->dueCount(now),
        service->learningCount(),
        service->newCount(),
        service->lapsedCount());
}

bool SrsPresenter::contains(uint32_t entryId) const
{
    return service->contains(entryId);
}

void SrsPresenter::add(uint32_t entryId)
{
    service->add(entryId);
    refreshStats();
}

void SrsPresenter::remove(uint32_t entryId)
{
    service->remove(entryId);
    refreshStats();
}

void SrsPresenter::autoSave()
{
    service->save(snapshotPath);
}
