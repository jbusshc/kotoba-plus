#include "EntryDetailsViewModel.h"
#include "../../core/include/viewer.h"
#include <QStringList>

EntryDetailsViewModel::EntryDetailsViewModel(kotoba_dict *dict, QObject *parent)
    : QObject(parent), m_dict(dict)
{
}

QVariantMap EntryDetailsViewModel::buildDetails(int docId)
{
    QVariantMap map;

    const entry_bin *entry = kotoba_dict_get_entry(m_dict, docId);
    if (!entry) return map;

    // MAIN WORD
    QString mainWord;
    if (entry->k_elements_count > 0)
    {
        const k_ele_bin *k = kotoba_k_ele(m_dict, entry, 0);
        kotoba_str s = kotoba_keb(m_dict, k);
        mainWord = QString::fromUtf8(s.ptr, s.len);
    }
    else if (entry->r_elements_count > 0)
    {
        const r_ele_bin *r = kotoba_r_ele(m_dict, entry, 0);
        kotoba_str s = kotoba_reb(m_dict, r);
        mainWord = QString::fromUtf8(s.ptr, s.len);
    }

    map["mainWord"] = mainWord;

    // READINGS
    QStringList readings;
    for (uint32_t i = 0; i < entry->r_elements_count; ++i)
    {
        const r_ele_bin *r = kotoba_r_ele(m_dict, entry, i);
        kotoba_str s = kotoba_reb(m_dict, r);
        readings << QString::fromUtf8(s.ptr, s.len);
    }
    map["readings"] = readings.join(" ・ ");

    // SENSES
    QVariantList senses;
    for (uint32_t s = 0; s < entry->senses_count; ++s)
    {
        const sense_bin *sense = kotoba_sense(m_dict, entry, s);
        QStringList glossParts;
        for (uint32_t g = 0; g < sense->gloss_count; ++g)
        {
            kotoba_str gs = kotoba_gloss(m_dict, sense, g);
            glossParts << QString::fromUtf8(gs.ptr, gs.len);
        }
        senses << glossParts.join("; ");
    }
    map["senses"] = senses;

    return map;
}