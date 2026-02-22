#pragma once
#include <QString>
#include <vector>

extern "C" {
#include <index_search.h>
#include <loader.h>
}

class SearchService
{
public:
    explicit SearchService(kotoba_dict *dict, uint32_t pageSize = DEFAULT_PAGE_SIZE);
    ~SearchService();

    void query(const QString &q);
    void queryNextPage();

    // access to context (read-only)
    const SearchContext* searchCtx() const { return &m_ctx; }

private:
    SearchContext m_ctx;
    kotoba_dict *m_dict;
};