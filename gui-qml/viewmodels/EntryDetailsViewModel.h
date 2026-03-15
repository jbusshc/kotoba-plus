#pragma once
#include <QObject>
#include <QVariantMap>

extern "C" {
#include "../../core/include/loader.h"
}

class Configuration; // forward declaration

class EntryDetailsViewModel : public QObject
{
    Q_OBJECT
public:
    explicit EntryDetailsViewModel(kotoba_dict *dict, Configuration *config, QObject *parent = nullptr);

    Q_INVOKABLE QVariantMap mapEntry(int docId);

private:
    kotoba_dict *m_dict;
    Configuration *m_config;
};