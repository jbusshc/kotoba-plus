#include "SrsLibraryViewModel.h"
#include "../infrastructure/SrsService.h"
#include "../infrastructure/SearchService.h"
#include "../infrastructure/CardType.h"
#include "../app/Configuration.h"
#include <QDebug>
#include <QDateTime>
#include <QThreadPool>

extern "C" {
#include "../../core/include/viewer.h"
#include "../../core/include/index_search.h"
}

static QString accentToHex(const QString &name)
{
    static const QHash<QString, QString> table = {
        { "red",        "#F44336" }, { "pink",       "#E91E63" },
        { "purple",     "#9C27B0" }, { "deeppurple", "#673AB7" },
        { "indigo",     "#3F51B5" }, { "blue",       "#2196F3" },
        { "lightblue",  "#03A9F4" }, { "cyan",       "#00BCD4" },
        { "teal",       "#009688" }, { "green",      "#4CAF50" },
        { "lightgreen", "#8BC34A" }, { "lime",       "#CDDC39" },
        { "yellow",     "#FFEB3B" }, { "amber",      "#FFC107" },
        { "orange",     "#FF9800" }, { "deeporange", "#FF5722" },
        { "brown",      "#795548" }, { "bluegrey",   "#607D8B" },
    };
    return table.value(name, "#2196F3");
}

// ─────────────────────────────────────────────────────────────────────────────

SrsLibraryViewModel::SrsLibraryViewModel(QObject* parent)
    : QAbstractListModel(parent)
    , m_service(nullptr), m_dict(nullptr)
    , m_searchService(nullptr), m_config(nullptr)
{
    m_debounceTimer.setSingleShot(true);
    connect(&m_debounceTimer, &QTimer::timeout,
            this, &SrsLibraryViewModel::onDebounceTimeout);
}

void SrsLibraryViewModel::initialize(
        SrsService*    service,
        kotoba_dict*   dict,
        SearchService* searchService,
        Configuration* config
    )
{
    m_service       = service;
    m_dict          = dict;
    m_searchService = searchService;
    m_config        = config;

    loadAllCards();
    rebuildFiltered();
    resetToInitialPage();
}

// ── Debounce ──────────────────────────────────────────────────────────────────

void SrsLibraryViewModel::setSearch(const QString &text)
{
    m_pendingSearch = text;
    m_debounceTimer.start(120);
}

void SrsLibraryViewModel::onDebounceTimeout()
{
    const QString prev = m_search;
    m_search = m_pendingSearch;
    loadAllCards();
    rebuildFiltered();
    resetToInitialPage();
    if (m_search != prev)
        emit activeSearchChanged();
}

// ── highlightField ────────────────────────────────────────────────────────────

QString SrsLibraryViewModel::highlightField(const QString &field) const
{
    if (m_search.isEmpty() || field.isEmpty())
        return field;

    const auto variants = m_searchService->lastVariants();
    const QString fieldLower = field.toLower();
    int matchIdx = -1;
    int matchLen = 0;

    for (const QString &v : { variants.normal, variants.romaji, variants.hiragana }) {
        if (v.isEmpty()) continue;
        const int idx = fieldLower.indexOf(v.toLower());
        if (idx >= 0 && (matchIdx < 0 || idx < matchIdx)) {
            matchIdx = idx;
            matchLen = v.length();
        }
    }

    if (matchIdx < 0) return field;

    const QString colorHex = m_config ? accentToHex(m_config->accentColor)
                                      : QStringLiteral("#2196F3");
    QString result;
    result.reserve(field.size() + 48);
    result += field.left(matchIdx).toHtmlEscaped();
    result += QStringLiteral("<b style=\"color:") + colorHex + QStringLiteral("\">");
    result += field.mid(matchIdx, matchLen).toHtmlEscaped();
    result += QStringLiteral("</b>");
    result += field.mid(matchIdx + matchLen).toHtmlEscaped();
    return result;
}

// ── loadAllCards ──────────────────────────────────────────────────────────────

void SrsLibraryViewModel::loadAllCards()
{
    m_allCards.clear();
    if (!m_service || !m_service->getDeck() || !m_dict) return;

    uint32_t count = fsrs_deck_count(m_service->getDeck());
    for (uint32_t i = 0; i < count; ++i) {
        const fsrs_card* card = fsrs_deck_card(m_service->getDeck(), i);
        if (!card) continue;

        // Descomponer el fsrsId en entryId + cardType
        uint32_t fsrsId  = card->id;
        uint32_t entryId = CardTypeHelper::toEntryId(fsrsId);
        CardType cardType = CardTypeHelper::toCardType(fsrsId);

        const entry_bin* entry = kotoba_dict_get_entry(m_dict, entryId);
        if (!entry) continue;

        SrsCardItem item;
        item.fsrsId       = fsrsId;
        item.entryId      = entryId;
        item.cardType     = cardType;
        item.reps         = card->reps;
        item.lapses       = card->lapses;
        item.stability    = card->stability;
        item.difficulty   = card->difficulty;
        item.lastReview   = card->last_review;
        item.totalReviews = card->total_answers;

        switch (card->state) {
            case FSRS_STATE_NEW:        item.state = QStringLiteral("New");        break;
            case FSRS_STATE_LEARNING:   item.state = QStringLiteral("Learning");   break;
            case FSRS_STATE_RELEARNING: item.state = QStringLiteral("Relearning"); break;
            case FSRS_STATE_REVIEW:     item.state = QStringLiteral("Review");     break;
            case FSRS_STATE_SUSPENDED:  item.state = QStringLiteral("Suspended");  break;
            default:                    item.state = QStringLiteral("Unknown");    break;
        }

        // headword
        if (entry->k_elements_count > 0) {
            const k_ele_bin* k = kotoba_k_ele(m_dict, entry, 0);
            kotoba_str s = kotoba_keb(m_dict, k);
            item.word = QString::fromUtf8(s.ptr, s.len);
        } else if (entry->r_elements_count > 0) {
            const r_ele_bin* r = kotoba_r_ele(m_dict, entry, 0);
            kotoba_str s = kotoba_reb(m_dict, r);
            item.word = QString::fromUtf8(s.ptr, s.len);
        } else {
            item.word = QStringLiteral("[unknown]");
        }

        // variants UI: kanji alternativos k_ele[1..N]
        for (uint32_t k = 1; k < entry->k_elements_count; ++k) {
            const k_ele_bin* ke = kotoba_k_ele(m_dict, entry, k);
            kotoba_str s = kotoba_keb(m_dict, ke);
            item.variants << QString::fromUtf8(s.ptr, s.len);
        }

        // matchVariants: k_ele[0] + hiragana de cada r_ele
        char mixed_buf[MAX_QUERY_LEN];
        if (entry->k_elements_count > 0) {
            const k_ele_bin* ke0 = kotoba_k_ele(m_dict, entry, 0);
            kotoba_str s0 = kotoba_keb(m_dict, ke0);
            item.matchVariants << QString::fromUtf8(s0.ptr, s0.len);
        }

        // readingsList UI + matchVariants hiragana
        for (uint32_t r = 0; r < entry->r_elements_count; ++r) {
            const r_ele_bin* re = kotoba_r_ele(m_dict, entry, r);
            kotoba_str s = kotoba_reb(m_dict, re);
            item.readingsList << QString::fromUtf8(s.ptr, s.len);

            size_t copyLen = qMin<size_t>(s.len, MAX_QUERY_LEN - 1);
            memcpy(mixed_buf, s.ptr, copyLen);
            mixed_buf[copyLen] = '\0';
            mixed_to_hiragana(m_searchService->searchCtx()->trie_ctx,
                              mixed_buf, mixed_buf, MAX_QUERY_LEN);
            item.matchVariants << QString::fromUtf8(mixed_buf);
        }

        // meaning
        if (entry->senses_count > 0) {
            const sense_bin* sense = kotoba_sense(m_dict, entry, 0);
            QStringList glosses;
            for (uint32_t g = 0; g < sense->gloss_count; ++g) {
                kotoba_str gs = kotoba_gloss(m_dict, sense, g);
                glosses << QString::fromUtf8(gs.ptr, gs.len);
            }
            item.meaning = glosses.join(QStringLiteral("; "));
        }

        item.due = QString::fromStdString(m_service->dueDateText(fsrsId));
        m_allCards.append(item);
    }
}

// ── rebuildFiltered ───────────────────────────────────────────────────────────

void SrsLibraryViewModel::rebuildFiltered()
{
    m_filtered.clear();

    const QString s = m_search.trimmed();
    const bool hasSearch = !s.isEmpty();

    if (hasSearch)
        m_searchService->queryNonPagination(s);

    QSet<uint32_t> resultEntryIds;
    if (hasSearch) {
        const SearchContext* ctx = m_searchService->searchCtx();
        const PostingRef* results     = ctx->results_buffer;
        const uint32_t   resultsCount = ctx->results_left;
        for (uint32_t i = 0; i < resultsCount; ++i)
            resultEntryIds.insert(results[i].p->doc_id);
    }

    for (const SrsCardItem &it : m_allCards) {
        // La búsqueda usa entryId (sin el bit de tipo) para matchear contra el dict
        bool matchSearch = !hasSearch
            || resultEntryIds.contains(it.entryId)
            || variantMatch(it, s);

        bool matchFilter = true;
        if      (m_filter == QLatin1String("New"))
            matchFilter = (it.state == QLatin1String("New"));
        else if (m_filter == QLatin1String("Learning"))
            matchFilter = (it.state == QLatin1String("Learning") ||
                           it.state == QLatin1String("Relearning"));
        else if (m_filter == QLatin1String("Review"))
            matchFilter = (it.state == QLatin1String("Review"));
        else if (m_filter == QLatin1String("Suspended"))
            matchFilter = (it.state == QLatin1String("Suspended"));

        if (matchSearch && matchFilter)
            m_filtered.append(it);
    }
}

void SrsLibraryViewModel::resetToInitialPage()
{
    beginResetModel();
    m_visible.clear();
    int to = qMin(m_pageSize, m_filtered.size());
    for (int i = 0; i < to; ++i)
        m_visible.append(m_filtered.at(i));
    endResetModel();
}

// ── API pública ───────────────────────────────────────────────────────────────

void SrsLibraryViewModel::refresh()
{
    if (m_refreshPending.exchange(true)) return;

    QThreadPool::globalInstance()->start([this]() {

        // Cargar todas las cartas con lock del deck
        QVector<SrsCardItem> allCards;
        {
            std::lock_guard<std::mutex> lock(m_service->deckMutex());

            uint32_t count = fsrs_deck_count(m_service->getDeck());
            for (uint32_t i = 0; i < count; ++i) {
                const fsrs_card* card = fsrs_deck_card(m_service->getDeck(), i);
                if (!card) continue;

                uint32_t fsrsId   = card->id;
                uint32_t entryId  = CardTypeHelper::toEntryId(fsrsId);
                CardType cardType = CardTypeHelper::toCardType(fsrsId);

                const entry_bin* entry = kotoba_dict_get_entry(m_dict, entryId);
                if (!entry) continue;

                SrsCardItem item;
                item.fsrsId       = fsrsId;
                item.entryId      = entryId;
                item.cardType     = cardType;
                item.reps         = card->reps;
                item.lapses       = card->lapses;
                item.stability    = card->stability;
                item.difficulty   = card->difficulty;
                item.lastReview   = card->last_review;
                item.totalReviews = card->total_answers;

                switch (card->state) {
                    case FSRS_STATE_NEW:        item.state = QStringLiteral("New");        break;
                    case FSRS_STATE_LEARNING:   item.state = QStringLiteral("Learning");   break;
                    case FSRS_STATE_RELEARNING: item.state = QStringLiteral("Relearning"); break;
                    case FSRS_STATE_REVIEW:     item.state = QStringLiteral("Review");     break;
                    case FSRS_STATE_SUSPENDED:  item.state = QStringLiteral("Suspended");  break;
                    default:                    item.state = QStringLiteral("Unknown");    break;
                }

                if (entry->k_elements_count > 0) {
                    const k_ele_bin* k = kotoba_k_ele(m_dict, entry, 0);
                    kotoba_str s = kotoba_keb(m_dict, k);
                    item.word = QString::fromUtf8(s.ptr, s.len);
                } else if (entry->r_elements_count > 0) {
                    const r_ele_bin* r = kotoba_r_ele(m_dict, entry, 0);
                    kotoba_str s = kotoba_reb(m_dict, r);
                    item.word = QString::fromUtf8(s.ptr, s.len);
                } else {
                    item.word = QStringLiteral("[unknown]");
                }

                if (entry->senses_count > 0) {
                    const sense_bin* sense = kotoba_sense(m_dict, entry, 0);
                    QStringList glosses;
                    for (uint32_t g = 0; g < sense->gloss_count; ++g) {
                        kotoba_str gs = kotoba_gloss(m_dict, sense, g);
                        glosses << QString::fromUtf8(gs.ptr, gs.len);
                    }
                    item.meaning = glosses.join(QStringLiteral("; "));
                }

                item.due = QString::fromStdString(m_service->dueDateText(fsrsId));
                allCards.append(item);
            }
        }

        // Filtrado / búsqueda
        QVector<SrsCardItem> filtered;
        {
            const QString s = m_search.trimmed();
            const bool hasSearch = !s.isEmpty();

            QSet<uint32_t> resultIds;
            if (hasSearch) {
                m_searchService->queryNonPagination(s);

                std::lock_guard<std::mutex> lock(m_searchService->mutex());
                const SearchContext *ctx = m_searchService->searchCtx();
                for (uint32_t i = 0; i < ctx->results_left; ++i)
                    resultIds.insert(ctx->results_buffer[i].p->doc_id);
            }

            for (const SrsCardItem &it : allCards) {
                bool matchSearch = !hasSearch
                    || resultIds.contains(it.entryId)
                    || variantMatch(it, s);

                bool matchFilter = true;
                if      (m_filter == QLatin1String("New"))
                    matchFilter = (it.state == QLatin1String("New"));
                else if (m_filter == QLatin1String("Learning"))
                    matchFilter = (it.state == QLatin1String("Learning") ||
                                   it.state == QLatin1String("Relearning"));
                else if (m_filter == QLatin1String("Review"))
                    matchFilter = (it.state == QLatin1String("Review"));
                else if (m_filter == QLatin1String("Suspended"))
                    matchFilter = (it.state == QLatin1String("Suspended"));

                if (matchSearch && matchFilter)
                    filtered.append(it);
            }
        }

        QMetaObject::invokeMethod(this,
            [this, allCards = std::move(allCards),
                   filtered = std::move(filtered)]() mutable {
                applyRefresh(std::move(allCards), std::move(filtered));
            }, Qt::QueuedConnection);
    });
}

void SrsLibraryViewModel::applyRefresh(QVector<SrsCardItem> allCards,
                                       QVector<SrsCardItem> filtered)
{
    m_allCards  = std::move(allCards);
    m_filtered  = std::move(filtered);
    m_refreshPending = false;

    beginResetModel();
    m_visible.clear();
    int to = qMin(m_pageSize, m_filtered.size());
    for (int i = 0; i < to; ++i)
        m_visible.append(m_filtered.at(i));
    endResetModel();
}

void SrsLibraryViewModel::setFilter(const QString &filter)
{
    m_filter = filter;
    loadAllCards();
    rebuildFiltered();
    resetToInitialPage();
}

// ── Getters — por fsrsId ──────────────────────────────────────────────────────

int SrsLibraryViewModel::getReps(int fsrsId) const {
    for (const auto &it : m_allCards) if ((int)it.fsrsId == fsrsId) return it.reps; return 0; }
int SrsLibraryViewModel::getLapses(int fsrsId) const {
    for (const auto &it : m_allCards) if ((int)it.fsrsId == fsrsId) return it.lapses; return 0; }
int SrsLibraryViewModel::getTotalReviews(int fsrsId) const {
    for (const auto &it : m_allCards) if ((int)it.fsrsId == fsrsId) return (int)it.totalReviews; return 0; }
float SrsLibraryViewModel::getStability(int fsrsId) const {
    for (const auto &it : m_allCards) if ((int)it.fsrsId == fsrsId) return it.stability; return 0.f; }
float SrsLibraryViewModel::getDifficulty(int fsrsId) const {
    for (const auto &it : m_allCards) if ((int)it.fsrsId == fsrsId) return it.difficulty; return 0.f; }

QString SrsLibraryViewModel::getLastReview(int fsrsId) const {
    for (const auto &it : m_allCards) {
        if ((int)it.fsrsId != fsrsId) continue;
        if (it.lastReview == 0) return QStringLiteral("Never");
        return QDateTime::fromSecsSinceEpoch((qint64)it.lastReview)
                         .toString(QStringLiteral("MMM d, yyyy"));
    }
    return QStringLiteral("Never");
}

QString SrsLibraryViewModel::getDue(int fsrsId) const {
    return m_service
        ? QString::fromStdString(m_service->dueDateText((uint32_t)fsrsId))
        : QString{};
}

QString SrsLibraryViewModel::getState(int fsrsId) const {
    return m_service
        ? QString::fromStdString(m_service->stateText((uint32_t)fsrsId))
        : QString{};
}

QString SrsLibraryViewModel::getCardType(int fsrsId) const {
    return CardTypeHelper::toCardType((uint32_t)fsrsId) == CardType::Recall
        ? QStringLiteral("Recall")
        : QStringLiteral("Recognition");
}

// ── Acciones — toman fsrsId completo ──────────────────────────────────────────

void SrsLibraryViewModel::suspend(int fsrsId) {
    if (!m_service || fsrsId < 0) return;
    m_service->suspend(CardTypeHelper::toEntryId((uint32_t)fsrsId),
                       CardTypeHelper::toCardType((uint32_t)fsrsId));
    refresh();
}

void SrsLibraryViewModel::unsuspend(int fsrsId) {
    if (!m_service || fsrsId < 0) return;
    m_service->unsuspend(CardTypeHelper::toEntryId((uint32_t)fsrsId),
                         CardTypeHelper::toCardType((uint32_t)fsrsId));
    refresh();
}

void SrsLibraryViewModel::reset(int fsrsId) {
    if (!m_service || fsrsId < 0) return;
    if (m_service->reset(CardTypeHelper::toEntryId((uint32_t)fsrsId),
                         CardTypeHelper::toCardType((uint32_t)fsrsId)))
        refresh();
}

void SrsLibraryViewModel::remove(int fsrsId) {
    if (!m_service || fsrsId < 0) return;
    if (m_service->remove(CardTypeHelper::toEntryId((uint32_t)fsrsId),
                          CardTypeHelper::toCardType((uint32_t)fsrsId)))
        refresh();
}

// ── Model interface ───────────────────────────────────────────────────────────

int SrsLibraryViewModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent) return m_visible.size();
}

QVariant SrsLibraryViewModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_visible.size()) return {};
    const SrsCardItem &c = m_visible.at(index.row());
    switch (role) {
        case EntryIdRole:  return (int)c.entryId;   // siempre sin bit de tipo
        case FsrsIdRole:   return (int)c.fsrsId;    // con bit de tipo (para acciones)
        case WordRole:     return c.word;
        case MeaningRole:  return c.meaning;
        case StateRole:    return c.state;
        case DueRole:      return c.due;
        case RepsRole:     return (int)c.reps;
        case LapsesRole:   return (int)c.lapses;
        case VariantsRole: return c.variants.join(QStringLiteral("・"));
        case ReadingsRole: return c.readingsList.join(QStringLiteral("・"));
        case CardTypeRole: return (c.cardType == CardType::Recall)
                               ? QStringLiteral("Recall")
                               : QStringLiteral("Recognition");
        default:           return {};
    }
}

QHash<int, QByteArray> SrsLibraryViewModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[EntryIdRole]  = "entryId";
    roles[FsrsIdRole]   = "fsrsId";
    roles[WordRole]     = "word";
    roles[MeaningRole]  = "meaning";
    roles[StateRole]    = "state";
    roles[DueRole]      = "due";
    roles[RepsRole]     = "reps";
    roles[LapsesRole]   = "lapses";
    roles[VariantsRole] = "variants";
    roles[ReadingsRole] = "readings";
    roles[CardTypeRole] = "cardType";
    return roles;
}

bool SrsLibraryViewModel::canFetchMore(const QModelIndex &parent) const
{
    Q_UNUSED(parent) return m_visible.size() < m_filtered.size();
}

void SrsLibraryViewModel::fetchMore(const QModelIndex &parent)
{
    Q_UNUSED(parent)
    int remaining = m_filtered.size() - m_visible.size();
    if (remaining <= 0) return;
    int n = qMin(remaining, m_pageSize);
    int begin = m_visible.size();
    beginInsertRows(QModelIndex(), begin, begin + n - 1);
    for (int i = 0; i < n; ++i) m_visible.append(m_filtered.at(begin + i));
    endInsertRows();
}

bool SrsLibraryViewModel::variantMatch(const SrsCardItem &it, const QString &q) const
{
    const QString qL = q.toLower();
    for (const QString &v : it.matchVariants)
        if (v.toLower().contains(qL)) return true;
    return false;
}