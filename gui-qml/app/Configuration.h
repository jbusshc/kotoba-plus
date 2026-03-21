#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <QObject>
#include <QString>
#include <QVector>
#include <stdint.h>

#include <types.h>

// ─────────────────────────────────────────────────────────────
// Core config
// ─────────────────────────────────────────────────────────────

struct Configuration {

    // ---------------- App ----------------
    QString appVersion = "1.0";
    bool autoSave = true;
    bool checkUpdates = false;

    // ---------------- Sync ----------------
    uint64_t deviceId = 0;
    bool backupEnabled = true;
    int backupIntervalDays = 7;

    // ---------------- Data ----------------
    QString dictIndexPath = "dict.kotoba.idx";
    QString dictPath = "dict.kotoba";
    QString glossEnPath = "gloss_en.invx";
    QString glossEsPath = "gloss_es.invx";
    QString jpPath = "jp.invx";
    QString srsPath = "profile.srs";

    // ---------------- Dictionary ----------------
    bool highlightMatches = true;
    int maxResults = 0;
    int pageSize = 20;
    int searchDelayMs = 150;
    bool searchOnTyping = true;

    // ---------------- Language ----------------
    QString fallbackLanguage = "en";
    QString glossLanguages = "es, en";
    QString interface = "en";

    // ---------------- Session ----------------
    int dailyNewCards = 20;
    int dailyReviewLimit = 200;

    // ---------------- FSRS ----------------
    double desiredRetention = 0.90;
    int maximumInterval = 36500;
    int newCardsPerDay = 20;
    int reviewsPerDay = 200;
    int leechThreshold = 8;
    bool enableFuzz = true;
    QString learningSteps = "1m 10m";
    QString relearningSteps = "10m";
    int dayOffset = 14400;

    // ---------------- UI ----------------
    QString accentColor = "blue";
    QString primaryColor = "indigo";
    QString theme = "dark";
    QString fontFamily = "default";
    double fontScale = 1.0;
    bool showFurigana = true;

    // ── cache parsed (opcional pero útil)
    QVector<uint32_t> learningStepsParsed;
    QVector<uint32_t> relearningStepsParsed;

    // LANG array for search (llenada al cargar config)
    bool languages[KOTOBA_LANG_COUNT];
};


// ─────────────────────────────────────────────────────────────
// QML wrapper (mínimo necesario)
// ─────────────────────────────────────────────────────────────

class ConfigWrapper : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString theme READ theme WRITE setTheme NOTIFY themeChanged)
    Q_PROPERTY(QString accentColor READ accentColor WRITE setAccentColor NOTIFY accentColorChanged)
    Q_PROPERTY(QString primaryColor READ primaryColor WRITE setPrimaryColor NOTIFY primaryColorChanged)
    Q_PROPERTY(double fontScale READ fontScale WRITE setFontScale NOTIFY fontScaleChanged)
    Q_PROPERTY(QString fontFamily READ fontFamily WRITE setFontFamily NOTIFY fontFamilyChanged)
    Q_PROPERTY(bool showFurigana READ showFurigana WRITE setShowFurigana NOTIFY showFuriganaChanged)

public:
    explicit ConfigWrapper(QObject *parent = nullptr) : QObject(parent) {}

    Configuration m_config;

    QString theme() const { return m_config.theme; }
    QString accentColor() const { return m_config.accentColor; }
    QString primaryColor() const { return m_config.primaryColor; }
    double fontScale() const { return m_config.fontScale; }
    bool showFurigana() const { return m_config.showFurigana; }
    QString fontFamily() const { return m_config.fontFamily; }
    void setTheme(const QString &v) {
        if (v != m_config.theme) { m_config.theme = v; emit themeChanged(); }
    }

    void setAccentColor(const QString &v) {
        if (v != m_config.accentColor) { m_config.accentColor = v; emit accentColorChanged(); }
    }

    void setPrimaryColor(const QString &v) {
        if (v != m_config.primaryColor) { m_config.primaryColor = v; emit primaryColorChanged(); }
    }

    void setFontScale(double v) {
        if (v != m_config.fontScale) { m_config.fontScale = v; emit fontScaleChanged(); }
    }

    void setShowFurigana(bool v) {
        if (v != m_config.showFurigana) { m_config.showFurigana = v; emit showFuriganaChanged(); }
    }
    void setFontFamily(const QString &v) {
        if (v != m_config.fontFamily) { m_config.fontFamily = v; emit fontFamilyChanged(); }
    }
signals:
    void themeChanged();
    void accentColorChanged();
    void primaryColorChanged();
    void fontScaleChanged();
    void showFuriganaChanged();
    void fontFamilyChanged();
};


// ─────────────────────────────────────────────────────────────
// API
// ─────────────────────────────────────────────────────────────

void saveConfiguration(const Configuration &config, const QString &filePath);
bool loadConfiguration(Configuration &config, const QString &filePath);
uint64_t generateDeviceId();

// helpers útiles para SRS
QVector<uint32_t> parseSteps(const QString &steps);

#endif