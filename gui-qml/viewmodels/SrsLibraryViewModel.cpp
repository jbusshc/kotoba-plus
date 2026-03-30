#include "SrsLibraryViewModel.h"
#include "../infrastructure/SrsService.h"
#include "../infrastructure/SearchService.h"
#include <QDebug>
#include <QDateTime>

extern "C" {
#include "../../core/include/viewer.h"
#include "../../core/include/index_search.h"
}

SrsLibraryViewModel::SrsLibraryViewModel(SrsService* service, kotoba_dict* dict,
                                         SearchService* searchService, QObject* parent)
    : QAbstractListModel(parent), m_service(service), m_dict(dict), m_searchService(searchService)
{
    m_debounceTimer.setSingleShot(true);
    connect(&m_debounceTimer, &QTimer::timeout,
            this, &SrsLibraryViewModel::onDebounceTimeout);

    loadAllCards();
    rebuildFiltered();
    resetToInitialPage();
}

// ── Debounce ─────────────────────────────────────────────────────────────────

void SrsLibraryViewModel::setSearch(const QString &text)
{
    m_pendingSearch = text;
    m_debounceTimer.start(120);
}

void SrsLibraryViewModel::onDebounceTimeout()
{
    m_search = m_pendingSearch;
    loadAllCards();
    rebuildFiltered();
    resetToInitialPage();
}

// ── Helpers privados ─────────────────────────────────────────────────────────

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

        // ── headword: primer kanji, o primer kana si no hay kanji ────────────
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

        // ── variants: formas kanji k_ele[1..N] (excluye el headword) ─────────
        for (uint32_t k = 1; k < entry->k_elements_count; ++k) {
            const k_ele_bin* ke = kotoba_k_ele(m_dict, entry, k);
            kotoba_str s = kotoba_keb(m_dict, ke);
            item.variants.append(QString::fromUtf8(s.ptr, s.len));
        }

        // ── variants internas para variantMatch (kanji[0] + hiragana) ────────
        // Se llenan aparte para que el match funcione aunque variants esté vacío.
        char mixed_buf[MAX_QUERY_LEN];
        if (entry->k_elements_count > 0) {
            const k_ele_bin* ke0 = kotoba_k_ele(m_dict, entry, 0);
            kotoba_str s0 = kotoba_keb(m_dict, ke0);
            item.matchVariants.append(QString::fromUtf8(s0.ptr, s0.len));
        }
        for (uint32_t r = 0; r < entry->r_elements_count; ++r) {
            const r_ele_bin* re = kotoba_r_ele(m_dict, entry, r);
            kotoba_str s = kotoba_reb(m_dict, re);

            // ── readingsList: lecturas originales para mostrar en UI ──────────
            item.readingsList << QString::fromUtf8(s.ptr, s.len);

            // hiragana para matchVariants
            size_t copyLen = qMin<size_t>(s.len, MAX_QUERY_LEN - 1);
            memcpy(mixed_buf, s.ptr, copyLen);
            mixed_buf[copyLen] = '\0';
            mixed_to_hiragana(m_searchService->searchCtx()->trie_ctx,
                              mixed_buf, mixed_buf, MAX_QUERY_LEN);
            item.matchVariants.append(QString::fromUtf8(mixed_buf));
        }

        uint32_t firstSenseId = 0;
        // ── meaning: primer sentido ───────────────────────────────────────────
        if (entry->senses_count > 0) {
            for (uint32_t s = 0; s < entry->senses_count; ++s) {
                const sense_bin* sense = kotoba_sense(m_dict, entry, s);
                if (sense->lang < 0 || sense->lang >= 32 || m_searchService->searchCtx()->is_gloss_active[sense->lang] == 0)
                    continue;
                firstSenseId = s;
                break;
            }
            if (firstSenseId >= entry->senses_count) goto noGloss;
            const sense_bin* sense = kotoba_sense(m_dict, entry, firstSenseId);
            QStringList glosses;
            for (uint32_t g = 0; g < sense->gloss_count; ++g) {
                kotoba_str gs = kotoba_gloss(m_dict, sense, g);
                glosses << QString::fromUtf8(gs.ptr, gs.len);
            }
            item.meaning = glosses.join(QStringLiteral("; "));
        }
        noGloss:

        item.due = QString::fromStdString(m_service->dueDateText(card->id));
        m_allCards.append(item);
    }
}

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

int SrsLibraryViewModel::getReps(int entryId) const
{
    for (const SrsCardItem &it : m_allCards)
        if (static_cast<int>(it.id) == entryId) return it.reps;
    return 0;
}

int SrsLibraryViewModel::getLapses(int entryId) const
{
    for (const SrsCardItem &it : m_allCards)
        if (static_cast<int>(it.id) == entryId) return it.lapses;
    return 0;
}

int SrsLibraryViewModel::getTotalReviews(int entryId) const
{
    for (const SrsCardItem &it : m_allCards)
        if (static_cast<int>(it.id) == entryId) return static_cast<int>(it.totalReviews);
    return 0;
}

float SrsLibraryViewModel::getStability(int entryId) const
{
    for (const SrsCardItem &it : m_allCards)
        if (static_cast<int>(it.id) == entryId) return it.stability;
    return 0.f;
}

float SrsLibraryViewModel::getDifficulty(int entryId) const
{
    for (const SrsCardItem &it : m_allCards)
        if (static_cast<int>(it.id) == entryId) return it.difficulty;
    return 0.f;
}

QString SrsLibraryViewModel::getLastReview(int entryId) const
{
    for (const SrsCardItem &it : m_allCards) {
        if (static_cast<int>(it.id) != entryId) continue;
        if (it.lastReview == 0) return QStringLiteral("Never");
        QDateTime dt = QDateTime::fromSecsSinceEpoch(static_cast<qint64>(it.lastReview));
        return dt.toString(QStringLiteral("MMM d, yyyy"));
    }
    return QStringLiteral("Never");
}

QString SrsLibraryViewModel::getDue(int entryId) const
{
    if (!m_service) return {};
    return QString::fromStdString(m_service->dueDateText(static_cast<uint32_t>(entryId)));
}

QString SrsLibraryViewModel::getState(int entryId) const
{
    if (!m_service) return {};
    return QString::fromStdString(m_service->stateText(static_cast<uint32_t>(entryId)));
}

// ── Acciones ─────────────────────────────────────────────────────────────────

void SrsLibraryViewModel::suspend(int entryId)
{
    if (!m_service || entryId < 0) return;
    m_service->suspend(static_cast<uint32_t>(entryId));
    refresh();
}

void SrsLibraryViewModel::unsuspend(int entryId)
{
    if (!m_service || entryId < 0) return;
    m_service->unsuspend(static_cast<uint32_t>(entryId));
    refresh();
}

void SrsLibraryViewModel::reset(int entryId)
{
    if (!m_service || entryId < 0) return;
    if (!m_service->reset(static_cast<uint32_t>(entryId))) {
        qWarning() << "SrsLibraryViewModel::reset failed for" << entryId;
        return;
    }
    refresh();
}

void SrsLibraryViewModel::remove(int entryId)
{
    if (!m_service || entryId < 0) return;
    if (!m_service->remove(static_cast<uint32_t>(entryId))) {
        qWarning() << "SrsLibraryViewModel::remove failed for" << entryId;
        return;
    }
    refresh();
}

// ── Model interface ───────────────────────────────────────────────────────────

int SrsLibraryViewModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_visible.size();
}

QVariant SrsLibraryViewModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_visible.size())
        return {};

    const SrsCardItem &c = m_visible.at(index.row());
    switch (role) {
        case EntryIdRole:  return static_cast<int>(c.id);
        case WordRole:     return c.word;
        case MeaningRole:  return c.meaning;
        case StateRole:    return c.state;
        case DueRole:      return c.due;
        case RepsRole:     return static_cast<int>(c.reps);
        case LapsesRole:   return static_cast<int>(c.lapses);
        // variants: kanji alternativos k_ele[1..N], para mostrar en UI
        case VariantsRole: return c.variants.join(QStringLiteral("・"));
        // readings: lecturas r_ele originales, para mostrar en UI
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
    Q_UNUSED(parent)
    return m_visible.size() < m_filtered.size();
}

void SrsLibraryViewModel::fetchMore(const QModelIndex &parent)
{
    Q_UNUSED(parent)
    int remaining = m_filtered.size() - m_visible.size();
    if (remaining <= 0) return;

    int itemsToFetch = qMin(remaining, m_pageSize);
    int begin = m_visible.size();

    beginInsertRows(QModelIndex(), begin, begin + itemsToFetch - 1);
    for (int i = 0; i < itemsToFetch; ++i)
        m_visible.append(m_filtered.at(begin + i));
    endInsertRows();
}

// variantMatch: usa matchVariants (kanji[0] + hiragana de cada reading)
// para cubrir casos que el índice full-text no alcanza.
bool SrsLibraryViewModel::variantMatch(const SrsCardItem &it, const QString &q) const
{
    const QString qLower = q.toLower();
    for (const QString &v : it.matchVariants)
        if (v.toLower().contains(qLower))
            return true;
    return false;
}