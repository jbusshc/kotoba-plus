#pragma once

#include <QObject>
#include <QString>
#include <QVector>
#include <cstdint>

#include "search_service.h"


#ifdef __cplusplus
extern "C" {
    #include "loader.h"
    #include "viewer.h"
}
#endif 

struct ResultRow {
    uint32_t doc_id;

    QString headword;   // kanji o lectura principal
    QString gloss;      // glosa ya en idioma elegido
};

struct EntryDetails
{
    QString mainWord;
    QString readings;
    QVector<QString> senses;
};


class KotobaSearchService;

class SearchPresenter : public QObject
{
    Q_OBJECT

public:
    explicit SearchPresenter(KotobaSearchService *service,
                             QObject *parent = nullptr);

public slots:
    // Desde la View
    void onSearchTextChanged(const QString &text);
    void onResultClicked(int row);
    void onNeedMoreResults(); // scroll / paginación

signals:
    // Hacia la View / Model
    void resultsReset();
    void resultsAppended(int first, int last);
    void entrySelected(uint32_t doc_id);

public:
    // Acceso de solo lectura para el modelo
    const QVector<ResultRow>& results() const;
    
    // Para mostrar detalles de una entrada
    EntryDetails buildEntryDetails(uint32_t docId) const;


private:
    KotobaSearchService *service;
    
    QVector<ResultRow> currentResults;
    kotoba_dict *dict; // acceso directo para lookup de glosas

private:
    void appendNewResults(); 
    // 👆 ACÁ se llenan los strings (lookup diccionario)
};
