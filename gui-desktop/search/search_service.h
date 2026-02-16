#pragma once

#include <QVector>
#include <QElapsedTimer>
#include <QDebug>
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

    inline void query(const std::string &queryStr)
    {
        if (!queryStr.empty())
        {
            QElapsedTimer timer;
            timer.start();

            query_results(&searchContext, queryStr.c_str());

            qint64 elapsed = timer.elapsed();
            qDebug() << "[QUERY]" 
                     << "text:" << QString::fromStdString(queryStr)
                     << "| time:" << elapsed << "ms";
        }
    }

    inline void queryNextPage()
    {
        if (searchContext.results_left)
        {
            QElapsedTimer timer;
            timer.start();

            query_next_page(&searchContext);

            qint64 elapsed = timer.elapsed();
            qDebug() << "[QUERY NEXT PAGE]"
                     << "| time:" << elapsed << "ms";
        }
    }

    inline bool hasMoreResults() const 
    { 
        return searchContext.results_left > 0; 
    }

    inline KotobaAppContext* appContext() const 
    { 
        return ctx; 
    }

    inline const SearchContext* searchCtx() const 
    { 
        return &searchContext; 
    }

private:
    KotobaAppContext *ctx;
    SearchContext searchContext;
    int pageSize;
};
