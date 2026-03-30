#pragma once
#include <QObject>
#include <QAbstractListModel>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QTimer>
#include <cstdint>

extern "C" {
#include "../../core/include/loader.h"
#include "../../core/include/fsrs.h"
}

class SrsService;
class SearchService;

struct SrsCardItem {
    uint32_t id      = 0;
    QString  word;       // headword: k_ele[0] o r_ele[0]
    QString  meaning;
    QString  state;
    QString  due;
    uint16_t reps    = 0;
    uint16_t lapses  = 0;

    uint32_t totalReviews = 0;
    float    stability    = 0.f;
    float    difficulty   = 0.f;
    uint64_t lastReview   = 0;

    // Para mostrar en UI ──────────────────────────────────────────────────────
    QStringList variants;      // k_ele[1..N]: formas kanji alternativas
    QStringList readingsList;  // r_ele: todas las lecturas originales

    // Para variantMatch() — incluye k_ele[0] + hiragana de cada r_ele ────────
    QStringList matchVariants;
};

class SrsLibraryViewModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        EntryIdRole = Qt::UserRole + 1,
        WordRole,
        MeaningRole,
        StateRole,
        DueRole,
        RepsRole,
        LapsesRole,
        VariantsRole,   // QString "v1・v2" — kanji alternativos k_ele[1..N]
        ReadingsRole    // QString "r1・r2" — lecturas r_ele
    };

    explicit SrsLibraryViewModel(
        SrsService*    service,
        kotoba_dict*   dict,
        SearchService* searchService,
        QObject*       parent = nullptr
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

    Q_INVOKABLE void suspend(int entryId);
    Q_INVOKABLE void unsuspend(int entryId);
    Q_INVOKABLE void reset(int entryId);
    Q_INVOKABLE void remove(int entryId);
    Q_INVOKABLE void refresh();

    int      rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;
    bool     canFetchMore(const QModelIndex &parent) const override;
    void     fetchMore(const QModelIndex &parent) override;

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

    QVector<SrsCardItem> m_allCards;
    QVector<SrsCardItem> m_filtered;
    QVector<SrsCardItem> m_visible;

    QString m_search;
    QString m_pendingSearch;
    QString m_filter;
    QTimer  m_debounceTimer;

    const int m_pageSize = 200;
};