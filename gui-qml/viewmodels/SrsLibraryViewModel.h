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

#include "../infrastructure/CardType.h"

extern "C" {
#include "../../core/include/loader.h"
#include "../../core/include/fsrs.h"
}

class SrsService;
class SearchService;
class Configuration;

struct SrsCardItem {
    uint32_t fsrsId  = 0;      // id completo en el deck (con bit de tipo)
    uint32_t entryId = 0;      // id del entry en el diccionario (sin bit de tipo)
    CardType cardType = CardType::Recognition;

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

    QStringList variants;
    QStringList readingsList;
    QStringList matchVariants;
};

class SrsLibraryViewModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(QString activeSearch READ activeSearch NOTIFY activeSearchChanged)

public:
    enum Roles {
        // Los roles QML usan entryId (sin bit de tipo) para identificar la entry
        EntryIdRole  = Qt::UserRole + 1,
        WordRole,
        MeaningRole,
        StateRole,
        DueRole,
        RepsRole,
        LapsesRole,
        VariantsRole,
        ReadingsRole,
        CardTypeRole,    // "Recognition" o "Recall" — nuevo
        FsrsIdRole,      // id completo con bit de tipo — para operaciones internas
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

    // Getters por fsrsId (id completo con bit de tipo)
    Q_INVOKABLE int     getReps        (int fsrsId) const;
    Q_INVOKABLE int     getLapses      (int fsrsId) const;
    Q_INVOKABLE int     getTotalReviews(int fsrsId) const;
    Q_INVOKABLE float   getStability   (int fsrsId) const;
    Q_INVOKABLE float   getDifficulty  (int fsrsId) const;
    Q_INVOKABLE QString getLastReview  (int fsrsId) const;
    Q_INVOKABLE QString getDue         (int fsrsId) const;
    Q_INVOKABLE QString getState       (int fsrsId) const;
    Q_INVOKABLE QString getCardType    (int fsrsId) const;

    // Acciones — toman fsrsId para operar sobre el tipo correcto
    Q_INVOKABLE void    suspend  (int fsrsId);
    Q_INVOKABLE void    unsuspend(int fsrsId);
    Q_INVOKABLE void    reset    (int fsrsId);
    Q_INVOKABLE void    remove   (int fsrsId);

    Q_INVOKABLE void    refresh();

    Q_INVOKABLE QString highlightField(const QString &field) const;

    QString activeSearch() const { return m_search; }

    int      rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;
    bool     canFetchMore(const QModelIndex &parent) const override;
    void     fetchMore(const QModelIndex &parent) override;

    void applyRefresh(QVector<SrsCardItem> allCards, QVector<SrsCardItem> filtered);

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

    QString m_search;
    QString m_pendingSearch;
    QString m_filter;
    QTimer  m_debounceTimer;

    const int m_pageSize = 200;
    std::atomic<bool> m_refreshPending{false};
};