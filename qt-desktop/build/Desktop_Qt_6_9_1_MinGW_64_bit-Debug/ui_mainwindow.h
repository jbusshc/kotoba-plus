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
#include <QtWidgets/QFrame>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QListView>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QStatusBar>
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
    QScrollArea *detailsScroll;
    QWidget *detailsContainer;
    QVBoxLayout *detailsLayout;
    QLabel *labelMainWord;
    QLabel *labelReadings;
    QFrame *lineSeparator1;
    QLabel *labelSensesTitle;
    QListWidget *listSenses;
    QSpacerItem *spacerBottom;
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

        detailsScroll = new QScrollArea(pageDetails);
        detailsScroll->setObjectName("detailsScroll");
        detailsScroll->setWidgetResizable(true);
        detailsContainer = new QWidget();
        detailsContainer->setObjectName("detailsContainer");
        detailsContainer->setGeometry(QRect(0, 0, 754, 434));
        detailsLayout = new QVBoxLayout(detailsContainer);
        detailsLayout->setObjectName("detailsLayout");
        labelMainWord = new QLabel(detailsContainer);
        labelMainWord->setObjectName("labelMainWord");
        labelMainWord->setAlignment(Qt::AlignmentFlag::AlignLeading|Qt::AlignmentFlag::AlignLeft|Qt::AlignmentFlag::AlignTop);

        detailsLayout->addWidget(labelMainWord);

        labelReadings = new QLabel(detailsContainer);
        labelReadings->setObjectName("labelReadings");

        detailsLayout->addWidget(labelReadings);

        lineSeparator1 = new QFrame(detailsContainer);
        lineSeparator1->setObjectName("lineSeparator1");
        lineSeparator1->setFrameShape(QFrame::Shape::HLine);
        lineSeparator1->setFrameShadow(QFrame::Shadow::Sunken);

        detailsLayout->addWidget(lineSeparator1);

        labelSensesTitle = new QLabel(detailsContainer);
        labelSensesTitle->setObjectName("labelSensesTitle");

        detailsLayout->addWidget(labelSensesTitle);

        listSenses = new QListWidget(detailsContainer);
        listSenses->setObjectName("listSenses");

        detailsLayout->addWidget(listSenses);

        spacerBottom = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        detailsLayout->addItem(spacerBottom);

        detailsScroll->setWidget(detailsContainer);

        layoutDetails->addWidget(detailsScroll);

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
        labelMainWord->setStyleSheet(QCoreApplication::translate("MainWindow", "\n"
"font-size: 32px;\n"
"font-weight: bold;\n"
"padding: 6px;\n"
"               ", nullptr));
        labelMainWord->setText(QCoreApplication::translate("MainWindow", "\342\200\224", nullptr));
        labelReadings->setStyleSheet(QCoreApplication::translate("MainWindow", "\n"
"font-size: 20px;\n"
"color: #444;\n"
"padding-left: 6px;\n"
"               ", nullptr));
        labelReadings->setText(QCoreApplication::translate("MainWindow", "Lecturas aqu\303\255\342\200\246", nullptr));
        labelSensesTitle->setStyleSheet(QCoreApplication::translate("MainWindow", "\n"
"font-size: 18px;\n"
"font-weight: 600;\n"
"padding: 6px 0 0 6px;\n"
"               ", nullptr));
        labelSensesTitle->setText(QCoreApplication::translate("MainWindow", "Significados", nullptr));
        listSenses->setStyleSheet(QCoreApplication::translate("MainWindow", "\n"
"font-size: 16px;\n"
"padding: 4px;\n"
"               ", nullptr));
        labelSRS->setText(QCoreApplication::translate("MainWindow", "Contenido SRS", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
