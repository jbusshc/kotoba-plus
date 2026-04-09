#include "EntryDetailsViewModel.h"

extern "C" {
#include "../../core/include/viewer.h"
}
#include "../../gui-qml/app/Configuration.h"

#include <QStringList>

EntryDetailsViewModel::EntryDetailsViewModel(
    QObject *parent
) : QObject(parent)
{
}

void EntryDetailsViewModel::initialize(kotoba_dict *dict, Configuration *config)
{
    m_dict   = dict;
    m_config = config;
}

/* -------------------------
 * HEADWORD HELPER
 * ------------------------- */
QString EntryDetailsViewModel::headword(uint32_t entSeq)
{
    const entry_bin *e = kotoba_dict_get_entry_by_entseq(m_dict, entSeq);
    if (!e) return {};

    if (e->k_elements_count > 0) {
        const k_ele_bin *k = kotoba_k_ele(m_dict, e, 0);
        kotoba_str s = kotoba_keb(m_dict, k);
        return QString::fromUtf8(s.ptr, s.len);
    }

    if (e->r_elements_count > 0) {
        const r_ele_bin *r = kotoba_r_ele(m_dict, e, 0);
        kotoba_str s = kotoba_reb(m_dict, r);
        return QString::fromUtf8(s.ptr, s.len);
    }

    return {};
}

/* -------------------------
 * MAP ENTRY
 * ------------------------- */
QVariantMap EntryDetailsViewModel::mapEntry(int entSeq)
{
    QVariantMap result;

    const entry_bin *entry = kotoba_dict_get_entry_by_entseq(m_dict, static_cast<uint32_t>(entSeq));
    if (!entry)
        return result;

    result["id"] = entSeq;   // ent_seq — el caller lo usa para llamar a SrsViewModel::add()

    /* -------------------------
     * K ELEMENTS
     * ------------------------- */
    QVariantList kElements;
    for (uint32_t i = 0; i < entry->k_elements_count; ++i) {
        QVariantMap kMap;
        const k_ele_bin *k = kotoba_k_ele(m_dict, entry, i);

        kotoba_str keb = kotoba_keb(m_dict, k);
        kMap["keb"] = QString::fromUtf8(keb.ptr, keb.len);

        QStringList priList;
        for (uint32_t p = 0; p < k->ke_pri_count; ++p) {
            const char *pri = kotoba_ke_pri(m_dict, k, p);
            if (pri) priList << QString::fromUtf8(pri);
        }
        kMap["ke_pri"] = priList;

        kElements.append(kMap);
    }
    result["k_elements"] = kElements;

    /* -------------------------
     * R ELEMENTS
     * ------------------------- */
    QVariantList rElements;
    for (uint32_t i = 0; i < entry->r_elements_count; ++i) {
        QVariantMap rMap;
        const r_ele_bin *r = kotoba_r_ele(m_dict, entry, i);

        kotoba_str reb = kotoba_reb(m_dict, r);
        rMap["reb"] = QString::fromUtf8(reb.ptr, reb.len);

        QStringList priList;
        for (uint32_t p = 0; p < r->re_pri_count; ++p) {
            const char *pri = kotoba_re_pri(m_dict, r, p);
            if (pri) priList << QString::fromUtf8(pri);
        }
        rMap["re_pri"] = priList;

        rElements.append(rMap);
    }
    result["r_elements"] = rElements;

    /* -------------------------
     * SENSES
     * ------------------------- */
    QVariantList senses;

    for (uint32_t i = 0; i < entry->senses_count; ++i) {
        QVariantMap sMap;
        const sense_bin *sense = kotoba_sense(m_dict, entry, i);
        if (!m_config->languages[sense->lang])
            continue;

        /* POS */
        QStringList posList;
        for (uint32_t p = 0; p < sense->pos_count; ++p) {
            const char *pos = kotoba_pos(m_dict, sense, p);
            if (pos) posList << QString::fromUtf8(pos);
        }
        sMap["pos"] = posList;

        /* GLOSS */
        QStringList glossList;
        for (uint32_t g = 0; g < sense->gloss_count; ++g) {
            kotoba_str gl = kotoba_gloss(m_dict, sense, g);
            glossList << QString::fromUtf8(gl.ptr, gl.len);
        }
        sMap["gloss"] = glossList;

        /* STAGK */
        QStringList stagkList;
        for (uint32_t sk = 0; sk < sense->stagk_count; ++sk) {
            kotoba_str sks = kotoba_stagk(m_dict, entry, i, sk);
            stagkList << QString::fromUtf8(sks.ptr, sks.len);
        }
        sMap["stagk"] = stagkList;

        /* STAGR */
        QStringList stagrList;
        for (uint32_t sr = 0; sr < sense->stagr_count; ++sr) {
            kotoba_str srs = kotoba_stagr(m_dict, entry, i, sr);
            stagrList << QString::fromUtf8(srs.ptr, srs.len);
        }
        sMap["stagr"] = stagrList;

        /* XREF (ent_seq → headword) */
        QVariantList xrefList;
        for (uint32_t x = 0; x < sense->xref_count; ++x) {
            uint32_t refEntSeq = kotoba_xref(m_dict, sense, x);
            QString hw = headword(refEntSeq);
            if (hw.isEmpty()) continue;
            QVariantMap item;
            item["id"]    = static_cast<int>(refEntSeq);   // ent_seq para navegación
            item["label"] = hw;
            xrefList.append(item);
        }
        sMap["xref"] = xrefList;

        /* ANT */
        QVariantList antList;
        for (uint32_t a = 0; a < sense->ant_count; ++a) {
            uint32_t refEntSeq = kotoba_ant(m_dict, sense, a);
            QString hw = headword(refEntSeq);
            if (hw.isEmpty()) continue;
            QVariantMap item;
            item["id"]    = static_cast<int>(refEntSeq);   // ent_seq para navegación
            item["label"] = hw;
            antList.append(item);
        }
        sMap["ant"] = antList;

        /* FIELD */
        QStringList fieldList;
        for (uint32_t f = 0; f < sense->field_count; ++f) {
            const char *fld = kotoba_field(m_dict, sense, f);
            if (fld) fieldList << QString::fromUtf8(fld);
        }
        sMap["field"] = fieldList;

        /* MISC */
        QStringList miscList;
        for (uint32_t m = 0; m < sense->misc_count; ++m) {
            const char *misc = kotoba_misc(m_dict, sense, m);
            if (misc) miscList << QString::fromUtf8(misc);
        }
        sMap["misc"] = miscList;

        /* S_INF */
        QStringList sinfList;
        for (uint32_t si = 0; si < sense->s_inf_count; ++si) {
            kotoba_str sinf = kotoba_s_inf(m_dict, sense, si);
            sinfList << QString::fromUtf8(sinf.ptr, sinf.len);
        }
        sMap["s_inf"] = sinfList;

        /* LSOURCE */
        QStringList lsourceList;
        for (uint32_t ls = 0; ls < sense->lsource_count; ++ls) {
            kotoba_str lsrc = kotoba_lsource(m_dict, sense, ls);
            lsourceList << QString::fromUtf8(lsrc.ptr, lsrc.len);
        }
        sMap["lsource"] = lsourceList;

        /* DIAL */
        QStringList dialList;
        for (uint32_t d = 0; d < sense->dial_count; ++d) {
            const char *dial = kotoba_dial(m_dict, sense, d);
            if (dial) dialList << QString::fromUtf8(dial);
        }
        sMap["dial"] = dialList;

        senses.append(sMap);
    }

    result["senses"] = senses;

    return result;
}