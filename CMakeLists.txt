cmake_minimum_required(VERSION 3.20)

project(kotobaplus
    VERSION 0.1
    LANGUAGES C CXX
)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(project_options INTERFACE)

target_compile_definitions(project_options INTERFACE
    $<$<CONFIG:Debug>:DEBUG>
)

add_subdirectory(core)
add_subdirectory(gui-qml)

# ✔ FIX: nombre válido (sin guiones)
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT kotoba_qml)