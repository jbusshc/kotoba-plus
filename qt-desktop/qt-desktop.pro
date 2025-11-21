QT       += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
CONFIG += c++17

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    searchresultmodel.cpp

HEADERS += \
    mainwindow.h \
    ../libtango/include/libtango.h \
    searchresultdelegate.h \
    searchresultmodel.h

INCLUDEPATH += ../libtango/include

LIBS += -L$$PWD/../libtango/build/lib -ltango

FORMS += \
    mainwindow.ui
