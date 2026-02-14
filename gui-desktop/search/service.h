#pragma once

#include <QVector>
#include <string>
#include "../app/context.h"

extern "C" {
#include "index_search.h"
}


class KotobaSearchService
{
public:
    explicit KotobaSearchService(KotobaAppContext *ctx);
    ~KotobaSearchService();

    inline void query(const std::string &query) { if (!query.empty()) query_results(&searchContext, query.c_str()); }
    inline void queryNextPage() { if (searchContext.results_left) query_next_page(&searchContext); }
    inline bool hasMoreResults() const { return searchContext.results_left > 0; }
    inline KotobaAppContext* appContext() const { return ctx; }
    inline const SearchContext* searchCtx() const { return &searchContext; }

private:
    KotobaAppContext *ctx;
    SearchContext searchContext;
    int pageSize;
};
