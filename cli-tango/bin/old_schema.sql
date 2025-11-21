CREATE TABLE entries (
    id INTEGER PRIMARY KEY -- <ent_seq>
, priority INTEGER DEFAULT 9999);
CREATE TABLE kanjis (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    entry_id INTEGER NOT NULL,
    keb TEXT NOT NULL,
    FOREIGN KEY (entry_id) REFERENCES entries(id)
);
CREATE TABLE sqlite_sequence(name,seq);
CREATE TABLE kanji_info (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    kanji_id INTEGER NOT NULL,
    info TEXT,
    priority TEXT,
    FOREIGN KEY (kanji_id) REFERENCES kanjis(id)
);
CREATE TABLE readings (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    entry_id INTEGER NOT NULL,
    reb TEXT NOT NULL,
    no_kanji BOOLEAN DEFAULT 0,
    FOREIGN KEY (entry_id) REFERENCES entries(id)
);
CREATE TABLE reading_restrictions (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    reading_id INTEGER NOT NULL,
    kanji TEXT NOT NULL,
    FOREIGN KEY (reading_id) REFERENCES readings(id)
);
CREATE TABLE reading_info (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    reading_id INTEGER NOT NULL,
    info TEXT,
    priority TEXT,
    FOREIGN KEY (reading_id) REFERENCES readings(id)
);
CREATE TABLE senses (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    entry_id INTEGER NOT NULL,
    FOREIGN KEY (entry_id) REFERENCES entries(id)
);
CREATE TABLE pos (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    sense_id INTEGER NOT NULL,
    tag TEXT NOT NULL,
    FOREIGN KEY (sense_id) REFERENCES senses(id)
);
CREATE TABLE glosses (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    sense_id INTEGER NOT NULL,
    gloss TEXT NOT NULL,
    lang TEXT DEFAULT 'eng',
    g_type TEXT,
    g_gend TEXT,
    FOREIGN KEY (sense_id) REFERENCES senses(id)
);
CREATE TABLE fields (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    sense_id INTEGER NOT NULL,
    field TEXT NOT NULL,
    FOREIGN KEY (sense_id) REFERENCES senses(id)
);
CREATE TABLE misc (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    sense_id INTEGER NOT NULL,
    tag TEXT NOT NULL,
    FOREIGN KEY (sense_id) REFERENCES senses(id)
);
CREATE TABLE xrefs (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    sense_id INTEGER NOT NULL,
    target TEXT NOT NULL,
    FOREIGN KEY (sense_id) REFERENCES senses(id)
);
CREATE TABLE antonyms (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    sense_id INTEGER NOT NULL,
    target TEXT NOT NULL,
    FOREIGN KEY (sense_id) REFERENCES senses(id)
);
CREATE TABLE lsource (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    sense_id INTEGER NOT NULL,
    source TEXT,
    lang TEXT DEFAULT 'eng',
    ls_type TEXT,
    ls_wasei BOOLEAN,
    FOREIGN KEY (sense_id) REFERENCES senses(id)
);
CREATE TABLE dialects (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    sense_id INTEGER NOT NULL,
    dialect TEXT NOT NULL,
    FOREIGN KEY (sense_id) REFERENCES senses(id)
);
CREATE TABLE sense_info (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    sense_id INTEGER NOT NULL,
    info TEXT NOT NULL,
    FOREIGN KEY (sense_id) REFERENCES senses(id)
);
CREATE TABLE sense_restrictions (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    sense_id INTEGER NOT NULL,
    keb TEXT,
    reb TEXT,
    FOREIGN KEY (sense_id) REFERENCES senses(id)
);
CREATE TABLE entry_priority (
    entry_id INTEGER PRIMARY KEY,
    priority INTEGER NOT NULL
);
CREATE INDEX idx_kanjis_entry_id ON kanjis(entry_id);
CREATE INDEX idx_readings_entry_id ON readings(entry_id);
CREATE INDEX idx_glosses_sense_id ON glosses(sense_id);
CREATE INDEX idx_senses_entry_id ON senses(entry_id);
CREATE INDEX idx_entry_priority_id ON entry_priority(entry_id);
CREATE INDEX idx_entries_id ON entries(id);
CREATE VIRTUAL TABLE dictionary_fts USING fts5(
  entry_id UNINDEXED,
  kanjis,
  readings,
  glosses,
  priority UNINDEXED,
  tokenize = 'unicode61 remove_diacritics 2'
)
/* dictionary_fts(entry_id,kanjis,readings,glosses,priority) */;
CREATE TABLE IF NOT EXISTS 'dictionary_fts_data'(id INTEGER PRIMARY KEY, block BLOB);
CREATE TABLE IF NOT EXISTS 'dictionary_fts_idx'(segid, term, pgno, PRIMARY KEY(segid, term)) WITHOUT ROWID;
CREATE TABLE IF NOT EXISTS 'dictionary_fts_content'(id INTEGER PRIMARY KEY, c0, c1, c2, c3, c4);
CREATE TABLE IF NOT EXISTS 'dictionary_fts_docsize'(id INTEGER PRIMARY KEY, sz BLOB);
CREATE TABLE IF NOT EXISTS 'dictionary_fts_config'(k PRIMARY KEY, v) WITHOUT ROWID;
CREATE TABLE cards (
    id INTEGER PRIMARY KEY AUTOINCREMENT,

    entry_id INTEGER NOT NULL,

    repetitions INTEGER NOT NULL DEFAULT 0,
    ease_factor REAL NOT NULL DEFAULT 2.5,
    interval INTEGER NOT NULL DEFAULT 1,

    last_reviewed INTEGER,
    due_date INTEGER,

    custom_question TEXT,
    custom_answer TEXT,

    status INTEGER DEFAULT 0,  -- 0 = activa, 1 = suspendida, etc.

    FOREIGN KEY (entry_id) REFERENCES entries(id)
);
