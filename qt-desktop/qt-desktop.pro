QT       += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
CONFIG += c++17

SOURCES += \
    main.cpp \
    mainwindow.cpp \

HEADERS += \
    mainwindow.h \
    ../libkotobaplus/include/libkotobaplus.h \
    listitemdelegate.h \



INCLUDEPATH += ../libkotobaplus/include

LIBS += -L$$PWD/../libkotobaplus/build/lib -ltango

FORMS += \
    mainwindow.ui
