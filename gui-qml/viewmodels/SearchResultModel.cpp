#include "SearchResultModel.h"

SearchResultModel::SearchResultModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int SearchResultModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return m_rows.size();
}

QVariant SearchResultModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) return {};
    int r = index.row();
    if (r < 0 || r >= m_rows.size()) return {};

    const ResultRow &row = m_rows.at(r);

    switch (role) {
    case DocIdRole: return (int)row.doc_id;
    case HeadwordRole: return row.headword;
    case GlossRole: return row.gloss;
    case Qt::DisplayRole:
        if (!row.gloss.isEmpty()) return row.headword + " — " + row.gloss;
        return row.headword;
    default:
        return {};
    }
}

QHash<int, QByteArray> SearchResultModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[DocIdRole] = "docId";
    roles[HeadwordRole] = "headword";
    roles[GlossRole] = "gloss";
    return roles;
}

void SearchResultModel::resetWith(const QVector<ResultRow> &rows)
{
    beginResetModel();
    m_rows = rows;
    endResetModel();
}

void SearchResultModel::append(const QVector<ResultRow> &rows)
{
    if (rows.isEmpty()) return;
    int old = m_rows.size();
    beginInsertRows(QModelIndex(), old, old + rows.size() - 1);
    m_rows += rows;
    endInsertRows();
}