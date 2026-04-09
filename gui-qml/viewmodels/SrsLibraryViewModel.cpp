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
// helpers: leer estado de una fsrs_card en un SrsCardItemDeckState
// ─────────────────────────────────────────────────────────────────────────────

static void fillDeckState(SrsCardItemDeckState &dst,
                           const fsrs_card      *card,
                           const std::string    &dueText)
{
    dst.present      = true;
    dst.reps         = card->reps;
    dst.lapses       = card->lapses;
    dst.stability    = card->stability;
    dst.difficulty   = card->difficulty;
    dst.lastReview   = card->last_review;
    dst.totalReviews = card->total_answers;
    dst.due          = QString::fromStdString(dueText);

    switch (card->state) {
        case FSRS_STATE_NEW:        dst.state = QStringLiteral("New");        break;
        case FSRS_STATE_LEARNING:   dst.state = QStringLiteral("Learning");   break;
        case FSRS_STATE_RELEARNING: dst.state = QStringLiteral("Relearning"); break;
        case FSRS_STATE_REVIEW:     dst.state = QStringLiteral("Review");     break;
        case FSRS_STATE_SUSPENDED:  dst.state = QStringLiteral("Suspended");  break;
        default:                    dst.state = QStringLiteral("Unknown");    break;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Constructor / initialize
// ─────────────────────────────────────────────────────────────────────────────

SrsLibraryViewModel::SrsLibraryViewModel(QObject *parent)
    : QAbstractListModel(parent)
{
    m_debounceTimer.setSingleShot(true);
    connect(&m_debounceTimer, &QTimer::timeout,
            this, &SrsLibraryViewModel::onDebounceTimeout);
}

void SrsLibraryViewModel::initialize(
        SrsService    *service,
        kotoba_dict   *dict,
        SearchService *searchService,
        Configuration *config)
{
    m_service       = service;
    m_dict          = dict;
    m_searchService = searchService;
    m_config        = config;

    loadAllCards();
    rebuildFiltered();
    resetToInitialPage();
}

// ─────────────────────────────────────────────────────────────────────────────
// Debounce
// ─────────────────────────────────────────────────────────────────────────────

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

// ─────────────────────────────────────────────────────────────────────────────
// highlightField
// ─────────────────────────────────────────────────────────────────────────────

QString SrsLibraryViewModel::highlightField(const QString &field) const
{
    if (m_search.isEmpty() || field.isEmpty()) return field;

    const auto variants  = m_searchService->lastVariants();
    const QString fLower = field.toLower();
    int matchIdx = -1, matchLen = 0;

    for (const QString &v : { variants.normal, variants.romaji, variants.hiragana }) {
        if (v.isEmpty()) continue;
        const int idx = fLower.indexOf(v.toLower());
        if (idx >= 0 && (matchIdx < 0 || idx < matchIdx)) {
            matchIdx = idx;
            matchLen = v.length();
        }
    }

    if (matchIdx < 0) return field;

    const QString color = m_config ? accentToHex(m_config->accentColor)
                                   : QStringLiteral("#2196F3");
    QString result;
    result.reserve(field.size() + 48);
    result += field.left(matchIdx).toHtmlEscaped();
    result += QStringLiteral("<b style=\"color:") + color + QStringLiteral("\">");
    result += field.mid(matchIdx, matchLen).toHtmlEscaped();
    result += QStringLiteral("</b>");
    result += field.mid(matchIdx + matchLen).toHtmlEscaped();
    return result;
}

// ─────────────────────────────────────────────────────────────────────────────
// loadAllCards — itera ambos decks y agrupa por ent_seq
// ─────────────────────────────────────────────────────────────────────────────

static void populateWordAndReadings(SrsCardItem       &item,
                                     const entry_bin   *entry,
                                     const kotoba_dict *dict,
                                     SearchService     *searchService)
{
    // headword
    if (entry->k_elements_count > 0) {
        const k_ele_bin *k = kotoba_k_ele(dict, entry, 0);
        kotoba_str s = kotoba_keb(dict, k);
        item.word = QString::fromUtf8(s.ptr, s.len);
    } else if (entry->r_elements_count > 0) {
        const r_ele_bin *r = kotoba_r_ele(dict, entry, 0);
        kotoba_str s = kotoba_reb(dict, r);
        item.word = QString::fromUtf8(s.ptr, s.len);
    } else {
        item.word = QStringLiteral("[unknown]");
    }

    // variants UI: kanji k_ele[1..N]
    for (uint32_t k = 1; k < entry->k_elements_count; ++k) {
        const k_ele_bin *ke = kotoba_k_ele(dict, entry, k);
        kotoba_str s = kotoba_keb(dict, ke);
        item.variants << QString::fromUtf8(s.ptr, s.len);
    }

    // matchVariants: k_ele[0]
    if (entry->k_elements_count > 0) {
        const k_ele_bin *ke0 = kotoba_k_ele(dict, entry, 0);
        kotoba_str s0 = kotoba_keb(dict, ke0);
        item.matchVariants << QString::fromUtf8(s0.ptr, s0.len);
    }

    // readingsList + matchVariants hiragana
    char mixed_buf[MAX_QUERY_LEN];
    for (uint32_t r = 0; r < entry->r_elements_count; ++r) {
        const r_ele_bin *re = kotoba_r_ele(dict, entry, r);
        kotoba_str s = kotoba_reb(dict, re);
        item.readingsList << QString::fromUtf8(s.ptr, s.len);

        size_t copyLen = qMin<size_t>(s.len, MAX_QUERY_LEN - 1);
        memcpy(mixed_buf, s.ptr, copyLen);
        mixed_buf[copyLen] = '\0';
        mixed_to_hiragana(searchService->searchCtx()->trie_ctx,
                          mixed_buf, mixed_buf, MAX_QUERY_LEN);
        item.matchVariants << QString::fromUtf8(mixed_buf);
    }

    // meaning (primer sense)
    if (entry->senses_count > 0) {
        const sense_bin *sense = kotoba_sense(dict, entry, 0);
        QStringList glosses;
        for (uint32_t g = 0; g < sense->gloss_count; ++g) {
            kotoba_str gs = kotoba_gloss(dict, sense, g);
            glosses << QString::fromUtf8(gs.ptr, gs.len);
        }
        item.meaning = glosses.join(QStringLiteral("; "));
    }
}

void SrsLibraryViewModel::loadAllCards()
{
    m_allCards.clear();
    if (!m_service || !m_dict) return;

    // Mapa ent_seq → índice en m_allCards para agrupar los dos decks
    QHash<uint32_t, int> indexMap;

    auto processeDeck = [&](fsrs_deck *deck, CardType type) {
        if (!deck) return;
        uint32_t count = fsrs_deck_count(deck);
        for (uint32_t i = 0; i < count; ++i) {
            const fsrs_card *card = fsrs_deck_card(deck, i);
            if (!card) continue;

            uint32_t entSeq = card->id;
            const entry_bin *entry = kotoba_entry(m_dict, entSeq);
            if (!entry) continue;

            std::string dueText = m_service->dueDateText(entSeq, type);

            auto it = indexMap.find(entSeq);
            if (it == indexMap.end()) {
                // Nueva entrada
                SrsCardItem item;
                item.entSeq = entSeq;
                populateWordAndReadings(item, entry, m_dict, m_searchService);

                fillDeckState(
                    (type == CardType::Recall) ? item.recall : item.recog,
                    card, dueText);

                indexMap.insert(entSeq, m_allCards.size());
                m_allCards.append(std::move(item));
            } else {
                // Entrada ya existente — rellenar el otro deck
                SrsCardItem &item = m_allCards[it.value()];
                fillDeckState(
                    (type == CardType::Recall) ? item.recall : item.recog,
                    card, dueText);
            }
        }
    };

    processeDeck(m_service->recognitionDeck(), CardType::Recognition);
    processeDeck(m_service->recallDeck(),      CardType::Recall);
}

// ─────────────────────────────────────────────────────────────────────────────
// rebuildFiltered
// ─────────────────────────────────────────────────────────────────────────────

void SrsLibraryViewModel::rebuildFiltered()
{
    m_filtered.clear();

    const QString s  = m_search.trimmed();
    const bool hasSearch = !s.isEmpty();

    if (hasSearch) m_searchService->queryNonPagination(s);

    QSet<uint32_t> resultEntSeqs;
    if (hasSearch) {
        const SearchContext *ctx = m_searchService->searchCtx();
        for (uint32_t i = 0; i < ctx->results_left; ++i)
            resultEntSeqs.insert(ctx->results_buffer[i].p->doc_id);
    }

    for (const SrsCardItem &it : m_allCards) {
        bool matchSearch = !hasSearch
            || resultEntSeqs.contains(it.entSeq)
            || variantMatch(it, s);

        // Filtro de estado: se aplica si al menos uno de los dos decks cumple
        bool matchFilter = true;
        if (!m_filter.isEmpty() && m_filter != QLatin1String("All")) {
            bool recogMatch = false, recallMatch = false;
            auto stateMatch = [&](const QString &state) -> bool {
                if (m_filter == QLatin1String("New"))
                    return state == QLatin1String("New");
                if (m_filter == QLatin1String("Learning"))
                    return state == QLatin1String("Learning") ||
                           state == QLatin1String("Relearning");
                if (m_filter == QLatin1String("Review"))
                    return state == QLatin1String("Review");
                if (m_filter == QLatin1String("Suspended"))
                    return state == QLatin1String("Suspended");
                return false;
            };
            if (it.recog.present)  recogMatch  = stateMatch(it.recog.state);
            if (it.recall.present) recallMatch = stateMatch(it.recall.state);
            matchFilter = recogMatch || recallMatch;
        }

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

// ─────────────────────────────────────────────────────────────────────────────
// refresh async
// ─────────────────────────────────────────────────────────────────────────────

void SrsLibraryViewModel::refresh()
{
    if (m_refreshPending.exchange(true)) return;

    QThreadPool::globalInstance()->start([this]() {

        QVector<SrsCardItem> allCards;
        QHash<uint32_t, int> indexMap;

        auto processeDeck = [&](fsrs_deck *deck, CardType type) {
            if (!deck) return;
            std::lock_guard<std::mutex> lock(m_service->deckMutex());
            uint32_t count = fsrs_deck_count(deck);
            for (uint32_t i = 0; i < count; ++i) {
                const fsrs_card *card = fsrs_deck_card(deck, i);
                if (!card) continue;

                uint32_t entSeq = card->id;
                const entry_bin *entry = kotoba_entry(m_dict, entSeq);
                if (!entry) continue;

                std::string dueText = m_service->dueDateText(entSeq, type);

                auto it = indexMap.find(entSeq);
                if (it == indexMap.end()) {
                    SrsCardItem item;
                    item.entSeq = entSeq;
                    populateWordAndReadings(item, entry, m_dict, m_searchService);
                    fillDeckState(
                        (type == CardType::Recall) ? item.recall : item.recog,
                        card, dueText);
                    indexMap.insert(entSeq, allCards.size());
                    allCards.append(std::move(item));
                } else {
                    fillDeckState(
                        (type == CardType::Recall) ? allCards[it.value()].recall
                                                   : allCards[it.value()].recog,
                        card, dueText);
                }
            }
        };

        processeDeck(m_service->recognitionDeck(), CardType::Recognition);
        processeDeck(m_service->recallDeck(),      CardType::Recall);

        // Filtrado
        QVector<SrsCardItem> filtered;
        {
            const QString s  = m_search.trimmed();
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
                    || resultIds.contains(it.entSeq)
                    || variantMatch(it, s);

                bool matchFilter = true;
                if (!m_filter.isEmpty() && m_filter != QLatin1String("All")) {
                    auto stateMatch = [&](const QString &state) -> bool {
                        if (m_filter == QLatin1String("New"))       return state == QLatin1String("New");
                        if (m_filter == QLatin1String("Learning"))  return state == QLatin1String("Learning") || state == QLatin1String("Relearning");
                        if (m_filter == QLatin1String("Review"))    return state == QLatin1String("Review");
                        if (m_filter == QLatin1String("Suspended")) return state == QLatin1String("Suspended");
                        return false;
                    };
                    bool rm = it.recog.present  && stateMatch(it.recog.state);
                    bool rcm = it.recall.present && stateMatch(it.recall.state);
                    matchFilter = rm || rcm;
                }

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
    m_allCards       = std::move(allCards);
    m_filtered       = std::move(filtered);
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

// ─────────────────────────────────────────────────────────────────────────────
// Getters de detalle por ent_seq + cardType
// ─────────────────────────────────────────────────────────────────────────────

const SrsCardItem *findItem(const QVector<SrsCardItem> &cards, int entSeq)
{
    for (const auto &it : cards)
        if ((int)it.entSeq == entSeq) return &it;
    return nullptr;
}

int SrsLibraryViewModel::getReps(int entSeq, int cardType) const {
    const auto *it = findItem(m_allCards, entSeq);
    return it ? (int)stateFor(*it, cardType).reps : 0;
}
int SrsLibraryViewModel::getLapses(int entSeq, int cardType) const {
    const auto *it = findItem(m_allCards, entSeq);
    return it ? (int)stateFor(*it, cardType).lapses : 0;
}
int SrsLibraryViewModel::getTotalReviews(int entSeq, int cardType) const {
    const auto *it = findItem(m_allCards, entSeq);
    return it ? (int)stateFor(*it, cardType).totalReviews : 0;
}
float SrsLibraryViewModel::getStability(int entSeq, int cardType) const {
    const auto *it = findItem(m_allCards, entSeq);
    return it ? stateFor(*it, cardType).stability : 0.f;
}
float SrsLibraryViewModel::getDifficulty(int entSeq, int cardType) const {
    const auto *it = findItem(m_allCards, entSeq);
    return it ? stateFor(*it, cardType).difficulty : 0.f;
}
QString SrsLibraryViewModel::getLastReview(int entSeq, int cardType) const {
    const auto *it = findItem(m_allCards, entSeq);
    if (!it) return QStringLiteral("Never");
    uint64_t lr = stateFor(*it, cardType).lastReview;
    if (lr == 0) return QStringLiteral("Never");
    return QDateTime::fromSecsSinceEpoch((qint64)lr)
                     .toString(QStringLiteral("MMM d, yyyy"));
}
QString SrsLibraryViewModel::getDue(int entSeq, int cardType) const {
    return m_service
        ? QString::fromStdString(m_service->dueDateText((uint32_t)entSeq, cardTypeFromInt(cardType)))
        : QString{};
}
QString SrsLibraryViewModel::getState(int entSeq, int cardType) const {
    return m_service
        ? QString::fromStdString(m_service->stateText((uint32_t)entSeq, cardTypeFromInt(cardType)))
        : QString{};
}

// ─────────────────────────────────────────────────────────────────────────────
// Acciones
// ─────────────────────────────────────────────────────────────────────────────

void SrsLibraryViewModel::suspend(int entSeq, int cardType) {
    if (!m_service) return;
    m_service->suspend((uint32_t)entSeq, cardTypeFromInt(cardType));
    refresh();
}
void SrsLibraryViewModel::unsuspend(int entSeq, int cardType) {
    if (!m_service) return;
    m_service->unsuspend((uint32_t)entSeq, cardTypeFromInt(cardType));
    refresh();
}
void SrsLibraryViewModel::reset(int entSeq, int cardType) {
    if (!m_service) return;
    if (m_service->reset((uint32_t)entSeq, cardTypeFromInt(cardType)))
        refresh();
}
void SrsLibraryViewModel::remove(int entSeq, int cardType) {
    if (!m_service) return;
    if (m_service->remove((uint32_t)entSeq, cardTypeFromInt(cardType)))
        refresh();
}

// ─────────────────────────────────────────────────────────────────────────────
// Model interface
// ─────────────────────────────────────────────────────────────────────────────

int SrsLibraryViewModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent) return m_visible.size();
}

QVariant SrsLibraryViewModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_visible.size())
        return {};
    const SrsCardItem &c = m_visible.at(index.row());

    switch (role) {
        case EntryIdRole:      return (int)c.entSeq;
        case WordRole:         return c.word;
        case MeaningRole:      return c.meaning;
        case VariantsRole:     return c.variants.join(QStringLiteral("・"));
        case ReadingsRole:     return c.readingsList.join(QStringLiteral("・"));

        case RecogPresentRole: return c.recog.present;
        case RecogStateRole:   return c.recog.state;
        case RecogDueRole:     return c.recog.due;
        case RecogRepsRole:    return (int)c.recog.reps;
        case RecogLapsesRole:  return (int)c.recog.lapses;

        case RecallPresentRole: return c.recall.present;
        case RecallStateRole:   return c.recall.state;
        case RecallDueRole:     return c.recall.due;
        case RecallRepsRole:    return (int)c.recall.reps;
        case RecallLapsesRole:  return (int)c.recall.lapses;

        default: return {};
    }
}

QHash<int, QByteArray> SrsLibraryViewModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[EntryIdRole]       = "entryId";
    roles[WordRole]          = "word";
    roles[MeaningRole]       = "meaning";
    roles[VariantsRole]      = "variants";
    roles[ReadingsRole]      = "readings";
    roles[RecogPresentRole]  = "recogPresent";
    roles[RecogStateRole]    = "recogState";
    roles[RecogDueRole]      = "recogDue";
    roles[RecogRepsRole]     = "recogReps";
    roles[RecogLapsesRole]   = "recogLapses";
    roles[RecallPresentRole] = "recallPresent";
    roles[RecallStateRole]   = "recallState";
    roles[RecallDueRole]     = "recallDue";
    roles[RecallRepsRole]    = "recallReps";
    roles[RecallLapsesRole]  = "recallLapses";
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
    int n     = qMin(remaining, m_pageSize);
    int begin = m_visible.size();
    beginInsertRows(QModelIndex(), begin, begin + n - 1);
    for (int i = 0; i < n; ++i)
        m_visible.append(m_filtered.at(begin + i));
    endInsertRows();
}

bool SrsLibraryViewModel::variantMatch(const SrsCardItem &it, const QString &q) const
{
    const QString qL = q.toLower();
    for (const QString &v : it.matchVariants)
        if (v.toLower().contains(qL)) return true;
    return false;
}