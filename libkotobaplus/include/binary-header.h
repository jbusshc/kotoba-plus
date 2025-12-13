#pragma pack(push, 1)
typedef struct {
    char magic[8];          // "KOTOBA+\0"
    uint32_t header_size;   // Tamaño total del header

    // Versiones
    uint32_t db_version;    // Versión de tu propio formato
    uint32_t jmdict_version; // Año/mes/día de la release de JMdict (ej. 20250120)

    // Cantidad de entradas
    uint32_t entry_count;

    // Offsets dentro del blob
    uint64_t offset_entries;   // Inicio de la tabla de entradas (array de structs pequeños)

    // Tamaños de cada sección
    uint64_t size_entries;
    uint64_t size_strings;
    uint64_t size_examples;
    uint64_t size_index;

    // Checksum simple para validar corrupción
    uint64_t checksum;

} KPHeader;
#pragma pack(pop)
