#pragma once
#include <QString>
#include <vector>
#include <mutex>
#include <QObject>        // ← nuevo: para poder emitir señales
#include <QFuture>

extern "C" {
#include <index_search.h>
#include <loader.h>
}

struct QueryVariants {
    QString normal;
    QString romaji;
    QString hiragana;
};

struct Configuration;

class SearchService : public QObject   // ← hereda QObject ahora
{
    Q_OBJECT
public:
    explicit SearchService(QObject *parent = nullptr);
    ~SearchService();

    void initialize(kotoba_dict *dict, Configuration* config);

    // Versión async: lanza el query en un hilo y emite searchDone() al terminar.
    // El caller NO debe leer searchCtx() hasta recibir searchDone().
    void queryAsync(const QString &q);

    // Versión síncrona — solo para uso interno / SrsLibraryViewModel bajo mutex.
    void query(const QString &q);
    void queryNonPagination(const QString &q);
    void queryNextPage();

    // Debe llamarse solo desde el main thread después de searchDone().
    void updateConfig(const Configuration* config);

    const SearchContext* searchCtx() const { return &m_ctx; }
    QueryVariants lastVariants() const;
    QString highlightField(const QString &field) const;

    // Mutex para que SrsLibraryViewModel pueda tomar el lock en sus hilos
    std::mutex& mutex() { return m_mutex; }

signals:
    void searchDone();       // emitido en el main thread al terminar queryAsync
    void pageReady();        // emitido cuando queryNextPage termina en hilo

private:
    SearchContext    m_ctx;
    kotoba_dict     *m_dict   = nullptr;
    const Configuration *m_config = nullptr;
    mutable std::mutex   m_mutex; // protege m_ctx completo
    QFuture<void>        m_future; // para no solapar queries
};