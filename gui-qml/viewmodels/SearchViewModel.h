#pragma once
#include <QObject>
#include <QVector>
#include <QVariantMap>

extern "C" {
#include "../../core/include/loader.h"
}

#include "SearchResultModel.h"

class SearchService;

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

public:
    SearchViewModel(SearchService *service,
                    SearchResultModel *model,
                    kotoba_dict *dict,
                    QObject *parent = nullptr);

    Q_INVOKABLE void search(const QString &text);
    Q_INVOKABLE void needMore();
    Q_INVOKABLE void openEntryAt(int index);

    int resultCount() const { return m_model->rowCount(); }

    QString query() const;
    void setQuery(const QString &q);

signals:
    void resultsChanged();
    void entrySelected(QVariantMap details);
    void queryChanged();

private:
    SearchService *m_service;
    SearchResultModel *m_model;
    kotoba_dict *m_dict;

    QVector<uint32_t> m_docCache;

    QString m_query;

    void fillFromContext(bool append = false);
};