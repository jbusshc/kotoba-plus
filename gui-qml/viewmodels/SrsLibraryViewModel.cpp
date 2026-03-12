#include "SrsLibraryViewModel.h"
#include "../infrastructure/SrsService.h"
extern "C" {
#include "../../core/include/loader.h"
#include "../../core/include/fsrs.h"
#include "../../core/include/viewer.h"
}

SrsLibraryViewModel::SrsLibraryViewModel(SrsService* service, kotoba_dict* dict, QObject* parent)
    : QAbstractListModel(parent), m_service(service), m_dict(dict)
{
    reloadCards();
}

void SrsLibraryViewModel::reloadCards()
{
    beginResetModel();
    m_cards.clear();

    if (!m_service || !m_dict) {
        endResetModel();
        return;
    }

    uint32_t count = fsrs_deck_count(m_service->getDeck());
    for (uint32_t i = 0; i < count; ++i) {
        const fsrs_card* card = fsrs_deck_card(m_service->getDeck(), i);
        if (!card) continue;

        const entry_bin* entry = kotoba_dict_get_entry(m_dict, card->id);
        if (!entry) continue;

        QString word;
        if (entry->k_elements_count > 0) {
            const k_ele_bin* k = kotoba_k_ele(m_dict, entry, 0);
            kotoba_str s = kotoba_keb(m_dict, k);
            word = QString::fromUtf8(s.ptr, s.len);
        } else if (entry->r_elements_count > 0) {
            const r_ele_bin* r = kotoba_r_ele(m_dict, entry, 0);
            kotoba_str s = kotoba_reb(m_dict, r);
            word = QString::fromUtf8(s.ptr, s.len);
        } else {
            word = "[unknown]";
        }

        QString meaning;
        if (entry->senses_count > 0) {
            const sense_bin* sense = kotoba_sense(m_dict, entry, 0);
            QStringList glosses;
            for (uint32_t g = 0; g < sense->gloss_count; ++g) {
                kotoba_str gs = kotoba_gloss(m_dict, sense, g);
                glosses << QString::fromUtf8(gs.ptr, gs.len);
            }
            meaning = glosses.join("; ");
        }

        QString stateStr;
        switch(card->state) {
            case FSRS_STATE_NEW: stateStr = "New"; break;
            case FSRS_STATE_LEARNING: stateStr = "Learning"; break;
            case FSRS_STATE_RELEARNING: stateStr = "Relearning"; break;
            case FSRS_STATE_REVIEW: stateStr = "Review"; break;
            case FSRS_STATE_SUSPENDED: stateStr = "Suspended"; break;
            default: stateStr = "Unknown"; break;
        }

        QString intervalStr = QString::fromStdString(m_service->predictInterval(card->id, FSRS_GOOD));

        SrsCardItem item;
        item.id = card->id;
        item.word = word;
        item.meaning = meaning;
        item.state = stateStr;
        item.interval = intervalStr;
        item.reps = card->reps;
        item.lapses = card->lapses;

        m_cards.append(item);
    }

    endResetModel();
}

// QAbstractListModel overrides
int SrsLibraryViewModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_cards.size();
}

QVariant SrsLibraryViewModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_cards.size()) return QVariant();
    const SrsCardItem& c = m_cards.at(index.row());

    switch (role) {
        case IdRole: return c.id;
        case WordRole: return c.word;
        case MeaningRole: return c.meaning;
        case StateRole: return c.state;
        case IntervalRole: return c.interval;
        case RepsRole: return c.reps;
        case LapsesRole: return c.lapses;
        default: return QVariant();
    }
}

QHash<int, QByteArray> SrsLibraryViewModel::roleNames() const
{
    return {
        { IdRole, "id" },
        { WordRole, "word" },
        { MeaningRole, "meaning" },
        { StateRole, "state" },
        { IntervalRole, "interval" },
        { RepsRole, "reps" },
        { LapsesRole, "lapses" }
    };
}