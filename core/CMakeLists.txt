cmake_minimum_required(VERSION 3.20)

project(core LANGUAGES C)

option(KOTOBA_BUILD_SHARED "Build core as shared library" ON)

set(KOTOBA_PUBLIC_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

file(GLOB_RECURSE KOTOBA_SRC CONFIGURE_DEPENDS src/*.c)

if(KOTOBA_BUILD_SHARED)
    add_library(core SHARED ${KOTOBA_SRC})
    target_compile_definitions(core PRIVATE KOTOBA_BUILD_DLL)
else()
    add_library(core STATIC ${KOTOBA_SRC})
endif()

target_compile_features(core PUBLIC c_std_99)

# Muy importante para Qt / Android
set_target_properties(core PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)

# Quita warning de pila ejecutable
if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
    target_compile_options(core PRIVATE -Wa,--noexecstack)
endif()

target_include_directories(core
    PUBLIC
        ${KOTOBA_PUBLIC_INCLUDE_DIR}
)

target_link_libraries(core PUBLIC project_options)