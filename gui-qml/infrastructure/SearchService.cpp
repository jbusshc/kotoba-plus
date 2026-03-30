#include "SearchService.h"
#include <cstring>
#include <QDebug>

#include "../app/Configuration.h"

SearchService::SearchService(kotoba_dict *dict, Configuration* config)
    : m_dict(dict), m_config(config)
{
    init_search_context(&m_ctx, config->languages, m_dict, config->pageSize);
    qDebug() << "SearchService initialized with languages:";
    for (int i = 0; i < KOTOBA_LANG_COUNT; ++i) {
        if (config->languages[i] == 0) continue;
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
    m_config = config;
    qDebug() << "SearchService::updateConfig called. New languages:";
    for (int i = 0; i < KOTOBA_LANG_COUNT; ++i) {
        if (config->languages[i] == 0) continue;
        qDebug() << i << " -" << config->languages[i];
    }
    update_search_config(&m_ctx, config->languages, config->pageSize);
}

// Debe llamarse DESPUÉS de query() / queryNonPagination().
// Los tres buffers son llenados por query_process() dentro de query_results().
// Si el contexto está limpio (query vacía), los tres strings serán vacíos.
QueryVariants SearchService::lastVariants() const
{
    QueryVariants v;
    // queries_buffer es un array de strings; el primero es la forma normalizada principal.
    // Usamos strnlen para no leer basura si el buffer no está null-terminado al final.
    v.normal   = QString::fromUtf8(m_ctx.queries_buffer,
                                   strnlen(m_ctx.queries_buffer, MAX_QUERY_LEN));
    v.romaji   = QString::fromUtf8(m_ctx.mixed_query,
                                   strnlen(m_ctx.mixed_query,   MAX_QUERY_LEN));
    v.hiragana = QString::fromUtf8(m_ctx.variant_query,
                                   strnlen(m_ctx.variant_query, MAX_QUERY_LEN));
    return v;
}