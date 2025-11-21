#include "SearchResultModel.h"
#include <QDebug>

SearchResultModel::SearchResultModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int SearchResultModel::rowCount(const QModelIndex&) const {
    return results.size();
}

QVariant SearchResultModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || role != Qt::DisplayRole)
        return {};

    const TangoSearchResult* r = results[index.row()];
    return QString("%1 — %2").arg(r->kanjis).arg(r->glosses);
}

void SearchResultModel::performSearch(TangoDB* db, const QString &query) {
    beginResetModel();
    qDeleteAll(results);
    results.clear();

    tango_db_text_search(
        db,
        query.toUtf8().constData(),
        [](const TangoSearchResult* r, void* ud){
            auto vec = static_cast<QVector<TangoSearchResult*>*>(ud);
            vec->append(new TangoSearchResult(*r));
        },
        &results
    );

    endResetModel();
}
