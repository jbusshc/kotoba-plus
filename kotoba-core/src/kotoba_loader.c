#include "kotoba_loader.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#endif

int kotoba_open(kotoba_db *db, const char *path)
{
#ifdef _WIN32
    HANDLE hFile = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ,
                              NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return 0;

    LARGE_INTEGER size;
    GetFileSizeEx(hFile, &size);

    HANDLE hMap = CreateFileMappingA(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
    if (!hMap) return 0;

    void *map = MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0);
    CloseHandle(hMap);
    CloseHandle(hFile);
#else
    int fd = open(path, O_RDONLY);
    if (fd < 0) return 0;

    struct stat st;
    fstat(fd, &st);

    void *map = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);
#endif

    const uint8_t *p = map;
    const kotoba_file_header *h = (const kotoba_file_header *)p;

    db->map = map;
    db->size = h->entry_count;
    db->entry_count = h->entry_count;
    db->index = (const entry_index *)(p + sizeof(*h));
    db->entries_base = p;

    return 1;
}

void kotoba_close(kotoba_db *db)
{
#ifdef _WIN32
    UnmapViewOfFile(db->map);
#else
    munmap(db->map, db->size);
#endif
}
