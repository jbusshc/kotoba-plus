#include "SearchService.h"
#include <cstring>
#include <QDebug>

SearchService::SearchService(kotoba_dict *dict, uint32_t pageSize)
    : m_dict(dict)
{
    // default enable english gloss
    bool active[KOTOBA_LANG_COUNT];
    memset(active, 0, sizeof(active));
    active[KOTOBA_LANG_EN] = true;

    init_search_context(&m_ctx, active, m_dict, (int)pageSize);
    warm_up(&m_ctx);
}

SearchService::~SearchService()
{
    free_search_context(&m_ctx);
}

void SearchService::query(const QString &q)
{
    if (q.isEmpty()) {
        reset_search_context(&m_ctx);
        return;
    }
    query_results(&m_ctx, q.toUtf8().constData());
    if (m_ctx.results_left)
        query_next_page(&m_ctx);
}

void SearchService::queryNextPage()
{
    if (m_ctx.results_left)
        query_next_page(&m_ctx);
}