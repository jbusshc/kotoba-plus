QT += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
CONFIG += c++17

SOURCES += \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    mainwindow.h

FORMS += \
    mainwindow.ui


# =========================
# kotoba-core
# =========================

KOTOBA_CORE_DIR   = $$PWD/../kotoba-core
KOTOBA_CORE_BUILD = $$KOTOBA_CORE_DIR/build
KOTOBA_CORE_DLL   = $$absolute_path($$KOTOBA_CORE_BUILD/libkotoba-core.dll)

INCLUDEPATH += \
    $$KOTOBA_CORE_DIR/include

unix {
    LIBS += -L$$KOTOBA_CORE_BUILD -lkotoba-core
}

win32 {
    LIBS += -L$$KOTOBA_CORE_BUILD -lkotoba-core
    DEFINES += KOTOBA_USE_DLL
}
