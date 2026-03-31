#pragma once
#include <QString>
#include <vector>

extern "C" {
#include <index_search.h>
#include <loader.h>
}

struct QueryVariants {
    QString normal;    // queries_buffer[0]: la forma normalizada principal
    QString romaji;    // mixed_query: romaji/mixed
    QString hiragana;  // variant_query: con guiones aplicados
};

struct Configuration; // forward declaration

class SearchService
{
public:
    explicit SearchService();
    ~SearchService();

    void initialize(kotoba_dict *dict, Configuration* config);
    void query(const QString &q);
    void queryNonPagination(const QString &q);
    void queryNextPage();

    void updateConfig(const Configuration* config);

    // Acceso de solo lectura al contexto
    const SearchContext* searchCtx() const { return &m_ctx; }

    // Retorna las variantes procesadas del ÚLTIMO query ejecutado.
    // Debe llamarse DESPUÉS de query() o queryNonPagination(), nunca antes.
    // El parámetro raw se ignora — las variantes vienen de m_ctx directamente.
    QueryVariants lastVariants() const;
    QString highlightField(const QString &field) const;

private:
    SearchContext m_ctx;
    kotoba_dict *m_dict;
    const Configuration* m_config;
};