#pragma once
#include <QString>
extern "C" {
#include <loader.h>
}

class DictionaryRepository
{
public:
    DictionaryRepository();
    ~DictionaryRepository();

    bool open(const QString &binPath, const QString &idxPath);
    kotoba_dict* dict();

private:
    kotoba_dict m_dict;
    bool m_open = false;
};