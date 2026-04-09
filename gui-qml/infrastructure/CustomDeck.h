#pragma once

#include <QString>
#include <QSet>
#include <QVector>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QFile>
#include <QDebug>
#include <cstdint>

// ─────────────────────────────────────────────────────────────────────────────
// CustomDeck — lista nombrada de ent_seq.
//
// No es un fsrs_deck real. Es un filtro sobre los dos decks base
// (Recognition y Recall). El scheduler los respeta limitando qué cartas
// sirve en una sesión de CustomDeck.
// ─────────────────────────────────────────────────────────────────────────────

struct CustomDeck {
    QString         id;          // UUID o slug único, persistido
    QString         name;        // nombre visible al usuario
    QSet<uint32_t>  entSeqs;     // ent_seq incluidos en este mazo
};

// ─────────────────────────────────────────────────────────────────────────────
// CustomDeckRegistry — carga, guarda y gestiona la colección de CustomDecks.
// ─────────────────────────────────────────────────────────────────────────────

class CustomDeckRegistry
{
public:
    // Carga desde JSON; devuelve true si el archivo existía y era válido.
    bool load(const QString &path)
    {
        m_path = path;
        m_decks.clear();

        QFile f(path);
        if (!f.exists()) return false;
        if (!f.open(QIODevice::ReadOnly)) {
            qWarning() << "CustomDeckRegistry: cannot open" << path;
            return false;
        }

        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &err);
        if (err.error != QJsonParseError::NoError) {
            qWarning() << "CustomDeckRegistry: JSON parse error:" << err.errorString();
            return false;
        }

        for (const QJsonValue &v : doc.array()) {
            QJsonObject obj = v.toObject();
            CustomDeck deck;
            deck.id   = obj["id"].toString();
            deck.name = obj["name"].toString();
            for (const QJsonValue &seq : obj["entSeqs"].toArray())
                deck.entSeqs.insert(static_cast<uint32_t>(seq.toInt()));
            if (!deck.id.isEmpty())
                m_decks.append(std::move(deck));
        }
        return true;
    }

    bool save() const
    {
        if (m_path.isEmpty()) return false;

        QJsonArray arr;
        for (const CustomDeck &deck : m_decks) {
            QJsonObject obj;
            obj["id"]   = deck.id;
            obj["name"] = deck.name;
            QJsonArray seqs;
            for (uint32_t seq : deck.entSeqs)
                seqs.append(static_cast<int>(seq));
            obj["entSeqs"] = seqs;
            arr.append(obj);
        }

        QFile f(m_path);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            qWarning() << "CustomDeckRegistry: cannot write" << m_path;
            return false;
        }
        f.write(QJsonDocument(arr).toJson(QJsonDocument::Compact));
        return true;
    }

    // ── CRUD ──────────────────────────────────────────────────────────────

    // Añade un nuevo mazo; id debe ser único.
    void add(CustomDeck deck)
    {
        m_decks.append(std::move(deck));
    }

    // Devuelve nullptr si no existe.
    CustomDeck *find(const QString &id)
    {
        for (auto &d : m_decks)
            if (d.id == id) return &d;
        return nullptr;
    }

    const CustomDeck *find(const QString &id) const
    {
        for (const auto &d : m_decks)
            if (d.id == id) return &d;
        return nullptr;
    }

    bool remove(const QString &id)
    {
        for (int i = 0; i < m_decks.size(); ++i) {
            if (m_decks[i].id == id) {
                m_decks.removeAt(i);
                return true;
            }
        }
        return false;
    }

    const QVector<CustomDeck> &decks() const { return m_decks; }

private:
    QVector<CustomDeck> m_decks;
    QString             m_path;
};