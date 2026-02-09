#pragma once

#include <QAbstractListModel>

#include "search_presenter.h"

class SearchResultModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit SearchResultModel(SearchPresenter *presenter,
                               QObject *parent = nullptr);

    // QAbstractItemModel interface
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index,
                  int role = Qt::DisplayRole) const override;

private slots:
    void onResultsReset();
    void onResultsAppended(int first, int last);

private:
    SearchPresenter *presenter;
};
