#include "SearchViewModel.h"
#include "../infrastructure/SearchService.h"
#include "SearchResultModel.h"
#include "../../core/include/viewer.h"
#include "../../core/include/types.h"
#include <QStringList>

SearchViewModel::SearchViewModel(SearchService *service, SearchResultModel *model, kotoba_dict *dict, QObject *parent)
    : QObject(parent), m_service(service), m_model(model), m_dict(dict)
{
}

void SearchViewModel::search(const QString &text)
{
    m_docCache.clear();
    m_model->resetWith({});
    m_service->query(text);
    fillFromContext(false);
    emit resultsChanged();
}

void SearchViewModel::needMore()
{
    if (!m_service) return;
    m_service->queryNextPage();
    fillFromContext(true);
    emit resultsChanged();
}

void SearchViewModel::fillFromContext(bool append)
{
    const SearchContext *ctx = m_service->searchCtx();
    if (!ctx) return;

    uint8_t cnt = ctx->page_result_count;
    if (cnt == 0) return;

    QVector<ResultRow> rows;
    rows.reserve(cnt);

    // results_doc_ids is an array of doc ids for the current page
    const uint32_t *docIds = ctx->results_doc_ids;
    if (!docIds) return;

    for (uint32_t i = 0; i < cnt; ++i)
    {
        uint32_t doc = docIds[i];

        // avoid duplicates if append
        if (append) {
            if (m_docCache.contains(doc)) continue;
        } else {
            // even in non-append ensure not duplicate already in cache
            if (m_docCache.contains(doc)) continue;
        }

        ResultRow r;
        r.doc_id = doc;
        r.headword = QString();
        r.gloss = QString();

        const entry_bin *entry = kotoba_dict_get_entry(m_dict, doc);
        if (entry)
        {
            // headword: prefer k_ele then r_ele
            if (entry->k_elements_count > 0)
            {
                const k_ele_bin *k = kotoba_k_ele(m_dict, entry, 0);
                kotoba_str s = kotoba_keb(m_dict, k);
                r.headword = QString::fromUtf8(s.ptr, s.len);
            }
            else if (entry->r_elements_count > 0)
            {
                const r_ele_bin *re = kotoba_r_ele(m_dict, entry, 0);
                kotoba_str s = kotoba_reb(m_dict, re);
                r.headword = QString::fromUtf8(s.ptr, s.len);
            }

            // gloss: use first sense's glosses (respecting enabled languages in SearchContext)
            if (entry->senses_count > 0)
            {
                const sense_bin *sense = kotoba_sense(m_dict, entry, 0);
                if (sense->lang < 0 || sense->lang >= 32 || ctx->is_gloss_active[sense->lang] == 0)
                    continue; // filtrar por idioma
                if (sense && sense->gloss_count > 0)
                {
                    QStringList parts;
                    for (uint32_t g = 0; g < sense->gloss_count; ++g)
                    {
                        kotoba_str gs = kotoba_gloss(m_dict, sense, g);
                        parts << QString::fromUtf8(gs.ptr, gs.len);
                    }
                    r.gloss = parts.join("; ");
                    // break; // stop at first active language with glosses}
                }
            }

        }

        rows.push_back(std::move(r));
        m_docCache.push_back(doc);
    }

    if (append)
        m_model->append(rows);
    else
        m_model->resetWith(rows);
}

void SearchViewModel::openEntryAt(int index)
{
    if (index < 0 || index >= m_docCache.size()) return;
    uint32_t doc = m_docCache[index];

    QVariantMap details;
    const entry_bin *entry = kotoba_dict_get_entry(m_dict, doc);
    if (!entry) {
        emit entrySelected(details);
        return;
    }

    QString mainWord;
    if (entry->k_elements_count > 0)
    {
        const k_ele_bin *k = kotoba_k_ele(m_dict, entry, 0);
        kotoba_str s = kotoba_keb(m_dict, k);
        mainWord = QString::fromUtf8(s.ptr, s.len);
    }
    else if (entry->r_elements_count > 0)
    {
        const r_ele_bin *r = kotoba_r_ele(m_dict, entry, 0);
        kotoba_str s = kotoba_reb(m_dict, r);
        mainWord = QString::fromUtf8(s.ptr, s.len);
    }

    details["mainWord"] = mainWord;

    // readings
    QStringList readings;
    for (uint32_t i = 0; i < entry->r_elements_count; ++i)
    {
        const r_ele_bin *r = kotoba_r_ele(m_dict, entry, i);
        kotoba_str s = kotoba_reb(m_dict, r);
        readings << QString::fromUtf8(s.ptr, s.len);
    }
    details["readings"] = readings.join(" ・ ");

    // senses
    QVariantList sensesList;
    for (uint32_t s = 0; s < entry->senses_count; ++s)
    {
        const sense_bin *sense = kotoba_sense(m_dict, entry, s);
        // only include enabled language glosses (SearchContext chooses languages); for simplicity include all glosses
        QStringList glossParts;
        for (uint32_t g = 0; g < sense->gloss_count; ++g)
        {
            kotoba_str gs = kotoba_gloss(m_dict, sense, g);
            glossParts << QString::fromUtf8(gs.ptr, gs.len);
        }
        sensesList << glossParts.join("; ");
    }
    details["senses"] = sensesList;

    emit entrySelected(details);
}

QString SearchViewModel::query() const
{
    return m_query;
}

void SearchViewModel::setQuery(const QString &q)
{
    if (m_query == q)
        return;

    m_query = q;
    emit queryChanged();

    // Opcional pero recomendado:
    search(m_query);
}