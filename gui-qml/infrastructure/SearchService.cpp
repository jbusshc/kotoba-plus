#include "SearchService.h"
#include <cstring>
#include <QDebug>

#include "../app/Configuration.h"

SearchService::SearchService(kotoba_dict *dict, Configuration* config)
    : m_dict(dict), m_config(config)
{
    init_search_context(&m_ctx, config->languages, m_dict, config->searchPageSize);
    // debug print language array
    qDebug() << "SearchService initialized with languages:";
    for (int i = 0; i < 29; ++i) {
        if (config->languages[i] == 0) break;
        qDebug() << i << " -" << config->languages[i];
    }
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