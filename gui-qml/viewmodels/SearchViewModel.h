#pragma once
#include <QObject>
#include <QVector>
#include <QVariantMap>
#include <QString>
#include <QTimer>

extern "C" {
#include "../../core/include/loader.h"
}

#include "SearchResultModel.h"

class SearchService;
class Configuration;

class SearchViewModel : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int resultCount
               READ resultCount
               NOTIFY resultsChanged)

    Q_PROPERTY(QString query
               READ query
               WRITE setQuery
               NOTIFY queryChanged)

    // Query post-debounce: la que realmente se ejecutó y cuyos buffers
    // del SearchContext están vigentes. Úsala en QML para el highlight.
    Q_PROPERTY(QString activeQuery
               READ activeQuery
               NOTIFY activeQueryChanged)

public:
    SearchViewModel(SearchService *service,
                    SearchResultModel *model,
                    kotoba_dict *dict,
                    Configuration* config,
                    QObject *parent = nullptr);

    Q_INVOKABLE void search(const QString &text);
    Q_INVOKABLE void needMore();
    Q_INVOKABLE void openEntryAt(int index);

    // Devuelve 'field' con la coincidencia envuelta en <b> para RichText.
    // El match se intenta contra las 3 variantes del último query ejecutado.
    // Retorna el texto original (plain) si no hay match o si activeQuery está vacío.
    Q_INVOKABLE QString highlightField(const QString &field) const;

    int     resultCount() const { return m_model->rowCount(); }
    QString query()       const { return m_query; }
    QString activeQuery() const { return m_activeQuery; }
    void    setQuery(const QString &q);

signals:
    void resultsChanged();
    void entrySelected(QVariantMap details);
    void queryChanged();
    void activeQueryChanged();

private slots:
    void onDebounceTimeout();

private:
    void fillFromContext(bool append = false);

    SearchService     *m_service;
    SearchResultModel *m_model;
    kotoba_dict       *m_dict;
    Configuration     *m_config;

    QVector<uint32_t>  m_docCache;

    QString  m_query;        // texto en el campo, actualizado en cada keystroke
    QString  m_activeQuery;  // query con la que se ejecutó la última búsqueda real
    QTimer   m_debounceTimer;
};