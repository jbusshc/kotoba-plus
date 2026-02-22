cmake_minimum_required(VERSION 3.10)
project(core C)

option(KOTOBA_BUILD_SHARED "Build core as a shared library (DLL)" ON)

set(KOTOBA_PUBLIC_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

file(GLOB_RECURSE KOTOBA_SRC src/*.c)

if(KOTOBA_BUILD_SHARED)
    add_library(core SHARED ${KOTOBA_SRC})
    target_compile_definitions(core PRIVATE KOTOBA_BUILD_DLL)
else()
    add_library(core STATIC ${KOTOBA_SRC})
endif()

target_compile_features(core PUBLIC c_std_99)

target_include_directories(core
    PUBLIC
        ${KOTOBA_PUBLIC_INCLUDE_DIR}
)
