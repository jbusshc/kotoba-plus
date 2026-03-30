#pragma once

#include <QAbstractListModel>
#include <QVector>
#include <QString>
#include <QStringList>

struct ResultRow {
    uint32_t    doc_id   = 0;
    QString     headword;   // kanji principal (o kana si no hay kanji)
    QString     gloss;      // primer sentido del idioma activo
    QStringList variants;   // todas las formas kanji (k_ele) excepto la principal
    QStringList readings;   // todas las lecturas (r_ele)
};

class SearchResultModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Roles {
        DocIdRole    = Qt::UserRole + 1,
        HeadwordRole,
        GlossRole,
        VariantsRole,   // QStringList → se serializa como string "var1・var2"
        ReadingsRole    // QStringList → se serializa como string "r1・r2"
    };

    explicit SearchResultModel(QObject *parent = nullptr);

    int      rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void resetWith(const QVector<ResultRow> &rows);
    void append(const QVector<ResultRow> &rows);

private:
    QVector<ResultRow> m_rows;
};