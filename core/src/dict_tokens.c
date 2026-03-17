#include "dict_tokens.h"
#include "types.h"
#include <string.h>

const char* dial[] = {
    "Brazilian",
    "Hokkaido-ben",
    "Kansai-ben",
    "Kantou-ben",
    "Kyoto-ben",
    "Kyuushuu-ben",
    "Nagano-ben",
    "Osaka-ben",
    "Ryuukyuu-ben",
    "Touhoku-ben",
    "Tosa-ben",
    "Tsugaru-ben"
};

const int dial_count = sizeof(dial) / sizeof(dial[0]);

int dial_token_index(const char *dial_str)
{
    for (int i = 0; i < dial_count; i++)
    {
        if (strcmp(dial_str, dial[i]) == 0)
        {
            return i;
        }
    }
    return -1; // not found
}

const char* dial_token_str(int index)
{
    if (index >= 0 && index < dial_count)
    {
        return dial[index];
    }
    return NULL; // invalid index
}

const char* field[] = {
    "agriculture",
    "anatomy",
    "archeology",
    "architecture",
    "art, aesthetics",
    "astronomy",
    "audiovisual",
    "aviation",
    "baseball",
    "biochemistry",
    "biology",
    "botany",
    "boxing",
    "Buddhism",
    "business",
    "card games",
    "chemistry",
    "Chinese mythology",
    "Christianity",
    "civil engineering",
    "clothing",
    "computing",
    "crystallography",
    "dentistry",
    "ecology",
    "economics",
    "electricity, elec. eng.",
    "electronics",
    "embryology",
    "engineering",
    "entomology",
    "figure skating",
    "film",
    "finance",
    "fishing",
    "food, cooking",
    "gardening, horticulture",
    "genetics",
    "geography",
    "geology",
    "geometry",
    "go (game)",
    "golf",
    "grammar",
    "Greek mythology",
    "hanafuda",
    "horse racing",
    "Internet",
    "Japanese mythology",
    "kabuki",
    "law",
    "linguistics",
    "logic",
    "martial arts",
    "mahjong",
    "manga",
    "mathematics",
    "mechanical engineering",
    "medicine",
    "meteorology",
    "military",
    "mineralogy",
    "mining",
    "motorsport",
    "music",
    "noh",
    "ornithology",
    "paleontology",
    "pathology",
    "pharmacology",
    "philosophy",
    "photography",
    "physics",
    "physiology",
    "politics",
    "printing",
    "professional wrestling",
    "psychiatry",
    "psychoanalysis",
    "psychology",
    "railway",
    "Roman mythology",
    "Shinto",
    "shogi",
    "skiing",
    "sports",
    "statistics",
    "stock market",
    "sumo",
    "surgery",
    "telecommunications",
    "trademark",
    "television",
    "veterinary terms",
    "video games",
    "zoology"
};
const int field_count = sizeof(field) / sizeof(field[0]);

int field_token_index(const char *field_str)
{
    for (int i = 0; i < field_count; i++)
    {
        if (strcmp(field_str, field[i]) == 0)
        {
            return i;
        }
    }
    return -1; // not found
}

const char* field_token_str(int index)
{
    if (index >= 0 && index < field_count)
    {
        return field[index];
    }
    return NULL; // invalid index
}

const char* ke_inf[] = {
    "ateji (phonetic) reading",
    "ateji (semantic) reading",
    "gikun (meaning) reading",
    "jukujikun reading",
    "nanori reading",
    "okurigana irregularity",
    "irregular okurigana",
    "search-only kanji form",
    "rarely used kanji form",
    "word usually written using kanji alone",
    "word usually written using kana alone",
    "word containing irregular kana usage",
    "word containing irregular kanji usage",
    "irregular okurigana usage",
    "word containing out-dated kanji or kanji usage"
};
const int ke_inf_count = sizeof(ke_inf) / sizeof(ke_inf[0]);

int ke_inf_token_index(const char *ke_inf_str)
{
    for (int i = 0; i < ke_inf_count; i++)
    {
        if (strcmp(ke_inf_str, ke_inf[i]) == 0)
        {
            return i;
        }
    }
    return -1; // not found
}

const char* ke_inf_token_str(int index)
{
    if (index >= 0 && index < ke_inf_count)
    {
        return ke_inf[index];
    }
    return NULL; // invalid index
}

const char* misc[] = {
    "abbreviation",
    "archaic",
    "character",
    "children's language",
    "colloquial",
    "company name",
    "creature",
    "dated term",
    "deity",
    "derogatory",
    "document",
    "euphemistic",
    "event",
    "familiar language",
    "female term or language",
    "fiction",
    "formal or literary term",
    "given name or forename, gender not specified",
    "group",
    "historical term",
    "honorific or respectful (sonkeigo) language",
    "humble (kenjougo) language",
    "idiomatic expression",
    "jocular, humorous term",
    "legend",
    "manga slang",
    "male term or language",
    "mythology",
    "Internet slang",
    "object",
    "obsolete term",
    "onomatopoeic or mimetic word",
    "organization name",
    "other",
    "full name of a particular person",
    "place name",
    "poetical term",
    "polite (teineigo) language",
    "product name",
    "proverb",
    "quotation",
    "rare term",
    "religion",
    "sensitive",
    "service",
    "ship name",
    "slang",
    "railway station",
    "family or surname",
    "word usually written using kana alone",
    "unclassified name",
    "vulgar expression or word",
    "work of art, literature, music, etc. name",
    "rude or X-rated term (not displayed in educational software)",
    "yojijukugo"
};
const int misc_count = sizeof(misc) / sizeof(misc[0]);

int misc_token_index(const char *misc_str)
{
    for (int i = 0; i < misc_count; i++)
    {
        if (strcmp(misc_str, misc[i]) == 0)
        {
            return i;
        }
    }
    return -1;
}

const char* misc_token_str(int index)
{
    if (index >= 0 && index < misc_count)
    {
        return misc[index];
    }
    return NULL;
}

const char *pos[] = {
    "noun or verb acting prenominally",
        "adjective (keiyoushi)",
        "adjective (keiyoushi) - yoi/ii class",
        "'kari' adjective (archaic)",
        "'ku' adjective (archaic)",
        "adjectival nouns or quasi-adjectives (keiyodoshi)",
        "archaic/formal form of na-adjective",
        "nouns which may take the genitive case particle 'no'",
        "pre-noun adjectival (rentaishi)",
        "'shiku' adjective (archaic)",
        "'taru' adjective",
        "adverb (fukushi)",
        "adverb taking the 'to' particle",
        "auxiliary",
        "auxiliary adjective",
        "auxiliary verb",
        "conjunction",
        "copula",
        "counter",
        "expressions (phrases, clauses, etc.)",
        "interjection (kandoushi)",
        "noun (common) (futsuumeishi)",
        "adverbial noun (fukushitekimeishi)",
        "proper noun",
        "noun, used as a prefix",
        "noun, used as a suffix",
        "noun (temporal) (jisoumeishi)",
        "numeric",
        "pronoun",
        "prefix",
        "particle",
        "suffix",
        "unclassified",
        "verb unspecified",
        "Ichidan verb",
        "Ichidan verb - kureru special class",
        "Nidan verb with 'u' ending (archaic)",
        "Nidan verb (upper class) with 'bu' ending (archaic)",
        "Nidan verb (lower class) with 'bu' ending (archaic)",
        "Nidan verb (upper class) with 'dzu' ending (archaic)",
        "Nidan verb (lower class) with 'dzu' ending (archaic)",
        "Nidan verb (upper class) with 'gu' ending (archaic)",
        "Nidan verb (lower class) with 'gu' ending (archaic)",
        "Nidan verb (upper class) with 'hu/fu' ending (archaic)",
        "Nidan verb (lower class) with 'hu/fu' ending (archaic)",
        "Nidan verb (upper class) with 'ku' ending (archaic)",
        "Nidan verb (lower class) with 'ku' ending (archaic)",
        "Nidan verb (upper class) with 'mu' ending (archaic)",
        "Nidan verb (lower class) with 'mu' ending (archaic)",
        "Nidan verb (lower class) with 'nu' ending (archaic)",
        "Nidan verb (upper class) with 'ru' ending (archaic)",
        "Nidan verb (lower class) with 'ru' ending (archaic)",
        "Nidan verb (lower class) with 'su' ending (archaic)",
        "Nidan verb (upper class) with 'tsu' ending (archaic)",
        "Nidan verb (lower class) with 'tsu' ending (archaic)",
        "Nidan verb (lower class) with 'u' ending and 'we' conjugation (archaic)",
        "Nidan verb (upper class) with 'yu' ending (archaic)",
        "Nidan verb (lower class) with 'yu' ending (archaic)",
        "Nidan verb (lower class) with 'zu' ending (archaic)",
        "Yodan verb with 'bu' ending (archaic)",
        "Yodan verb with 'gu' ending (archaic)",
        "Yodan verb with 'hu/fu' ending (archaic)",
        "Yodan verb with 'ku' ending (archaic)",
        "Yodan verb with 'mu' ending (archaic)",
        "Yodan verb with 'nu' ending (archaic)",
        "Yodan verb with 'ru' ending (archaic)",
        "Yodan verb with 'su' ending (archaic)",
        "Yodan verb with 'tsu' ending (archaic)",
        "Godan verb - -aru special class",
        "Godan verb with 'bu' ending",
        "Godan verb with 'gu' ending",
        "Godan verb with 'ku' ending",
        "Godan verb - Iku/Yuku special class",
        "Godan verb with 'mu' ending",
        "Godan verb with 'nu' ending",
        "Godan verb with 'ru' ending",
        "Godan verb with 'ru' ending (irregular verb)",
        "Godan verb with 'su' ending",
        "Godan verb with 'tsu' ending",
        "Godan verb with 'u' ending",
        "Godan verb with 'u' ending (special class)",
        "Godan verb - Uru old class verb (old form of Eru)",
        "intransitive verb",
        "Kuru verb - special class",
        "irregular nu verb",
        "irregular ru verb, plain form ends with -ri",
        "noun or participle which takes the aux. verb suru",
        "su verb - precursor to the modern suru",
        "suru verb - included",
        "suru verb - special class",
        "transitive verb",
        "Ichidan verb - zuru verb (alternative form of -jiru verbs)"
};
const int pos_count = sizeof(pos) / sizeof(pos[0]);

int pos_token_index(const char *pos_str)
{
    for (int i = 0; i < pos_count; i++)
    {
        if (strcmp(pos_str, pos[i]) == 0)
        {
            return i;
        }
    }
    return -1; // not found
}

const char* pos_token_str(int index)
{
    if (index >= 0 && index < pos_count)
    {
        return pos[index];
    }
    return NULL; // invalid index
}

const char* re_inf[] = {
    "gikun (meaning as reading) or jukujikun (special kanji reading)",
    "word containing irregular kana usage",
    "out-dated or obsolete kana usage",
    "rarely used kana form",
    "search-only kana form"
};
const int re_inf_count = sizeof(re_inf) / sizeof(re_inf[0]);

int re_inf_token_index(const char *re_inf_str)
{
    for (int i = 0; i < re_inf_count; i++)
    {
        if (strcmp(re_inf_str, re_inf[i]) == 0)
        {
            return i;
        }
    }
    return -1; // not found
}

const char* re_inf_token_str(int index)
{
    if (index >= 0 && index < re_inf_count)
    {
        return re_inf[index];
    }
    return NULL; // invalid index
}

const char* pri_types[] = {
    "news1",
    "news2",
    "ichi1",
    "ichi2",
    "spec1",
    "spec2",
    "gai1",
    "gai2",
    "nf01",
    "nf02",
    "nf03",
    "nf04",
    "nf05",
    "nf06",
    "nf07",
    "nf08",
    "nf09",
    "nf10",
    "nf11",
    "nf12",
    "nf13",
    "nf14",
    "nf15",
    "nf16",
    "nf17",
    "nf18",
    "nf19",
    "nf20",
    "nf21",
    "nf22",
    "nf23",
    "nf24",
    "nf25",
    "nf26",
    "nf27",
    "nf28",
    "nf29",
    "nf30",
    "nf31",
    "nf32",
    "nf33",
    "nf34",
    "nf35",
    "nf36",
    "nf37",
    "nf38",
    "nf39",
    "nf40",
    "nf41",
    "nf42",
    "nf43",
    "nf44",
    "nf45",
    "nf46",
    "nf47",
    "nf48"
};
const int pri_types_count = sizeof(pri_types) / sizeof(pri_types[0]);

const char** ke_pri = pri_types; // KE_PRI and RE_PRI share the same set of priority types
const int ke_pri_count = pri_types_count;

const char** re_pri = pri_types;
const int re_pri_count = pri_types_count;

int ke_pri_token_index(const char *ke_pri_str)
{
    for (int i = 0; i < ke_pri_count; i++)
    {
        if (strcmp(ke_pri_str, ke_pri[i]) == 0)
        {
            return i;
        }
    }
    return -1; // not found
}

const char* ke_pri_token_str(int index)
{
    if (index >= 0 && index < ke_pri_count)
    {
        return ke_pri[index];
    }
    return NULL; // invalid index
}

int re_pri_token_index(const char *re_pri_str)
{
    for (int i = 0; i < re_pri_count; i++)
    {
        if (strcmp(re_pri_str, re_pri[i]) == 0)
        {
            return i;
        }
    }
    return -1; // not found
}

const char* re_pri_token_str(int index)
{
    if (index >= 0 && index < re_pri_count)
    {
        return re_pri[index];
    }
    return NULL; // invalid index
}