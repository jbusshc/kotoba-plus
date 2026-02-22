#pragma once

#include <QAbstractListModel>
#include <QVector>
#include <QString>

struct ResultRow {
    uint32_t doc_id;
    QString headword;
    QString gloss;
};

class SearchResultModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Roles {
        DocIdRole = Qt::UserRole + 1,
        HeadwordRole,
        GlossRole
    };

    explicit SearchResultModel(QObject *parent = nullptr);

    // model interface
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // helpers
    void resetWith(const QVector<ResultRow> &rows);
    void append(const QVector<ResultRow> &rows);

private:
    QVector<ResultRow> m_rows;
};