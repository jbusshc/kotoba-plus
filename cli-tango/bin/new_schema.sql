-- Crear tabla principal entries con columnas JSON para kanjis, lecturas y sentidos
CREATE TABLE entries (
    id INTEGER PRIMARY KEY,
    priority INTEGER DEFAULT 9999,
    entry_json TEXT NOT NULL  -- JSON con { "k_ele": [...], "r_ele": [...], "sense": [...] }
);


CREATE VIRTUAL TABLE entry_search USING fts5(
    entry_id UNINDEXED,
    priority UNINDEXED,  -- usada solo para ordenamiento en consultas
    kanji,
    reading,
    gloss,
    tokenize = "unicode61"
);

CREATE TABLE IF NOT EXISTS srs_reviews (
    entry_id INTEGER NOT NULL,
    last_review INTEGER NOT NULL,   -- Unix timestamp en segundos
    interval INTEGER NOT NULL,      -- días para la próxima revisión
    ease_factor REAL NOT NULL,      -- factor de facilidad, ejemplo 2.5 inicial
    repetitions INTEGER NOT NULL,   -- cuántas veces se repasó
    due_date INTEGER NOT NULL,      -- Unix timestamp para la siguiente revisión

    PRIMARY KEY (entry_id)
);

CREATE TABLE stats (
    key TEXT PRIMARY KEY,
    value INTEGER
);

