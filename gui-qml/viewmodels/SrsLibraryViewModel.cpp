#include "SrsLibraryViewModel.h"
#include "../infrastructure/SrsService.h"
#include "../infrastructure/SearchService.h"
#include "../app/Configuration.h"
#include <QDebug>
#include <QDateTime>

extern "C" {
#include "../../core/include/viewer.h"
#include "../../core/include/index_search.h"
}

// Misma tabla que SearchViewModel::accentToHex — hex CSS para inline style de <b>
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

void SrsLibraryViewModel::initialize(
        SrsService*    service,
        kotoba_dict*   dict,
        SearchService* searchService,
        Configuration* config
    )
{
    m_service = service;
    m_dict = dict;
    m_searchService = searchService;
    m_config = config;


    loadAllCards();
    rebuildFiltered();
    resetToInitialPage();
}

SrsLibraryViewModel::SrsLibraryViewModel(QObject* parent)
    : QAbstractListModel(parent)
    , m_service(nullptr), m_dict(nullptr)
    , m_searchService(nullptr), m_config(nullptr)
{
    m_debounceTimer.setSingleShot(true);
    connect(&m_debounceTimer, &QTimer::timeout,
            this, &SrsLibraryViewModel::onDebounceTimeout);
}

// ── Debounce ─────────────────────────────────────────────────────────────────

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
    rebuildFiltered();   // llama queryNonPagination → llena ctx buffers
    resetToInitialPage();
    if (m_search != prev)
        emit activeSearchChanged();
}

// ── highlightField ────────────────────────────────────────────────────────────
// Mismo algoritmo que SearchViewModel::highlightField.
// Válido solo después de rebuildFiltered() (que llama queryNonPagination).

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

        const entry_bin* entry = kotoba_dict_get_entry(m_dict, card->id);
        if (!entry) continue;

        SrsCardItem item;
        item.id           = card->id;
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

        // matchVariants: k_ele[0] + hiragana de cada r_ele (para variantMatch)
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

        item.due = QString::fromStdString(m_service->dueDateText(card->id));
        m_allCards.append(item);
    }
}

// ── rebuildFiltered ───────────────────────────────────────────────────────────

void SrsLibraryViewModel::rebuildFiltered()
{
    m_filtered.clear();

    const QString s = m_search.trimmed();
    const bool hasSearch = !s.isEmpty();

    // queryNonPagination llena ctx buffers → lastVariants() válido para highlightField()
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
        bool matchSearch = !hasSearch
            || resultEntryIds.contains(it.id)
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
    loadAllCards();
    rebuildFiltered();
    resetToInitialPage();
}

void SrsLibraryViewModel::setFilter(const QString &filter)
{
    m_filter = filter;
    loadAllCards();
    rebuildFiltered();
    resetToInitialPage();
}

// ── Getters ───────────────────────────────────────────────────────────────────

int SrsLibraryViewModel::getReps(int entryId) const {
    for (const auto &it : m_allCards) if ((int)it.id == entryId) return it.reps; return 0; }
int SrsLibraryViewModel::getLapses(int entryId) const {
    for (const auto &it : m_allCards) if ((int)it.id == entryId) return it.lapses; return 0; }
int SrsLibraryViewModel::getTotalReviews(int entryId) const {
    for (const auto &it : m_allCards) if ((int)it.id == entryId) return (int)it.totalReviews; return 0; }
float SrsLibraryViewModel::getStability(int entryId) const {
    for (const auto &it : m_allCards) if ((int)it.id == entryId) return it.stability; return 0.f; }
float SrsLibraryViewModel::getDifficulty(int entryId) const {
    for (const auto &it : m_allCards) if ((int)it.id == entryId) return it.difficulty; return 0.f; }

QString SrsLibraryViewModel::getLastReview(int entryId) const {
    for (const auto &it : m_allCards) {
        if ((int)it.id != entryId) continue;
        if (it.lastReview == 0) return QStringLiteral("Never");
        return QDateTime::fromSecsSinceEpoch((qint64)it.lastReview)
                         .toString(QStringLiteral("MMM d, yyyy"));
    }
    return QStringLiteral("Never");
}
QString SrsLibraryViewModel::getDue(int entryId) const {
    return m_service ? QString::fromStdString(m_service->dueDateText((uint32_t)entryId)) : QString{}; }
QString SrsLibraryViewModel::getState(int entryId) const {
    return m_service ? QString::fromStdString(m_service->stateText((uint32_t)entryId)) : QString{}; }

// ── Acciones ─────────────────────────────────────────────────────────────────

void SrsLibraryViewModel::suspend(int entryId)   { if (m_service&&entryId>=0){m_service->suspend((uint32_t)entryId);refresh();} }
void SrsLibraryViewModel::unsuspend(int entryId) { if (m_service&&entryId>=0){m_service->unsuspend((uint32_t)entryId);refresh();} }
void SrsLibraryViewModel::reset(int entryId)     { if (m_service&&entryId>=0&&m_service->reset((uint32_t)entryId))refresh(); }
void SrsLibraryViewModel::remove(int entryId)    { if (m_service&&entryId>=0&&m_service->remove((uint32_t)entryId))refresh(); }

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
        case EntryIdRole:  return (int)c.id;
        case WordRole:     return c.word;
        case MeaningRole:  return c.meaning;
        case StateRole:    return c.state;
        case DueRole:      return c.due;
        case RepsRole:     return (int)c.reps;
        case LapsesRole:   return (int)c.lapses;
        case VariantsRole: return c.variants.join(QStringLiteral("・"));
        case ReadingsRole: return c.readingsList.join(QStringLiteral("・"));
        default:           return {};
    }
}

QHash<int, QByteArray> SrsLibraryViewModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[EntryIdRole]  = "entryId";
    roles[WordRole]     = "word";
    roles[MeaningRole]  = "meaning";
    roles[StateRole]    = "state";
    roles[DueRole]      = "due";
    roles[RepsRole]     = "reps";
    roles[LapsesRole]   = "lapses";
    roles[VariantsRole] = "variants";
    roles[ReadingsRole] = "readings";
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