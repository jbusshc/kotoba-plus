// kotoba_writer.h
#ifndef KOTOBA_WRITER_H
#define KOTOBA_WRITER_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "kotoba_types.h"

#include "api.h"

/* Helper para escribir un string y devolver offset relativo al inicio del entry */
static uint32_t write_string(FILE *fp, const char *str, uint32_t entry_start) {
    uint32_t offset = ftell(fp) - entry_start;
    fwrite(str, 1, strlen(str)+1, fp); // +1 para null terminator
    return offset;
}

/* Helper para escribir un array de offsets desde la pila */
static uint32_t write_offsets(FILE *fp, uint32_t *offsets, int count, uint32_t entry_start) {
    uint32_t off = ftell(fp) - entry_start;
    fwrite(offsets, sizeof(uint32_t), count, fp);
    return off;
}

/* Función principal: escribe un entry_bin completo en el archivo */
KOTOBA_API uint32_t write_entry(FILE *fp, const entry *e);
KOTOBA_API uint32_t write_entry_with_index(FILE *fp_bin, FILE *fp_idx, const entry *e);



#endif /* KOTOBA_WRITER_H */
