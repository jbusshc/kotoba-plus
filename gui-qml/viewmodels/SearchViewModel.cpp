#include "SearchViewModel.h"
#include "../infrastructure/SearchService.h"
#include "SearchResultModel.h"
#include <QStringList>
#include "../app/Configuration.h"

#include <QTimer>
#include <QDebug>

#include "../../core/include/viewer.h"
#include "../../core/include/types.h"
#include "../../core/include/kana.h"

void SearchViewModel::initialize(SearchService *service,
                                 SearchResultModel *model,
                                 kotoba_dict *dict,
                                 Configuration* config)
{
    m_service = service;
    m_model   = model;
    m_dict    = dict;
    m_config  = config;

    // Conectar resultados async del servicio
    connect(m_service, &SearchService::searchDone,
            this,      &SearchViewModel::onSearchDone);
    connect(m_service, &SearchService::pageReady,
            this,      &SearchViewModel::onPageReady);
}

SearchViewModel::SearchViewModel(QObject *parent)
    : QObject(parent), m_service(nullptr), m_model(nullptr), m_dict(nullptr), m_config(nullptr)
{
    // Constructor limpio, sin timer
}

// ── Debounce ─────────────────────────────────────────────────────────────────

void SearchViewModel::setQuery(const QString &q)
{
    qDebug() << "SearchViewModel::setQuery called with:" << q;
    if (q == m_query) return;

    m_query = q;
    emit queryChanged();

    // Debounce usando QTimer::singleShot
    static int lastDebounceId = 0;
    ++lastDebounceId;
    int thisDebounceId = lastDebounceId;

    int delayMs = m_config ? m_config->searchDelayMs : 150;

    QTimer::singleShot(delayMs, this, [this, thisDebounceId]() {
        // Solo ejecutar si no hubo otro setQuery más reciente
        if (thisDebounceId != lastDebounceId)
            return;

        this->onDebounceTimeout();
    });
}

void SearchViewModel::onDebounceTimeout()
{
    m_activeQuery = m_query;
    emit activeQueryChanged();

    if (m_activeQuery.isEmpty()) {
        m_docCache.clear();
        if (m_model) m_model->resetWith({});
        emit resultsChanged();
        return;
    }

    // Lanzar query async para no bloquear la UI
    if (m_service) m_service->queryAsync(m_activeQuery);
}

// ── Búsqueda ─────────────────────────────────────────────────────────────────

void SearchViewModel::search(const QString &text)
{
    qDebug() << "SearchViewModel::search called with:" << text;
    m_docCache.clear();
    if (m_model) m_model->resetWith({});
    if (m_service) m_service->query(text);
    fillFromContext(false);
    emit resultsChanged();
}

void SearchViewModel::needMore()
{
    if (!m_service) return;
    m_service->queryNextPage(); // ahora es async, emite pageReady()
}

// ── fillFromContext ───────────────────────────────────────────────────────────

void SearchViewModel::fillFromContext(bool append)
{
    if (!m_service) return;
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

    if (!m_model) return;

    if (append)
        m_model->append(rows);
    else
        m_model->resetWith(rows);
}

QString SearchViewModel::highlightField(const QString &field) const
{
    if (m_activeQuery.isEmpty() || field.isEmpty())
        return field;
    if (!m_service) return field;
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

// ── Slots async ─────────────────────────────────────────────────────────────

void SearchViewModel::onSearchDone()
{
    m_docCache.clear();
    if (m_model) m_model->resetWith({});
    fillFromContext(false);
    emit resultsChanged();
}

void SearchViewModel::onPageReady()
{
    fillFromContext(true);
    emit resultsChanged();
}