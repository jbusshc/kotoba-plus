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
    explicit EntryDetailsViewModel(QObject *parent = nullptr);
    void initialize(kotoba_dict *dict, Configuration *config);

    Q_INVOKABLE QVariantMap mapEntry(int docId);

private:
    QString headword(uint32_t id);

    kotoba_dict *m_dict;
    Configuration *m_config;

};