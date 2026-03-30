cmake_minimum_required(VERSION 3.20)
project(kotoba_gui_qml LANGUAGES CXX)

# ── Qt6
find_package(Qt6 REQUIRED COMPONENTS
    Quick
    Qml
    QuickControls2
)

qt_policy(SET QTP0001 NEW)
qt_policy(SET QTP0004 NEW)
qt_standard_project_setup()

# ── Carpeta de assets original (android/assets)
set(KOTOBAPOS_ASSETS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/android/assets)

# ── Sources
file(GLOB_RECURSE GUIQML_SOURCES
    CONFIGURE_DEPENDS
    app/*.cpp
    infrastructure/*.cpp
    viewmodels/*.cpp
)

# ── QML resources
qt_add_resources(GUIQML_RES
    resources/qml.qrc
)

# ── Ejecutable
qt_add_executable(kotoba-qml
    ${GUIQML_SOURCES}
    ${GUIQML_RES}
)

target_link_libraries(kotoba-qml
    PRIVATE
        Qt6::Quick
        Qt6::Qml
        Qt6::QuickControls2
        core
)

# ── QML singleton
set_source_files_properties(
    resources/qml/theme/Theme.qml
    PROPERTIES QT_QML_SINGLETON_TYPE TRUE
)
set(QT_QUICK_CONTROLS_STYLE "Material")

# ── DESKTOP: copiar assets al lado del ejecutable
if(NOT ANDROID)
    add_custom_command(TARGET kotoba-qml POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${KOTOBAPOS_ASSETS_DIR}
            $<TARGET_FILE_DIR:kotoba-qml>
    )
    # Icono opcional
    target_sources(kotoba-qml PRIVATE app/app.rc)
endif()

# ── ANDROID: empaquetar assets dentro del APK
if(ANDROID)
    set_target_properties(kotoba-qml PROPERTIES
        QT_ANDROID_EXTRA_FILES "${KOTOBAPOS_ASSETS_DIR}"
        QT_ANDROID_PACKAGE_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/android
    )
    set(ANDROID_ABI arm64-v8a)
endif()