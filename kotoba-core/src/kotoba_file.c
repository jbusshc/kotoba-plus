#include "kotoba_file.h"
#include <string.h>

#ifndef _WIN32
int kotoba_file_open(kotoba_file *kf, const char *path)
{
    struct stat st;
    kf->fd = open(path, O_RDONLY);
    if (kf->fd < 0) return 0;

    if (fstat(kf->fd, &st) != 0) {
        close(kf->fd);
        return 0;
    }

    kf->size = st.st_size;
    kf->base = mmap(NULL, kf->size, PROT_READ, MAP_PRIVATE, kf->fd, 0);
    if (kf->base == MAP_FAILED) {
        close(kf->fd);
        return 0;
    }

    if (kf->size < sizeof(kotoba_bin_header)) return 0;
    memcpy(&kf->header, kf->base, sizeof(kotoba_bin_header));

    if (kf->header.magic != KOTOBA_MAGIC) return 0;
    if (kf->header.version != KOTOBA_VERSION) return 0;

    return 1;
}

void kotoba_file_close(kotoba_file *kf)
{
    if (kf->base) munmap((void *)kf->base, kf->size);
    if (kf->fd >= 0) close(kf->fd);
}

#else
#include <windows.h>

int kotoba_file_open(kotoba_file *kf, const char *path)
{
    kf->hFile = CreateFileA(path, GENERIC_READ,
                            FILE_SHARE_READ, NULL,
                            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (kf->hFile == INVALID_HANDLE_VALUE) return 0;

    LARGE_INTEGER sz;
    if (!GetFileSizeEx(kf->hFile, &sz)) {
        CloseHandle(kf->hFile);
        return 0;
    }
    kf->size = (size_t)sz.QuadPart;

    kf->hMap = CreateFileMappingA(kf->hFile, NULL,
                                  PAGE_READONLY, 0, 0, NULL);
    if (!kf->hMap) {
        CloseHandle(kf->hFile);
        return 0;
    }

    kf->base = MapViewOfFile(kf->hMap, FILE_MAP_READ, 0, 0, 0);
    if (!kf->base) {
        CloseHandle(kf->hMap);
        CloseHandle(kf->hFile);
        return 0;
    }

    if (kf->size < sizeof(kotoba_bin_header)) return 0;
    memcpy(&kf->header, kf->base, sizeof(kotoba_bin_header));

    if (kf->header.magic != KOTOBA_MAGIC) return 0;
    if (kf->header.version != KOTOBA_VERSION) return 0;

    return 1;
}

void kotoba_file_close(kotoba_file *kf)
{
    if (kf->base) UnmapViewOfFile(kf->base);
    if (kf->hMap) CloseHandle(kf->hMap);
    if (kf->hFile) CloseHandle(kf->hFile);
}

#endif
