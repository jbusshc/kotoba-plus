cmake_minimum_required(VERSION 3.10)
project(libkotobaplus C)

# Estándar C
set(CMAKE_C_STANDARD 99)

# Incluir headers
include_directories(include)

# Archivos fuente
file(GLOB_RECURSE SRC_FILES src/*.c)

# Crear la librería dinámica
add_library(libkotobaplus SHARED ${SRC_FILES})

# Exportar símbolos en Windows
if(MSVC OR WIN32)
    target_compile_definitions(libkotobaplus PRIVATE KOTOBA_BUILD_DLL)
endif()

# Salida de archivos según plataforma
if(WIN32)
    set_target_properties(libkotobaplus PROPERTIES
        OUTPUT_NAME "kotobaplus"
        PREFIX ""    # Sin 'lib' en Windows
        SUFFIX ".dll"
    )
elseif(APPLE)
    set_target_properties(libkotobaplus PROPERTIES
        OUTPUT_NAME "kotobaplus"
        PREFIX "lib"
        SUFFIX ".dylib"
    )
else()
    set_target_properties(libkotobaplus PROPERTIES
        OUTPUT_NAME "kotobaplus"
        PREFIX "lib"
        SUFFIX ".so"
    )
endif()

# Directorios de salida organizados
set_target_properties(libkotobaplus PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# SQLite3 support
find_package(SQLite3 REQUIRED)
target_include_directories(libkotobaplus PRIVATE ${SQLite3_INCLUDE_DIRS})
target_link_libraries(libkotobaplus PRIVATE ${SQLite3_LIBRARIES})

# CJSON support
find_package(CJSON REQUIRED)
target_include_directories(libkotobaplus PRIVATE ${CJSON_INCLUDE_DIRS})
target_link_libraries(libkotobaplus PRIVATE ${CJSON_LIBRARIES})

# Copiar el DLL generado a dos carpetas específicas en Windows
if(WIN32)
    add_custom_command(TARGET libkotobaplus POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:libkotobaplus>
            "C:/Users/Jorge/Desktop/Proyectos/kotobaplus/cli-kotobaplus/bin"
    )
    add_custom_command(TARGET libkotobaplus POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:libkotobaplus>
            "C:/Users/Jorge/Desktop/Proyectos/kotobaplus/qt-desktop/build/Desktop_Qt_6_9_1_MinGW_64_bit-Debug/debug"
    )
    add_custom_command(TARGET libkotobaplus POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:libkotobaplus>
            "C:/Users/Jorge/Desktop/Proyectos/kotobaplus/xml-parser/bin"
    )
endif()
