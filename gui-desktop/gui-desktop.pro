QT += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
CONFIG += c++17

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    app_context.cpp \
    search_presenter.cpp \
    search_service.cpp \
    search_result_model.cpp


HEADERS += \
    mainwindow.h \
    app_context.h \
    search_presenter.h \
    search_service.h \
    search_result_model.h

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
