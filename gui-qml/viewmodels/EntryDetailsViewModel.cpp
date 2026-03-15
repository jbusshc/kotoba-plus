#include "EntryDetailsViewModel.h"

extern "C" {
#include "../../core/include/viewer.h"
}
#include "../../gui-qml/app/Configuration.h"

#include <QStringList>

EntryDetailsViewModel::EntryDetailsViewModel(
    kotoba_dict *dict,
    Configuration *config,
    QObject *parent
) : QObject(parent), m_dict(dict), m_config(config)
{
}

QVariantMap EntryDetailsViewModel::mapEntry(int docId)
{
    QVariantMap result;

    const entry_bin *entry = kotoba_entry(m_dict, docId);
    if (!entry)
        return result;

    // IDs y conteos básicos
    result["id"] = docId;
    result["k_elements_count"] = entry->k_elements_count;
    result["r_elements_count"] = entry->r_elements_count;
    result["senses_count"] = entry->senses_count;

    // KANJI ELEMENTS
    QVariantList kElements;
    for (uint32_t i = 0; i < entry->k_elements_count; ++i) {
        QVariantMap kMap;
        const k_ele_bin *k = kotoba_k_ele(m_dict, entry, i);
        kotoba_str keb = kotoba_keb(m_dict, k);
        kMap["keb"] = QString::fromUtf8(keb.ptr, keb.len);
        kElements.append(kMap);
    }
    result["k_elements"] = kElements;

    // READING ELEMENTS
    QVariantList rElements;
    for (uint32_t i = 0; i < entry->r_elements_count; ++i) {
        QVariantMap rMap;
        const r_ele_bin *r = kotoba_r_ele(m_dict, entry, i);
        kotoba_str reb = kotoba_reb(m_dict, r);
        rMap["reb"] = QString::fromUtf8(reb.ptr, reb.len);
        rElements.append(rMap);
    }
    result["r_elements"] = rElements;

    // SENSES
    QVariantList senses;
    for (uint32_t i = 0; i < entry->senses_count; ++i) {
        QVariantMap sMap;
        const sense_bin *sense = kotoba_sense(m_dict, entry, i);

        sMap["lang"] = sense->lang;

        // POS
        QStringList posList;
        for (uint32_t p = 0; p < sense->pos_count; ++p) {
            kotoba_str pos = kotoba_pos(m_dict, sense, p);
            posList << QString::fromUtf8(pos.ptr, pos.len);
        }
        sMap["pos"] = posList;

        // GLOSS
        QStringList glossList;
        for (uint32_t g = 0; g < sense->gloss_count; ++g) {
            kotoba_str gl = kotoba_gloss(m_dict, sense, g);
            glossList << QString::fromUtf8(gl.ptr, gl.len);
        }
        sMap["gloss"] = glossList;

        // STAGK
        QStringList stagkList;
        for (uint32_t sk = 0; sk < sense->stagk_count; ++sk) {
            kotoba_str sks = kotoba_stagk(m_dict, sense, sk);
            stagkList << QString::fromUtf8(sks.ptr, sks.len);
        }
        sMap["stagk"] = stagkList;

        // STAGR
        QStringList stagrList;
        for (uint32_t sr = 0; sr < sense->stagr_count; ++sr) {
            kotoba_str srs = kotoba_stagr(m_dict, sense, sr);
            stagrList << QString::fromUtf8(srs.ptr, srs.len);
        }
        sMap["stagr"] = stagrList;

        // XREF
        QStringList xrefList;
        for (uint32_t x = 0; x < sense->xref_count; ++x) {
            kotoba_str xr = kotoba_xref(m_dict, sense, x);
            xrefList << QString::fromUtf8(xr.ptr, xr.len);
        }
        sMap["xref"] = xrefList;

        // ANT
        QStringList antList;
        for (uint32_t a = 0; a < sense->ant_count; ++a) {
            kotoba_str an = kotoba_ant(m_dict, sense, a);
            antList << QString::fromUtf8(an.ptr, an.len);
        }
        sMap["ant"] = antList;

        // FIELD
        QStringList fieldList;
        for (uint32_t f = 0; f < sense->field_count; ++f) {
            kotoba_str fld = kotoba_field(m_dict, sense, f);
            fieldList << QString::fromUtf8(fld.ptr, fld.len);
        }
        sMap["field"] = fieldList;

        // MISC
        QStringList miscList;
        for (uint32_t m = 0; m < sense->misc_count; ++m) {
            kotoba_str misc = kotoba_misc(m_dict, sense, m);
            miscList << QString::fromUtf8(misc.ptr, misc.len);
        }
        sMap["misc"] = miscList;

        // S_INF
        QStringList sinfList;
        for (uint32_t si = 0; si < sense->s_inf_count; ++si) {
            kotoba_str sinf = kotoba_s_inf(m_dict, sense, si);
            sinfList << QString::fromUtf8(sinf.ptr, sinf.len);
        }
        sMap["s_inf"] = sinfList;

        // LSOURCE
        QStringList lsourceList;
        for (uint32_t ls = 0; ls < sense->lsource_count; ++ls) {
            kotoba_str lsrc = kotoba_lsource(m_dict, sense, ls);
            lsourceList << QString::fromUtf8(lsrc.ptr, lsrc.len);
        }
        sMap["lsource"] = lsourceList;

        // DIAL
        QStringList dialList;
        for (uint32_t d = 0; d < sense->dial_count; ++d) {
            kotoba_str dial = kotoba_dial(m_dict, sense, d);
            dialList << QString::fromUtf8(dial.ptr, dial.len);
        }
        sMap["dial"] = dialList;

        senses.append(sMap);
    }
    result["senses"] = senses;

    return result;
}