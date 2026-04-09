#include "DictionaryRepository.h"
#include <QDebug>

DictionaryRepository::DictionaryRepository()
{
    m_open = false;
}

DictionaryRepository::~DictionaryRepository()
{
    if (m_open) kotoba_dict_close(&m_dict);
}

bool DictionaryRepository::open(const QString &binPath, const QString &idxPath)
{
    int rc = kotoba_dict_open(&m_dict,
                              binPath.toUtf8().constData(),
                              idxPath.toUtf8().constData());
    if (rc == 0) {
        qWarning("kotoba_dict_open failed");
        return false;
    }
    m_open = true;
    return true;
}

kotoba_dict* DictionaryRepository::dict()
{
    return &m_dict;
}