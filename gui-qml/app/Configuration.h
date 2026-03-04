#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <QObject>
#include <QString>
#include "../../core/include/types.h"

struct Configuration {
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
    bool languages[KOTOBA_LANG_COUNT] = {false};

    // Dictionary
    int maxResults = 50;
    bool searchOnTyping = true;
    int searchDelayMs = 150;
    bool rememberLastQuery = true;
    bool highlightMatches = true;
    int searchPageSize = 20;

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

// QObject wrapper para exponerlo a QML
class ConfigWrapper : public QObject {
    Q_OBJECT

    Q_PROPERTY(QString theme READ theme WRITE setTheme NOTIFY themeChanged)
    Q_PROPERTY(QString primaryColor READ primaryColor WRITE setPrimaryColor NOTIFY primaryColorChanged)
    Q_PROPERTY(QString accentColor READ accentColor WRITE setAccentColor NOTIFY accentColorChanged)
    Q_PROPERTY(bool showFurigana READ showFurigana WRITE setShowFurigana NOTIFY showFuriganaChanged)
    Q_PROPERTY(int fontSize READ fontSize WRITE setFontSize NOTIFY fontSizeChanged)
    Q_PROPERTY(bool compactMode READ compactMode WRITE setCompactMode NOTIFY compactModeChanged)
    Q_PROPERTY(bool animations READ animations WRITE setAnimations NOTIFY animationsChanged)
    // puedes agregar más propiedades si QML las necesita

public:
    explicit ConfigWrapper(QObject* parent = nullptr) : QObject(parent) {}

    // getters
    QString theme() const { return m_config.theme; }
    QString primaryColor() const { return m_config.primaryColor; }
    QString accentColor() const { return m_config.accentColor; }
    bool showFurigana() const { return m_config.showFurigana; }
    int fontSize() const { return m_config.fontSize; }
    bool compactMode() const { return m_config.compactMode; }
    bool animations() const { return m_config.animations; }

    // setters
    void setTheme(const QString &v) { if (v != m_config.theme) { m_config.theme = v; emit themeChanged(); } }
    void setPrimaryColor(const QString &v) { if (v != m_config.primaryColor) { m_config.primaryColor = v; emit primaryColorChanged(); } }
    void setAccentColor(const QString &v) { if (v != m_config.accentColor) { m_config.accentColor = v; emit accentColorChanged(); } }
    void setShowFurigana(bool v) { if (v != m_config.showFurigana) { m_config.showFurigana = v; emit showFuriganaChanged(); } }
    void setFontSize(int v) { if (v != m_config.fontSize) { m_config.fontSize = v; emit fontSizeChanged(); } }
    void setCompactMode(bool v) { if (v != m_config.compactMode) { m_config.compactMode = v; emit compactModeChanged(); } }
    void setAnimations(bool v) { if (v != m_config.animations) { m_config.animations = v; emit animationsChanged(); } }

signals:
    void themeChanged();
    void primaryColorChanged();
    void accentColorChanged();
    void showFuriganaChanged();
    void fontSizeChanged();
    void compactModeChanged();
    void animationsChanged();

public:
    Configuration m_config;
};

void saveConfiguration(const Configuration &config, const QString &filePath);
void loadConfiguration(Configuration &config, const QString &filePath);

#endif // CONFIGURATION_H