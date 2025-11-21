#ifndef SEARCHRESULTMODEL_H
#define SEARCHRESULTMODEL_H

#include <QAbstractListModel>
#include "libtango.h"

class SearchResultModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit SearchResultModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;

    void performSearch(TangoDB* db, const QString& query);

    const TangoSearchResult* get(int index) const { return results[index]; }

private:
    QVector<TangoSearchResult*> results;
};

#endif
