#include "srs_presenter.h"

SrsPresenter::SrsPresenter(KotobaAppContext* ctx,
                           QObject* parent)
    : QObject(parent),
      context(ctx),
      dict(&ctx->dictionary)
{
    service = new SrsService(ctx->dictionary.entry_count);
}

void SrsPresenter::startSession()
{
    uint64_t now = srs_now();
    totalDue = service->dueCount(now);
    studied = 0;

    emit showCounts(totalDue,
                    service->learningCount(),
                    service->newCount(),
                    service->lapsedCount());

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

    const entry_bin* entry =
        kotoba_dict_get_entry(dict, currentEntryId);

    if (!entry)
    {
        emit noMoreCards();
        return;
    }

    QString word;
    QString meaning;

    // HEADWORD
    if (entry->k_elements_count > 0)
    {
        const k_ele_bin* k_ele =
            kotoba_k_ele(dict, entry, 0);

        kotoba_str keb =
            kotoba_keb(dict, k_ele);

        word = QString::fromUtf8(keb.ptr, keb.len);
    }
    else if (entry->r_elements_count > 0)
    {
        const r_ele_bin* r_ele =
            kotoba_r_ele(dict, entry, 0);

        kotoba_str reb =
            kotoba_reb(dict, r_ele);

        word = QString::fromUtf8(reb.ptr, reb.len);
    }

    // MEANING (primer sense)
    if (entry->senses_count > 0)
    {
        const sense_bin* sense =
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

    currentMeaning = meaning; // guardamos hasta que se pida revelar
    emit showQuestion(word);

    // IMPORTANTE: no emitir showAnswer aquí — será emitido por revealAnswer()
}

void SrsPresenter::revealAnswer()
{
    emit showAnswer(currentMeaning);
}

void SrsPresenter::answerAgain()
{
    service->answer(currentEntryId, SRS_AGAIN);
    loadNext();
}

void SrsPresenter::answerHard()
{
    service->answer(currentEntryId, SRS_HARD);
    loadNext();
}

void SrsPresenter::answerGood()
{
    service->answer(currentEntryId, SRS_GOOD);
    loadNext();
}

void SrsPresenter::answerEasy()
{
    service->answer(currentEntryId, SRS_EASY);
    loadNext();
}
