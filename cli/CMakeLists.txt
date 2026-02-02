cmake_minimum_required(VERSION 3.10)
project(kotoba-cli C)

add_subdirectory(../kotoba-core kotoba-core-build)

find_package(LibXml2 REQUIRED)

add_executable(kotoba-cli
    src/main.c
)

set_target_properties(kotoba-cli PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../bin
)

target_link_libraries(kotoba-cli PRIVATE kotoba-core LibXml2::LibXml2)
target_compile_definitions(kotoba-cli PRIVATE KOTOBA_USE_DLL)

get_target_property(_type kotoba-core TYPE)
if(_type STREQUAL "SHARED_LIBRARY")
    add_custom_command(TARGET kotoba-cli POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:kotoba-core>
            $<TARGET_FILE_DIR:kotoba-cli>
    )
endif()
