cmake_minimum_required(VERSION 3.20)
project(kotoba-cli LANGUAGES C)


# Crear un target vacío para que project_options no falle
add_library(project_options INTERFACE)

# Agrega core como subdirectorio (no cambiamos su CMake)
add_subdirectory(../core core-build)

# Busca LibXml2
find_package(LibXml2 REQUIRED)

# Define el ejecutable
add_executable(kotoba-cli
    src/main.c
)

# Carpeta de salida del binario
set_target_properties(kotoba-cli PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/../bin"
)

# Link con core y LibXml2
target_link_libraries(kotoba-cli PRIVATE core LibXml2::LibXml2)

# Definiciones de compilación
target_compile_definitions(kotoba-cli PRIVATE KOTOBA_USE_DLL)

# Si core es shared, copia el binario al mismo directorio
get_target_property(_type core TYPE)
if(_type STREQUAL "SHARED_LIBRARY")
    add_custom_command(TARGET kotoba-cli POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:core>
            $<TARGET_FILE_DIR:kotoba-cli>
    )

endif()