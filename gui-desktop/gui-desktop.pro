QT += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
CONFIG += c++17

SOURCES += \
    main.cpp \
    ui/mainwindow.cpp \
    ui/dictionary/dictionarypage.cpp \
    ui/details/detailspage.cpp \
    ui/srs/srspage.cpp \
    ui/srs/srsdashboard.cpp \
    app/context.cpp \
    search/search_presenter.cpp \
    search/search_service.cpp \
    search/result_model.cpp \
    srs/srs_presenter.cpp \
    srs/srs_service.cpp 

HEADERS += \
    ui/mainwindow.h \
    ui/dictionary/dictionarypage.h \
    ui/details/detailspage.h \
    ui/srs/srspage.h \
    ui/srs/srsdashboard.h \
    app/context.h \
    search/search_presenter.h \
    search/search_service.h \
    search/result_model.h \
    srs/srs_presenter.h \
    srs/srs_service.h

FORMS += \
    ui/mainwindow.ui \
    ui/dictionary/dictionarypage.ui \
    ui/details/detailspage.ui \
    ui/srs/srspage.ui \
    ui/srs/srsdashboard.ui


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
