cmake_minimum_required(VERSION 3.20)

project(kotoba_gui_qml LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS
    Quick
    Qml
    QuickControls2
)

qt_standard_project_setup()

file(GLOB_RECURSE GUIQML_SOURCES
    CONFIGURE_DEPENDS
    app/*.cpp
    infrastructure/*.cpp
    viewmodels/*.cpp
)

qt_add_resources(GUIQML_RES
    resources/qml.qrc
)

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
