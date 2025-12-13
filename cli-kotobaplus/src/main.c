#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "libtango.h"

#include <cjson/cJSON.h>
#include <sqlite3.h>

#ifdef _WIN32
#include <windows.h>
void set_console_utf8() {
    system("chcp 65001 > nul");
}
#endif


int main() {
#ifdef _WIN32
    set_console_utf8();
#endif

    const char* db_path = "tango.db";

    sqlite3* sdb;
    if (sqlite3_open(db_path, &sdb) != SQLITE_OK) {
        fprintf(stderr, "No se pudo abrir SQLite: %s\n", sqlite3_errmsg(sdb));
        return 1;
    }

    TangoDB* db = tango_db_open(db_path);
    if (!db) {
        printf("Error abriendo Tango.\n");
        return 1;
    }

    printf("%.2f MB\n", sizeof(entry) / (1024.0 * 1024.0));
    return 0;
}
