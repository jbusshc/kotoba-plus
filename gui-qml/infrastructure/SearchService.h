#pragma once
#include <QString>
#include <vector>

extern "C" {
#include <index_search.h>
#include <loader.h>
}

struct Configuration; // forward declaration

class SearchService
{
public:
    explicit SearchService(kotoba_dict *dict, Configuration* config);
    ~SearchService();

    void query(const QString &q);
    void queryNonPagination(const QString &q);   // nueva: query sin paginación (para obtener conteo total)
    void queryNextPage();

    // access to context (read-only)
    const SearchContext* searchCtx() const { return &m_ctx; }

private:
    SearchContext m_ctx;
    kotoba_dict *m_dict;
    Configuration* m_config;
};