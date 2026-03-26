#include "SearchService.h"
#include <cstring>
#include <QDebug>

#include "../app/Configuration.h"

SearchService::SearchService(kotoba_dict *dict, Configuration* config)
    : m_dict(dict), m_config(config)
{
    init_search_context(&m_ctx, config->languages, m_dict, config->pageSize);
    // debug print language array
    qDebug() << "SearchService initialized with languages:";
    for (int i = 0; i < KOTOBA_LANG_COUNT; ++i) {
        if (config->languages[i] == 0) continue; // skip false entries;
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

    QByteArray utf8 = q.toUtf8();   

    qDebug() << "SearchService::query called with:" << q;
    qDebug() << "SearchService::query - q.toUtf8():" << utf8;

    if (utf8.size() >= MAX_QUERY_LEN) {
        qDebug() << "SearchService::query - Query too long, truncating to" << MAX_QUERY_LEN - 1 << "bytes";
        utf8.truncate(MAX_QUERY_LEN - 1);
    }

    query_results(&m_ctx, utf8.constData());
    query_next_page(&m_ctx);
}

void SearchService::queryNonPagination(const QString &q)
{
    if (q.isEmpty()) {
        reset_search_context(&m_ctx);
        return;
    }

    QByteArray utf8 = q.toUtf8();   


    if (utf8.size() >= MAX_QUERY_LEN) {
        qDebug() << "SearchService::queryNonPagination - Query too long, truncating to" << MAX_QUERY_LEN - 1 << "bytes";
        utf8.truncate(MAX_QUERY_LEN - 1);
    }

    query_results(&m_ctx, utf8.constData());
}

void SearchService::queryNextPage()
{
    if (m_ctx.results_left)
        query_next_page(&m_ctx);
}

void SearchService::updateConfig(const Configuration* config)
{
    m_config = config; // actualizar puntero a config
    // debug print
    qDebug() << "SearchService::updateConfig called. New languages:";
    for (int i = 0; i < KOTOBA_LANG_COUNT; ++i) {
        if (config->languages[i] == 0) continue; // skip false entries;
        qDebug() << i << " -" << config->languages[i];
    }
    update_search_config(&m_ctx, config->languages, config->pageSize);
}