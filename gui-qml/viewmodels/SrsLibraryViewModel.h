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

// ─────────────────────────────────────────────────────────────────────────────
// SrsCardItem — una fila por ent_seq, con estado de ambas cartas.
// ─────────────────────────────────────────────────────────────────────────────

struct SrsCardItemDeckState {
    bool    present   = false;
    QString state;          // "New" | "Learning" | "Relearning" | "Review" | "Suspended"
    QString due;
    uint16_t reps     = 0;
    uint16_t lapses   = 0;
    uint32_t totalReviews = 0;
    float    stability    = 0.f;
    float    difficulty   = 0.f;
    uint64_t lastReview   = 0;
};

struct SrsCardItem {
    uint32_t entSeq = 0;    // ent_seq del JMdict — ID estable

    QString  word;
    QString  meaning;

    SrsCardItemDeckState recog;
    SrsCardItemDeckState recall;

    QStringList variants;
    QStringList readingsList;
    QStringList matchVariants;
};

// ─────────────────────────────────────────────────────────────────────────────
// SrsLibraryViewModel
// ─────────────────────────────────────────────────────────────────────────────

class SrsLibraryViewModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(QString activeSearch READ activeSearch NOTIFY activeSearchChanged)

public:
    enum Roles {
        EntryIdRole       = Qt::UserRole + 1,  // ent_seq
        WordRole,
        MeaningRole,
        VariantsRole,
        ReadingsRole,

        // Recognition
        RecogPresentRole,
        RecogStateRole,
        RecogDueRole,
        RecogRepsRole,
        RecogLapsesRole,

        // Recall
        RecallPresentRole,
        RecallStateRole,
        RecallDueRole,
        RecallRepsRole,
        RecallLapsesRole,
    };

    explicit SrsLibraryViewModel(QObject *parent = nullptr);

    void initialize(
        SrsService    *service,
        kotoba_dict   *dict,
        SearchService *searchService,
        Configuration *config
    );

    Q_INVOKABLE void    setSearch(const QString &text);
    Q_INVOKABLE void    setFilter(const QString &filter);

    // Getters de detalle por ent_seq + tipo
    Q_INVOKABLE int     getReps        (int entSeq, int cardType) const;
    Q_INVOKABLE int     getLapses      (int entSeq, int cardType) const;
    Q_INVOKABLE int     getTotalReviews(int entSeq, int cardType) const;
    Q_INVOKABLE float   getStability   (int entSeq, int cardType) const;
    Q_INVOKABLE float   getDifficulty  (int entSeq, int cardType) const;
    Q_INVOKABLE QString getLastReview  (int entSeq, int cardType) const;
    Q_INVOKABLE QString getDue         (int entSeq, int cardType) const;
    Q_INVOKABLE QString getState       (int entSeq, int cardType) const;

    // Acciones por ent_seq + cardType (0=Recognition, 1=Recall)
    Q_INVOKABLE void    suspend  (int entSeq, int cardType);
    Q_INVOKABLE void    unsuspend(int entSeq, int cardType);
    Q_INVOKABLE void    reset    (int entSeq, int cardType);
    Q_INVOKABLE void    remove   (int entSeq, int cardType);

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

    CardType cardTypeFromInt(int v) const {
        return (v == 1) ? CardType::Recall : CardType::Recognition;
    }

    const SrsCardItemDeckState &stateFor(const SrsCardItem &it, int cardType) const {
        return (cardType == 1) ? it.recall : it.recog;
    }

    SrsService    *m_service       = nullptr;
    SearchService *m_searchService = nullptr;
    kotoba_dict   *m_dict          = nullptr;
    Configuration *m_config        = nullptr;

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