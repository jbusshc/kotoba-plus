QT += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
CONFIG += c++17

SOURCES += \
    main.cpp \
    ui/mainwindow.cpp \
    ui/dictionary/dictionarypage.cpp \
    ui/details/detailspage.cpp \
    ui/srs/srspage.cpp \
    app/context.cpp \
    search/presenter.cpp \
    search/service.cpp \
    search/result_model.cpp

HEADERS += \
    ui/mainwindow.h \
    ui/dictionary/dictionarypage.h \
    ui/details/detailspage.h \
    ui/srs/srspage.h \
    app/context.h \
    search/presenter.h \
    search/service.h \
    search/result_model.h

FORMS += \
    ui/mainwindow.ui \
    ui/dictionary/dictionarypage.ui \
    ui/details/detailspage.ui 
    ui/srs/srspage.ui


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
