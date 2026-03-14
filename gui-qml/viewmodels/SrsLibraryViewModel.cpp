#include "SrsLibraryViewModel.h"
#include "../infrastructure/SrsService.h"
#include <QDebug>

extern "C" {
#include "../../core/include/viewer.h"
}

SrsLibraryViewModel::SrsLibraryViewModel(SrsService* service, kotoba_dict* dict, QObject* parent)
    : QAbstractListModel(parent), m_service(service), m_dict(dict)
{
    loadAllCards();
    rebuildFiltered();
    resetToInitialPage();
}

/* ---- helpers privados ---- */

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
        item.id     = card->id;
        item.reps   = card->reps;
        item.lapses = card->lapses;

        switch (card->state) {
            case FSRS_STATE_NEW:        item.state = QStringLiteral("New");        break;
            case FSRS_STATE_LEARNING:   item.state = QStringLiteral("Learning");   break;
            case FSRS_STATE_RELEARNING: item.state = QStringLiteral("Relearning"); break;
            case FSRS_STATE_REVIEW:     item.state = QStringLiteral("Review");     break;
            case FSRS_STATE_SUSPENDED:  item.state = QStringLiteral("Suspended");  break;
            default:                    item.state = QStringLiteral("Unknown");    break;
        }

        /* word */
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

        /* meaning */
        if (entry->senses_count > 0) {
            const sense_bin* sense = kotoba_sense(m_dict, entry, 0);
            QStringList glosses;
            for (uint32_t g = 0; g < sense->gloss_count; ++g) {
                kotoba_str gs = kotoba_gloss(m_dict, sense, g);
                glosses << QString::fromUtf8(gs.ptr, gs.len);
            }
            item.meaning = glosses.join(QStringLiteral("; "));
        }

        /*
         * BUG FIX: "due" ahora muestra la fecha de vencimiento REAL de la
         * carta (cuánto falta hasta card->due_ts), no el intervalo que
         * obtendría si respondiera FSRS_GOOD en este momento.
         *
         * predictInterval(FSRS_GOOD) era incorrecto para la librería: mostraba
         * "¿cuánto tiempo ganaría si respondiera bien?" en vez de "¿cuándo toca
         * repasar esta carta?".
         */
        item.due = QString::fromStdString(m_service->dueDateText(card->id));

        m_allCards.append(item);
    }
}

void SrsLibraryViewModel::rebuildFiltered()
{
    m_filtered.clear();

    for (const SrsCardItem &it : m_allCards) {
        bool matchSearch = true;
        if (!m_search.trimmed().isEmpty()) {
            const QString s = m_search.trimmed();
            matchSearch = it.word.contains(s, Qt::CaseInsensitive) ||
                          it.meaning.contains(s, Qt::CaseInsensitive) ||
                          QString::number(it.id) == s;
        }

        bool matchFilter = true;
        if (m_filter == QLatin1String("New"))
            matchFilter = (it.state == QLatin1String("New"));
        else if (m_filter == QLatin1String("Learning"))
            matchFilter = (it.state == QLatin1String("Learning") || it.state == QLatin1String("Relearning"));
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

/* ---- API pública ---- */

void SrsLibraryViewModel::refresh()
{
    loadAllCards();
    rebuildFiltered();
    resetToInitialPage();
}

void SrsLibraryViewModel::setSearch(const QString &text)
{
    // No early-return por igualdad: el deck puede haber cambiado
    // desde la última vez que se cargaron las cartas.
    m_search = text;
    loadAllCards();       // siempre refrescar desde el deck
    rebuildFiltered();
    resetToInitialPage();
}

void SrsLibraryViewModel::setFilter(const QString &filter)
{
    m_filter = filter;
    loadAllCards();       // idem
    rebuildFiltered();
    resetToInitialPage();
}

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

/*
 * getDue() — BUG FIX.
 * Devuelve el tiempo real hasta el vencimiento (dueDateText),
 * no el intervalo hipotético de FSRS_GOOD.
 */
QString SrsLibraryViewModel::getDue(int entryId) const
{
    if (!m_service) return {};
    return QString::fromStdString(m_service->dueDateText(static_cast<uint32_t>(entryId)));
}

void SrsLibraryViewModel::suspend(int entryId)
{
    if (!m_service || entryId < 0) return;
    m_service->suspend(static_cast<uint32_t>(entryId));
    refresh();
}

/*
 * reset() — BUG FIX.
 *
 * Antes manipulaba el fsrs_card* directamente sin pasar por el sistema de
 * sync, perdiendo el cambio en el siguiente load(). Ahora delega a
 * SrsService::reset() que hace remove+add con eventos de sync.
 */
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

/* ---- model interface ---- */

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
        case EntryIdRole: return static_cast<int>(c.id);
        case WordRole:    return c.word;
        case MeaningRole: return c.meaning;
        case StateRole:   return c.state;
        case DueRole:     return c.due;
        case RepsRole:    return static_cast<int>(c.reps);
        case LapsesRole:  return static_cast<int>(c.lapses);
        default:          return {};
    }
}

QHash<int, QByteArray> SrsLibraryViewModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[EntryIdRole] = "entryId";
    roles[WordRole]    = "word";
    roles[MeaningRole] = "meaning";
    roles[StateRole]   = "state";
    roles[DueRole]     = "due";
    roles[RepsRole]    = "reps";
    roles[LapsesRole]  = "lapses";
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