#include "SearchViewModel.h"
#include "../infrastructure/SearchService.h"
#include "SearchResultModel.h"
#include <QStringList>
#include "../app/Configuration.h"

#include "../../core/include/viewer.h"
#include "../../core/include/types.h"
#include "../../core/include/kana.h"



SearchViewModel::SearchViewModel(SearchService *service,
                                 SearchResultModel *model,
                                 kotoba_dict *dict,
                                 Configuration* config,
                                 QObject *parent)
    : QObject(parent), m_service(service), m_model(model), m_dict(dict), m_config(config)
{
    m_debounceTimer.setSingleShot(true);
    connect(&m_debounceTimer, &QTimer::timeout,
            this, &SearchViewModel::onDebounceTimeout);
}

// ── Debounce ─────────────────────────────────────────────────────────────────

void SearchViewModel::setQuery(const QString &q)
{
    if (q == m_query) return;
    m_query = q;
    emit queryChanged();
    m_debounceTimer.start(m_config ? m_config->searchDelayMs : 150);
}

void SearchViewModel::onDebounceTimeout()
{
    m_activeQuery = m_query;
    emit activeQueryChanged();

    if (m_activeQuery.isEmpty()) {
        m_docCache.clear();
        m_model->resetWith({});
        emit resultsChanged();
        return;
    }
    search(m_activeQuery);
}

// ── Búsqueda ─────────────────────────────────────────────────────────────────

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

// ── fillFromContext ───────────────────────────────────────────────────────────

void SearchViewModel::fillFromContext(bool append)
{
    const SearchContext *ctx = m_service->searchCtx();
    if (!ctx) return;

    uint8_t cnt = ctx->page_result_count;
    if (cnt == 0) return;

    const uint32_t *docIds = ctx->results_doc_ids;
    if (!docIds) return;

    QVector<ResultRow> rows;
    rows.reserve(cnt);

    for (uint32_t i = 0; i < cnt; ++i)
    {
        uint32_t doc = docIds[i];
        if (m_docCache.contains(doc)) continue;

        ResultRow r;
        r.doc_id = doc;

        const entry_bin *entry = kotoba_dict_get_entry(m_dict, doc);
        if (entry)
        {
            // ── headword: primer kanji, o primer kana si no hay kanji ────────
            if (entry->k_elements_count > 0) {
                const k_ele_bin *k = kotoba_k_ele(m_dict, entry, 0);
                kotoba_str s = kotoba_keb(m_dict, k);
                r.headword = QString::fromUtf8(s.ptr, s.len);
            } else if (entry->r_elements_count > 0) {
                const r_ele_bin *re = kotoba_r_ele(m_dict, entry, 0);
                kotoba_str s = kotoba_reb(m_dict, re);
                r.headword = QString::fromUtf8(s.ptr, s.len);
            }

            // ── variants: formas kanji restantes k_ele[1..N] ─────────────────
            for (uint32_t k = 1; k < entry->k_elements_count; ++k) {
                const k_ele_bin *ke = kotoba_k_ele(m_dict, entry, k);
                kotoba_str s = kotoba_keb(m_dict, ke);
                r.variants << QString::fromUtf8(s.ptr, s.len);
            }

            // ── readings: todos los r_ele ─────────────────────────────────────
            for (uint32_t re = 0; re < entry->r_elements_count; ++re) {
                const r_ele_bin *rel = kotoba_r_ele(m_dict, entry, re);
                kotoba_str s = kotoba_reb(m_dict, rel);
                r.readings << QString::fromUtf8(s.ptr, s.len);
            }

            // ── gloss: primer sentido del idioma activo ───────────────────────
            for (uint32_t s = 0; s < entry->senses_count; ++s)
            {
                const sense_bin *sense = kotoba_sense(m_dict, entry, s);
                if (sense->lang < 0 || sense->lang >= 32 || ctx->is_gloss_active[sense->lang] == 0)
                    continue;
                if (sense && sense->gloss_count > 0) {
                    QStringList parts;
                    for (uint32_t g = 0; g < sense->gloss_count; ++g) {
                        kotoba_str gs = kotoba_gloss(m_dict, sense, g);
                        parts << QString::fromUtf8(gs.ptr, gs.len);
                    }
                    r.gloss = parts.join("; ");
                    break;
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


QString SearchViewModel::highlightField(const QString &field) const
{
    if (m_activeQuery.isEmpty() || field.isEmpty())
        return field;
    return m_service->highlightField(field);
}

// ── openEntryAt ──────────────────────────────────────────────────────────────

void SearchViewModel::openEntryAt(int index)
{
    if (index < 0 || index >= m_docCache.size()) return;
    uint32_t doc = m_docCache[index];

    QVariantMap details;
    const entry_bin *entry = kotoba_dict_get_entry(m_dict, doc);
    if (!entry) { emit entrySelected(details); return; }

    QString mainWord;
    if (entry->k_elements_count > 0) {
        const k_ele_bin *k = kotoba_k_ele(m_dict, entry, 0);
        kotoba_str s = kotoba_keb(m_dict, k);
        mainWord = QString::fromUtf8(s.ptr, s.len);
    } else if (entry->r_elements_count > 0) {
        const r_ele_bin *r = kotoba_r_ele(m_dict, entry, 0);
        kotoba_str s = kotoba_reb(m_dict, r);
        mainWord = QString::fromUtf8(s.ptr, s.len);
    }
    details["mainWord"] = mainWord;

    QStringList readings;
    for (uint32_t i = 0; i < entry->r_elements_count; ++i) {
        const r_ele_bin *r = kotoba_r_ele(m_dict, entry, i);
        kotoba_str s = kotoba_reb(m_dict, r);
        readings << QString::fromUtf8(s.ptr, s.len);
    }
    details["readings"] = readings.join(" ・ ");

    QVariantList sensesList;
    for (uint32_t s = 0; s < entry->senses_count; ++s) {
        const sense_bin *sense = kotoba_sense(m_dict, entry, s);
        QStringList glossParts;
        for (uint32_t g = 0; g < sense->gloss_count; ++g) {
            kotoba_str gs = kotoba_gloss(m_dict, sense, g);
            glossParts << QString::fromUtf8(gs.ptr, gs.len);
        }
        sensesList << glossParts.join("; ");
    }
    details["senses"] = sensesList;

    emit entrySelected(details);
}