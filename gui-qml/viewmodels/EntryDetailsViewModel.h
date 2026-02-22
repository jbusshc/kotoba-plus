#pragma once
#include <QObject>
#include <QVariantMap>

extern "C" {
#include "../../core/include/loader.h"
}

class EntryDetailsViewModel : public QObject
{
    Q_OBJECT
public:
    explicit EntryDetailsViewModel(kotoba_dict *dict, QObject *parent = nullptr);

    Q_INVOKABLE QVariantMap buildDetails(int docId);

private:
    kotoba_dict *m_dict;
};