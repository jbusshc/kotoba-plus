#include "app_context.h"
#include <cstring>

KotobaAppContext::KotobaAppContext()
{
    kotoba_dict_open(&dictionary,
                     "dict.kotoba",
                     "dict.kotoba.idx");

    std::memset(languages, 0, sizeof(languages));
    languages[KOTOBA_LANG_EN] = true;
    languages[KOTOBA_LANG_ES] = true;
}

KotobaAppContext::~KotobaAppContext()
{
    kotoba_dict_close(&dictionary);
}
