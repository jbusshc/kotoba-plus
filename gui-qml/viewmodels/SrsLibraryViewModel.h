#pragma once
#include <QObject>
#include <QAbstractListModel>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QTimer>
#include <cstdint>

#include <mutex>
#include <atomic>


extern "C" {
#include "../../core/include/loader.h"
#include "../../core/include/fsrs.h"
}

class SrsService;
class SearchService;
class Configuration;

struct SrsCardItem {
    uint32_t id      = 0;
    QString  word;
    QString  meaning;
    QString  state;
    QString  due;
    uint16_t reps    = 0;
    uint16_t lapses  = 0;

    uint32_t totalReviews = 0;
    float    stability    = 0.f;
    float    difficulty   = 0.f;
    uint64_t lastReview   = 0;

    QStringList variants;      // k_ele[1..N]: kanji alternativos para UI
    QStringList readingsList;  // r_ele: lecturas originales para UI
    QStringList matchVariants; // k_ele[0] + hiragana de cada r_ele, para variantMatch()
};

class SrsLibraryViewModel : public QAbstractListModel
{
    Q_OBJECT

    // Query activa post-debounce — el delegate la lee para saber si highlight está activo
    Q_PROPERTY(QString activeSearch READ activeSearch NOTIFY activeSearchChanged)

public:
    enum Roles {
        EntryIdRole = Qt::UserRole + 1,
        WordRole,
        MeaningRole,
        StateRole,
        DueRole,
        RepsRole,
        LapsesRole,
        VariantsRole,
        ReadingsRole
    };

    explicit SrsLibraryViewModel(QObject* parent = nullptr);
    void initialize(
        SrsService*    service,
        kotoba_dict*   dict,
        SearchService* searchService,
        Configuration* config
    );

    Q_INVOKABLE void    setSearch(const QString &text);
    Q_INVOKABLE void    setFilter(const QString &filter);

    Q_INVOKABLE int     getReps(int entryId)         const;
    Q_INVOKABLE int     getLapses(int entryId)       const;
    Q_INVOKABLE int     getTotalReviews(int entryId) const;
    Q_INVOKABLE float   getStability(int entryId)    const;
    Q_INVOKABLE float   getDifficulty(int entryId)   const;
    Q_INVOKABLE QString getLastReview(int entryId)   const;
    Q_INVOKABLE QString getDue(int entryId)          const;
    Q_INVOKABLE QString getState(int entryId)        const;

    Q_INVOKABLE void    suspend(int entryId);
    Q_INVOKABLE void    unsuspend(int entryId);
    Q_INVOKABLE void    reset(int entryId);
    Q_INVOKABLE void    remove(int entryId);
    Q_INVOKABLE void    refresh();

    // Igual que SearchViewModel::highlightField — usa lastVariants() del SearchService
    // tras la última llamada a queryNonPagination() en rebuildFiltered().
    Q_INVOKABLE QString highlightField(const QString &field) const;

    QString activeSearch() const { return m_search; }

    int      rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;
    bool     canFetchMore(const QModelIndex &parent) const override;
    void     fetchMore(const QModelIndex &parent) override;
    void applyRefresh(QVector<SrsCardItem> allCards,
                      QVector<SrsCardItem> filtered);
signals:
    void activeSearchChanged();

private slots:
    void onDebounceTimeout();

private:
    void loadAllCards();
    void rebuildFiltered();
    void resetToInitialPage();
    bool variantMatch(const SrsCardItem &it, const QString &q) const;

    SrsService*    m_service       = nullptr;
    SearchService* m_searchService = nullptr;
    kotoba_dict*   m_dict          = nullptr;
    Configuration* m_config        = nullptr;

    QVector<SrsCardItem> m_allCards;
    QVector<SrsCardItem> m_filtered;
    QVector<SrsCardItem> m_visible;

    QString m_search;        // query activa (post-debounce)
    QString m_pendingSearch; // texto del campo, esperando debounce
    QString m_filter;
    QTimer  m_debounceTimer;

    const int m_pageSize = 200;

    std::atomic<bool> m_refreshPending{false};

};