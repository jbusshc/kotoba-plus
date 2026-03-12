#pragma once
#include <optional>
#include <string>
#include <cstdint>

extern "C" {
#include <fsrs.h>
#include <fsrs_sync.h>
}

class Configuration;

/*
 * SrsService — adaptador entre el GUI (ViewModel) y el core FSRS.
 * Provee operaciones: carga/guardado, sesión, next card, respuesta y estadísticas.
 */
class SrsService
{
public:
    explicit SrsService(uint32_t dictSize, Configuration *config);
    ~SrsService();

    bool load(const char *path);
    bool save(const char *path);

    bool add(uint32_t entryId);
    bool remove(uint32_t entryId);
    bool contains(uint32_t entryId) const;

    void startSession(uint32_t limit = 0);

    /* Obtener la siguiente tarjeta para la sesión actual (o nullptr). */
    fsrs_card* nextCard();

    /* Procesar respuesta (FSRS rating enum). */
    void answer(uint32_t entryId, fsrs_rating rating);

    /* Estadísticas simples */
    uint32_t dueCount() const;
    uint32_t learningCount() const;
    uint32_t newCount() const;
    uint32_t lapsedCount() const;

    /* Segundos hasta la próxima tarjeta si no hay disponible */
    uint64_t waitSeconds() const;

    /* Predecir intervalo (texto formateado) */
    std::string predictInterval(uint32_t entryId, fsrs_rating rating);

    fsrs_deck* getDeck() const { return m_deck; }
private:
    fsrs_card* getCard(uint32_t entryId) const;

    fsrs_deck *m_deck = nullptr;
    FsrsSync  m_sync;

    fsrs_session m_session;

    uint32_t m_dictSize = 0;
    Configuration *m_config = nullptr;

    std::string m_profilePath;
};