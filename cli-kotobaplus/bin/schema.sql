-- Tabla principal: solo IDs y prioridad
CREATE TABLE IF NOT EXISTS entries (
    id INTEGER PRIMARY KEY,   -- ID igual al archivo binario
    priority INTEGER DEFAULT 0
);

-- Índice FTS5 para buscar en el diccionario
CREATE VIRTUAL TABLE IF NOT EXISTS entry_search USING fts5(
    id UNINDEXED,     -- referencia a la entrada binaria
    priority UNINDEXED,    -- usado para ordenar resultados
    kanji,
    reading,
    gloss,                 -- idioma primario del usuario
    tokenize = "unicode61"
);

-- Tabla SRS
CREATE TABLE IF NOT EXISTS srs_reviews (
    ent_seq INTEGER NOT NULL,
    last_review INTEGER NOT NULL,
    interval INTEGER NOT NULL,
    ease_factor REAL NOT NULL,   -- típicamente 2.5 inicial
    repetitions INTEGER NOT NULL,
    due_date INTEGER NOT NULL,

    PRIMARY KEY (ent_seq)
);

-- Tabla de estadísticas
CREATE TABLE IF NOT EXISTS stats (
    key TEXT PRIMARY KEY,
    value INTEGER
);
