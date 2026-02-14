#include "search_presenter.h"

// ------------------------------------------------------------
// Constructor
// ------------------------------------------------------------
SearchPresenter::SearchPresenter(KotobaSearchService *service,
                                 QObject *parent)
    : QObject(parent),
      service(service),
      dict(&service->appContext()->dictionary), 
      lastCachedResultIndex(0)
{
}

// ------------------------------------------------------------
// Slot: texto de búsqueda cambió
// ------------------------------------------------------------
void SearchPresenter::onSearchTextChanged(const QString &text)
{
    if (text.isEmpty()) {
        // Si el texto está vacío, limpiamos resultados y avisamos
        currentResults.clear();
        lastCachedResultIndex = 0;
        emit resultsReset();
        return;
    }

    // 1. Limpiar estado actual
    currentResults.clear();
    lastCachedResultIndex = 0; // reset paginación

    // 2. Ejecutar nueva búsqueda en el servicio
    service->query(text.toStdString());
    service->queryNextPage(); // cargar la primera página de resultados

    // 3. Llenar resultados iniciales
    appendNewResults();

    // 4. Avisar a la vista/modelo que todo cambió
    emit resultsAppended(0, currentResults.size() - 1);
}

// ------------------------------------------------------------
// Slot: el usuario hizo click en un resultado
// ------------------------------------------------------------
void SearchPresenter::onResultClicked(int row)
{
    if (row < 0 || row >= currentResults.size())
        return;

    // Emitimos solo el doc_id
    emit entrySelected(currentResults[row].doc_id);
}

// ------------------------------------------------------------
// Slot: la vista pide más resultados (scroll / paginación)
// ------------------------------------------------------------
void SearchPresenter::onNeedMoreResults()
{
    // Si el servicio aún puede producir más resultados
    if (!service->hasMoreResults())
        return;

    int oldCount = currentResults.size();


    service->queryNextPage(); // cargar la siguiente página de resultados
    // 2. Agregar los nuevos resultados al cache
    appendNewResults();

    int newCount = currentResults.size();

    // 3. Avisar exactamente qué filas se agregaron
    if (newCount > oldCount)
    {
        emit resultsAppended(oldCount, newCount - 1);
    }
}

// ------------------------------------------------------------
// Acceso read-only para el modelo Qt
// ------------------------------------------------------------
const QVector<ResultRow> &SearchPresenter::results() const
{
    return currentResults;
}

// ------------------------------------------------------------
// Método interno: agrega nuevos resultados desde el servicio
// ------------------------------------------------------------
void SearchPresenter::appendNewResults()
{
    if (service->searchCtx()->results_processed <= lastCachedResultIndex)
        return; // no hay nuevos resultados para agregar
    
    const SearchContext *searchCtx = service->searchCtx(); 
    const uint32_t *docIds = searchCtx->results_doc_ids;

    const int newpageResultIndex = lastCachedResultIndex;
    const int toAppend = std::min((int)searchCtx->results_processed - newpageResultIndex, (int)searchCtx->page_size);

    printf("appendNewResults called. results_left: %d, results_processed: %d, lastCachedResultIndex: %d\n", searchCtx->results_left, searchCtx->results_processed, lastCachedResultIndex);
    for (int i = 0; i < toAppend; ++i)
    {
        uint32_t docId = docIds[newpageResultIndex + i];
        ResultRow row;
        row.doc_id = docId;

        // Lookup en el diccionario para llenar headword y gloss
        const entry_bin *entry = kotoba_dict_get_entry(dict, docId);

        // if kanji
        if (entry->k_elements_count > 0)
        {
            const k_ele_bin *k_ele = kotoba_k_ele(dict, entry, 0);
            kotoba_str keb = kotoba_keb(dict, k_ele);
            row.headword = QString::fromUtf8(keb.ptr, keb.len);
        }
        // else reading
        else
        {
            const r_ele_bin *r_ele = kotoba_r_ele(dict, entry, 0);
            kotoba_str reb = kotoba_reb(dict, r_ele);
            row.headword = QString::fromUtf8(reb.ptr, reb.len);
        }

        // gloss
        const sense_bin *sense = kotoba_sense(dict, entry, 0);
        if (sense->gloss_count > 0)
        {
            QString gloss;
            for (int g = 0; g < sense->gloss_count; ++g)
            {
                kotoba_str glossPart = kotoba_gloss(dict, sense, g);
                gloss += QString::fromUtf8(glossPart.ptr, glossPart.len);
                if (g < sense->gloss_count - 1)
                {
                    gloss += "; ";
                }
            }
            row.gloss = gloss;
        }

        currentResults.push_back(std::move(row));

    }
    lastCachedResultIndex += toAppend;
}

EntryDetails SearchPresenter::buildEntryDetails(uint32_t docId) const
{
    EntryDetails details;

    const entry_bin *entry = kotoba_dict_get_entry(dict, docId);
    if (!entry)
        return details;

    // ======================
    // MAIN WORD
    // ======================
    if (entry->k_elements_count > 0)
    {
        const k_ele_bin *k_ele = kotoba_k_ele(dict, entry, 0);
        kotoba_str keb = kotoba_keb(dict, k_ele);
        details.mainWord = QString::fromUtf8(keb.ptr, keb.len);
    }
    else if (entry->r_elements_count > 0)
    {
        const r_ele_bin *r_ele = kotoba_r_ele(dict, entry, 0);
        kotoba_str reb = kotoba_reb(dict, r_ele);
        details.mainWord = QString::fromUtf8(reb.ptr, reb.len);
    }

    // ======================
    // READINGS
    // ======================
    QString readings;

    for (uint32_t i = 0; i < entry->r_elements_count; ++i)
    {
        const r_ele_bin *r_ele = kotoba_r_ele(dict, entry, i);
        kotoba_str reb = kotoba_reb(dict, r_ele);

        readings += QString::fromUtf8(reb.ptr, reb.len);

        if (i < entry->r_elements_count - 1)
            readings += " ・ ";
    }

    details.readings = readings;

    // ======================
    // SENSES
    // ======================

    bool *langs = service->appContext()->languages;

    for (uint32_t s = 0; s < entry->senses_count; ++s)
    {

        const sense_bin *sense = kotoba_sense(dict, entry, s);

        if (!langs[sense->lang])
            continue;

        QString senseText;

        for (uint32_t g = 0; g < sense->gloss_count; ++g)
        {
            kotoba_str gloss = kotoba_gloss(dict, sense, g);
            senseText += QString::fromUtf8(gloss.ptr, gloss.len);

            if (g < sense->gloss_count - 1)
                senseText += "; ";
        }

        details.senses.push_back(senseText);
    }

    return details;
}
