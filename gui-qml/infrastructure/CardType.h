#pragma once
#include <cstdint>

// ─────────────────────────────────────────────────────────────────────────────
// CardType — diferencia cartas de Recognition vs Recall.
//
// El core FSRS usa id_map y bitmap como arrays directos indexados por ID.
// Usar bit 31 genera IDs de ~2^31 → realloc de 16 GB → crash en Android.
//
// Solución: RECALL_OFFSET fijo.
//   Recognition  →  fsrsId = entryId
//   Recall       →  fsrsId = entryId + RECALL_OFFSET
//
// RECALL_OFFSET debe ser > max entryId del diccionario.
// JMdict tiene ~220 K entries. Usamos 500 000 para tener margen.
// El deck necesitará id_map de ~720 K entradas = ~3 MB. Aceptable.
//
// Para cambiar el offset en el futuro (p.ej. si el dict crece):
//   1. Cambiar RECALL_OFFSET aquí
//   2. Migrar el .srs existente (los fsrsIds guardados cambiarían)
//   Se recomienda elegir un valor conservador desde el inicio.
// ─────────────────────────────────────────────────────────────────────────────

enum class CardType : uint8_t {
    Recognition = 0,   // 日 → significado / lectura
    Recall      = 1,   // significado → 日
};

namespace CardTypeHelper {

// Debe ser mayor que el número máximo de entries del diccionario.
// JMdict actual: ~220 K. Usamos 500 K para tener margen holgado.
static constexpr uint32_t RECALL_OFFSET = 262'144u;

// entryId + tipo → id único para el deck FSRS
inline uint32_t toFsrsId(uint32_t entryId, CardType type) noexcept {
    return (type == CardType::Recall) ? (entryId + RECALL_OFFSET) : entryId;
}

// fsrsId → entryId original del diccionario
inline uint32_t toEntryId(uint32_t fsrsId) noexcept {
    return (fsrsId >= RECALL_OFFSET) ? (fsrsId - RECALL_OFFSET) : fsrsId;
}

// fsrsId → tipo de carta
inline CardType toCardType(uint32_t fsrsId) noexcept {
    return (fsrsId >= 2) ? CardType::Recall : CardType::Recognition;
}

// fsrsId → string legible para UI / logs
inline const char* typeName(uint32_t fsrsId) noexcept {
    return (fsrsId >= RECALL_OFFSET) ? "Recall" : "Recognition";
}

} // namespace CardTypeHelper