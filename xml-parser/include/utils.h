// Utility functions for the parser
#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


// ------------- Route Processing -------------
// Processes a source route and normalizes it to a destination route depending on the platform

static inline void process_route(const char* src_route, char* dest_route) {
    if (src_route == NULL || dest_route == NULL) {
        fprintf(stderr, "Error: Null route provided.\n");
        return;
    }
    
    #ifdef _WIN32    
        // Copy the source route to the destination
        strcpy(dest_route, src_route);
        // Normalize the route by replacing '/' with '\\'
        for (char *p = dest_route; *p; ++p) {
            if (*p == '/') *p = '\\';

        }
    #endif
    #ifdef __unix__
        // Copy the source route to the destination
        strncpy(dest_route, src_route, MAX_STR_LEN);
        // Normalize the route by replacing '\\' with '/'
        for (char *p = dest_route; *p; ++p) {
            if (*p == '\\') *p = '/';
        }
    #endif
}


//----------------- Vector --------------------
typedef struct vector {
    void* data;
    size_t size;
    size_t capacity;
    size_t elem_size;
} vector;

#define VEC_INITIAL_CAPACITY 2
#define VEC_AT(vec, type, index) (((type*)(vec)->data)[index])
// Vector functions

// Initializes a vector with a given element size
static inline struct vector* vector_init(size_t elem_size) {
    struct vector* vec = (struct vector*)malloc(sizeof(struct vector));
    if (!vec) return NULL;
    vec->data = malloc(VEC_INITIAL_CAPACITY * elem_size);
    if (!vec->data) {
        free(vec);
        return NULL;
    }
    vec->size = 0;
    vec->capacity = VEC_INITIAL_CAPACITY;
    vec->elem_size = elem_size;
    return vec;
}

// Pushes an element to the end of the vector, resizing if necessary
static inline void vector_push_back(struct vector* vec, const void* elem) {
    if (vec->size >= vec->capacity) {
        vec->capacity = vec->capacity << 1; // duplicate the capacity
        vec->data = realloc(vec->data, vec->capacity * vec->elem_size);
    }
    memcpy((char*)vec->data + vec->size * vec->elem_size, elem, vec->elem_size);
    vec->size++;
}

// Pops an element from the end of the vector
static inline void vector_pop_back(struct vector* vec) {
    if (vec->size > 0) {
        vec->size--;
    }
}

// Gets an element at a specific index
static inline void vector_free(struct vector* vec) {
    if (vec) {
        free(vec->data);
        free(vec);
    }
}



#endif // UTILS_H