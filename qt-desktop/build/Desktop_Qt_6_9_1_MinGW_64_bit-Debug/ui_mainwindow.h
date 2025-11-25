/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 6.9.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QListView>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralwidget;
    QVBoxLayout *verticalLayoutMain;
    QHBoxLayout *tabBarLayout;
    QToolButton *toolBtnDiccionario;
    QToolButton *toolBtnSRS;
    QStackedWidget *stackedWidget;
    QWidget *pageDictionary;
    QVBoxLayout *layoutDictionary;
    QLineEdit *lineEditSearch;
    QListView *searchResultList;
    QWidget *pageDetails;
    QVBoxLayout *layoutDetails;
    QToolButton *btnBack;
    QTextEdit *detailTextEdit;
    QWidget *pageSRS;
    QVBoxLayout *layoutSRS;
    QLabel *labelSRS;
    QMenuBar *menubar;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName("MainWindow");
        MainWindow->resize(800, 600);
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName("centralwidget");
        verticalLayoutMain = new QVBoxLayout(centralwidget);
        verticalLayoutMain->setObjectName("verticalLayoutMain");
        tabBarLayout = new QHBoxLayout();
        tabBarLayout->setObjectName("tabBarLayout");
        toolBtnDiccionario = new QToolButton(centralwidget);
        toolBtnDiccionario->setObjectName("toolBtnDiccionario");
        toolBtnDiccionario->setCheckable(true);

        tabBarLayout->addWidget(toolBtnDiccionario);

        toolBtnSRS = new QToolButton(centralwidget);
        toolBtnSRS->setObjectName("toolBtnSRS");
        toolBtnSRS->setCheckable(true);

        tabBarLayout->addWidget(toolBtnSRS);


        verticalLayoutMain->addLayout(tabBarLayout);

        stackedWidget = new QStackedWidget(centralwidget);
        stackedWidget->setObjectName("stackedWidget");
        pageDictionary = new QWidget();
        pageDictionary->setObjectName("pageDictionary");
        layoutDictionary = new QVBoxLayout(pageDictionary);
        layoutDictionary->setObjectName("layoutDictionary");
        lineEditSearch = new QLineEdit(pageDictionary);
        lineEditSearch->setObjectName("lineEditSearch");

        layoutDictionary->addWidget(lineEditSearch);

        searchResultList = new QListView(pageDictionary);
        searchResultList->setObjectName("searchResultList");

        layoutDictionary->addWidget(searchResultList);

        stackedWidget->addWidget(pageDictionary);
        pageDetails = new QWidget();
        pageDetails->setObjectName("pageDetails");
        layoutDetails = new QVBoxLayout(pageDetails);
        layoutDetails->setObjectName("layoutDetails");
        btnBack = new QToolButton(pageDetails);
        btnBack->setObjectName("btnBack");
        btnBack->setAutoRaise(true);

        layoutDetails->addWidget(btnBack);

        detailTextEdit = new QTextEdit(pageDetails);
        detailTextEdit->setObjectName("detailTextEdit");

        layoutDetails->addWidget(detailTextEdit);

        stackedWidget->addWidget(pageDetails);
        pageSRS = new QWidget();
        pageSRS->setObjectName("pageSRS");
        layoutSRS = new QVBoxLayout(pageSRS);
        layoutSRS->setObjectName("layoutSRS");
        labelSRS = new QLabel(pageSRS);
        labelSRS->setObjectName("labelSRS");

        layoutSRS->addWidget(labelSRS);

        stackedWidget->addWidget(pageSRS);

        verticalLayoutMain->addWidget(stackedWidget);

        MainWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(MainWindow);
        menubar->setObjectName("menubar");
        menubar->setGeometry(QRect(0, 0, 800, 25));
        MainWindow->setMenuBar(menubar);
        statusbar = new QStatusBar(MainWindow);
        statusbar->setObjectName("statusbar");
        MainWindow->setStatusBar(statusbar);

        retranslateUi(MainWindow);

        stackedWidget->setCurrentIndex(1);


        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "App Diccionario", nullptr));
        toolBtnDiccionario->setText(QCoreApplication::translate("MainWindow", "Diccionario", nullptr));
        toolBtnSRS->setText(QCoreApplication::translate("MainWindow", "SRS", nullptr));
        lineEditSearch->setPlaceholderText(QCoreApplication::translate("MainWindow", "Buscar...", nullptr));
        btnBack->setText(QCoreApplication::translate("MainWindow", "\342\206\220", nullptr));
        labelSRS->setText(QCoreApplication::translate("MainWindow", "Contenido SRS", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
