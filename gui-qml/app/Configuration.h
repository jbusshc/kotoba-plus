#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <QString>
#include <QVariant>
#include <QMap>
#include "../../core/include/types.h"

// Struct filled with all the configuration options for the application, with default values
struct Configuration {
    QString filePath = "config.ini";
    // General
    bool firstRun = true;
    QString appVersion = "1.0";
    bool autoSave = true;
    bool checkUpdates = false;

    // UI
    QString theme = "dark";
    QString primaryColor = "indigo";
    QString accentColor = "blue";
    QString fontFamily = "default";
    int fontSize = 14;
    bool showFurigana = true;
    bool compactMode = false;
    bool animations = true;

    // Language
    QString interface = "en";
    QString glossLanguages = "en";
    QString fallbackLanguage = "en";
    bool languages[KOTOBA_LANG_COUNT] = {false}; // initialized to false, will be set to true for active gloss languages

    // Dictionary
    int maxResults = 50;
    bool searchOnTyping = true;
    int searchDelayMs = 150;
    bool rememberLastQuery = true;
    bool highlightMatches = true;

    // SRS
    int dailyNewCards = 20;
    int dailyReviewLimit = 200;
    QString learningSteps = "1,10";
    int graduatingInterval = 1;
    int easyInterval = 4;
    double startingEase = 2.5;
    bool buryRelated = true;
    bool showAnswerAuto = false;

    // Audio
    bool audioEnabled = false;
    int volume = 80;
    bool autoPlay = false;

    // Data
    QString dictPath = "dict.kotoba";
    QString dictIndexPath = "dict.kotoba.idx";
    QString srsDbPath = "srs.bin";
    QString srsEventLogPath = "srs_events.log";
    QString jpPath = "jp.invx";
    QString glossEnPath = "gloss_en.invx";
    QString glossEsPath = "gloss_es.invx";
    bool backupEnabled = true;
    int backupIntervalDays = 7;
};

void saveConfiguration(const Configuration &config, const QString &filePath);
void loadConfiguration(Configuration &config, const QString &filePath);

#endif // CONFIGURATION_H