cmake_minimum_required(VERSION 3.16)
project(KotobaPlus LANGUAGES C)

# Soporte C estándar
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Opciones para generar DLL en Windows
if(WIN32)
    add_definitions(-DKOTOBA_BUILD_DLL)
endif()

# Carpeta de headers públicos
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Archivos fuente
set(SRC_FILES
    src/kotoba_loader.c
    src/kotoba_writer.c
)

# Crear librería compartida
add_library(kotoba SHARED ${SRC_FILES})

# Definir macro para exportar símbolos en Windows
target_compile_definitions(kotoba PRIVATE KOTOBA_BUILD_DLL)

# Especificar carpeta de salida para la librería
set_target_properties(kotoba PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lib" # Para DLLs en Windows
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lib" # Para .lib en Windows
)
