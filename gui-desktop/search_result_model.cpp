#include "search_result_model.h"

SearchResultModel::SearchResultModel(SearchPresenter *presenter,
                                     QObject *parent)
    : QAbstractListModel(parent),
      presenter(presenter)
{
    connect(presenter, &SearchPresenter::resultsReset,
            this, &SearchResultModel::onResultsReset);

    connect(presenter, &SearchPresenter::resultsAppended,
            this, &SearchResultModel::onResultsAppended);
}

int SearchResultModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return presenter->results().size();
}

QVariant SearchResultModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return {};

    const auto &results = presenter->results();

    if (index.row() < 0 || index.row() >= results.size())
        return {};

    const ResultRow &row = results[index.row()];

    if (role == Qt::DisplayRole)
    {
        if (!row.gloss.isEmpty())
            return row.headword + " — " + row.gloss;

        return row.headword;
    }

    return {};
}

void SearchResultModel::onResultsReset()
{
    beginResetModel();
    endResetModel();
}

void SearchResultModel::onResultsAppended(int first, int last)
{
    beginInsertRows(QModelIndex(), first, last);
    endInsertRows();
}
