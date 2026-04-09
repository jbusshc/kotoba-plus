#pragma once
#include <cstdint>

// ─────────────────────────────────────────────────────────────────────────────
// CardType — diferencia cartas de Recognition vs Recall.
//
// Con la arquitectura de dos decks independientes cada deck usa ent_seq
// directamente como ID. No se necesita offset ni helpers de conversión.
// ─────────────────────────────────────────────────────────────────────────────

enum class CardType : uint8_t {
    Recognition = 0,   // headword/lectura → significado
    Recall      = 1,   // significado → headword/lectura
};