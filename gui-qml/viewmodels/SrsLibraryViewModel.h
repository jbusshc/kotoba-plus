#pragma once
#include <QObject>
#include <QAbstractListModel>
#include <QString>
#include <QVector>
#include <cstdint>

extern "C" {
#include "../../core/include/loader.h"
}

class SrsService;

struct SrsCardItem {
    uint32_t id;
    QString word;
    QString meaning;
    QString state;
    QString interval;
    uint16_t reps;
    uint16_t lapses;
};

class SrsLibraryViewModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        WordRole,
        MeaningRole,
        StateRole,
        IntervalRole,
        RepsRole,
        LapsesRole
    };

    explicit SrsLibraryViewModel(SrsService* service, kotoba_dict* dict, QObject* parent = nullptr);

    Q_INVOKABLE void reloadCards();   // <-- agregar Q_INVOKABLE
    
    // QAbstractListModel overrides
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

private:
    SrsService* m_service;
    kotoba_dict* m_dict;
    QVector<SrsCardItem> m_cards;
};