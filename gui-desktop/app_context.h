#pragma once

extern "C" {
#include "loader.h"
}

struct  KotobaAppContext
{
    kotoba_dict dictionary;
    bool languages[KOTOBA_LANG_COUNT];

    KotobaAppContext();
    ~KotobaAppContext();
};
