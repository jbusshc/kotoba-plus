cmake_minimum_required(VERSION 3.10)
project(kotoba-cli C)

add_subdirectory(../kotoba-core kotoba-core-build)

find_package(LibXml2 REQUIRED)

add_executable(kotoba-cli
    src/main.c
)

set_target_properties(kotoba-cli PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../bin)

target_link_libraries(kotoba-cli PRIVATE kotoba-core ${LIBXML2_LIBRARIES})
target_include_directories(kotoba-cli PRIVATE ${LIBXML2_INCLUDE_DIR})
target_compile_definitions(kotoba-cli PRIVATE KOTOBA_USE_DLL)

if(KOTOBA_BUILD_SHARED)
    add_custom_command(TARGET kotoba-cli POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_SOURCE_DIR}/../kotoba-core/build/kotoba-core.dll
            ${CMAKE_BINARY_DIR}/../bin/
    )
endif()