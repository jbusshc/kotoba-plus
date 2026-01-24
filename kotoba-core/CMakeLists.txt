cmake_minimum_required(VERSION 3.10)
project(kotoba-core C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Opción principal
option(KOTOBA_BUILD_SHARED "Build kotoba-core as a shared library (DLL)" ON)

# Headers públicos
set(KOTOBA_PUBLIC_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Fuentes
file(GLOB_RECURSE KOTOBA_SRC
    src/*.c
)

if(KOTOBA_BUILD_SHARED)
    add_library(kotoba-core SHARED ${KOTOBA_SRC})

    target_compile_definitions(kotoba-core PRIVATE KOTOBA_BUILD_DLL)

    if(WIN32)
        set_target_properties(kotoba-core PROPERTIES
            OUTPUT_NAME "kotoba-core"
            PREFIX ""
            SUFFIX ".dll"
        )
    endif()
else()
    add_library(kotoba-core OBJECT ${KOTOBA_SRC})
endif()

target_include_directories(kotoba-core
    PUBLIC
        ${KOTOBA_PUBLIC_INCLUDE_DIR}
)

# No target_link_libraries needed
