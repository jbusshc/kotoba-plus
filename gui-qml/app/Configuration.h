#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <QObject>
#include <QString>
#include <QVector>
#include <stdint.h>

#include <types.h>

class SearchService;
class SrsService;

// ─────────────────────────────────────────────────────────────
// Core config struct
//
// NOTE ON DATA PATHS:
//   dictPath, dictIndexPath, srsPath, gloss*Path, jpPath are NOT
//   read from or written to config.ini. They are always set at
//   startup by AppPaths::resolve() in main.cpp, so they correctly
//   point to writable storage on both desktop and Android.
// ─────────────────────────────────────────────────────────────

struct Configuration {

    // LANG array for search (filled when config is loaded)
    bool languages[KOTOBA_LANG_COUNT];

    // ---------------- App ----------------
    QString appVersion = "1.0";
    bool autoSave      = true;
    bool checkUpdates  = false;
    bool firstRun      = true;

    // ---------------- Sync ----------------
    uint64_t deviceId       = 0;
    bool     backupEnabled  = true;
    int      backupIntervalDays = 7;

    // ---------------- Data (set by AppPaths, not config.ini) ----------------
    QString dictIndexPath = "";
    QString dictPath      = "";
    QString glossEnPath   = "";
    QString glossEsPath   = "";
    QString glossDePath   = "";
    QString glossFrPath   = "";
    QString glossHuPath   = "";
    QString glossNlPath   = "";
    QString glossRuPath   = "";
    QString glossSlvPath  = "";
    QString glossSvPath   = "";
    QString jpPath        = "";
    QString srsPath       = "";

    // ---------------- Dictionary ----------------
    bool highlightMatches = true;
    int  maxResults       = 0;
    int  pageSize         = 20;
    int  searchDelayMs    = 150;
    bool searchOnTyping   = true;
    bool showRomaji       = false;

    // ---------------- Language ----------------
    QString fallbackLanguage = "en";
    QString glossLanguages   = "en";
    QString interface        = "en";   // UI language

    // ---------------- Session ----------------
    int dailyNewCards    = 20;
    int dailyReviewLimit = 200;

    // ---------------- FSRS ----------------
    double  desiredRetention = 0.90;
    int     maximumInterval  = 36500;
    int     newCardsPerDay   = 20;
    int     reviewsPerDay    = 200;
    int     leechThreshold   = 8;
    bool    enableFuzz       = true;
    QString learningSteps    = "1m 10m";
    QString relearningSteps  = "10m";
    int     dayOffset        = 14400;
    int     orderMode        = 0;  // 0=MIXED, 1=REVIEW_FIRST, 2=NEW_FIRST

    // ---------------- UI ----------------
    QString accentColor  = "blue";
    QString primaryColor = "indigo";
    QString theme        = "dark";
    QString fontFamily   = "default";
    double  fontScale    = 1.0;
    bool    showFurigana = true;

    // ── parsed caches ────────────────────────────────────────────────────────
    QVector<uint32_t> learningStepsParsed;
    QVector<uint32_t> relearningStepsParsed;
};


// ─────────────────────────────────────────────────────────────
// QML wrapper
// ─────────────────────────────────────────────────────────────

class ConfigWrapper : public QObject
{
    Q_OBJECT

    // ── UI ──────────────────────────────────────────────────────────────────
    Q_PROPERTY(QString theme             READ theme             WRITE setTheme             NOTIFY themeChanged)
    Q_PROPERTY(QString accentColor       READ accentColor       WRITE setAccentColor       NOTIFY accentColorChanged)
    Q_PROPERTY(QString primaryColor      READ primaryColor      WRITE setPrimaryColor      NOTIFY primaryColorChanged)
    Q_PROPERTY(double  fontScale         READ fontScale         WRITE setFontScale         NOTIFY fontScaleChanged)
    Q_PROPERTY(QString fontFamily        READ fontFamily        WRITE setFontFamily        NOTIFY fontFamilyChanged)
    Q_PROPERTY(bool    showFurigana      READ showFurigana      WRITE setShowFurigana      NOTIFY showFuriganaChanged)

    // ── Dictionary ──────────────────────────────────────────────────────────
    Q_PROPERTY(int     searchDelayMs     READ searchDelayMs     WRITE setSearchDelayMs     NOTIFY searchDelayMsChanged)
    Q_PROPERTY(bool    searchOnTyping    READ searchOnTyping    WRITE setSearchOnTyping    NOTIFY searchOnTypingChanged)
    Q_PROPERTY(bool    showRomaji        READ showRomaji        WRITE setShowRomaji        NOTIFY showRomajiChanged)

    // ── Language ────────────────────────────────────────────────────────────
    Q_PROPERTY(QString glossLanguages    READ glossLanguages    WRITE setGlossLanguages    NOTIFY glossLanguagesChanged)
    Q_PROPERTY(QString fallbackLanguage  READ fallbackLanguage  WRITE setFallbackLanguage  NOTIFY fallbackLanguageChanged)
    Q_PROPERTY(QString interfaceLanguage READ interfaceLanguage WRITE setInterfaceLanguage NOTIFY interfaceLanguageChanged)

    // ── FSRS ────────────────────────────────────────────────────────────────
    Q_PROPERTY(int     newCardsPerDay    READ newCardsPerDay    WRITE setNewCardsPerDay    NOTIFY newCardsPerDayChanged)
    Q_PROPERTY(int     reviewsPerDay     READ reviewsPerDay     WRITE setReviewsPerDay     NOTIFY reviewsPerDayChanged)
    Q_PROPERTY(double  desiredRetention  READ desiredRetention  WRITE setDesiredRetention  NOTIFY desiredRetentionChanged)
    Q_PROPERTY(bool    enableFuzz        READ enableFuzz        WRITE setEnableFuzz        NOTIFY enableFuzzChanged)
    Q_PROPERTY(int     leechThreshold    READ leechThreshold    WRITE setLeechThreshold    NOTIFY leechThresholdChanged)
    Q_PROPERTY(int     maximumInterval   READ maximumInterval   WRITE setMaximumInterval   NOTIFY maximumIntervalChanged)
    Q_PROPERTY(int     dayOffset         READ dayOffset         WRITE setDayOffset         NOTIFY dayOffsetChanged)
    Q_PROPERTY(int     orderMode         READ orderMode         WRITE setOrderMode         NOTIFY orderModeChanged)

    // ── Read-only ────────────────────────────────────────────────────────────
    Q_PROPERTY(QString appVersion  CONSTANT READ appVersion)
    Q_PROPERTY(quint64 deviceId    CONSTANT READ deviceId)
    Q_PROPERTY(bool    firstRun    CONSTANT READ firstRun)

public:
    explicit ConfigWrapper(QObject *parent = nullptr) : QObject(parent)
    {
        memset(m_config.languages, 0, sizeof(m_config.languages));
        m_config.languages[KOTOBA_LANG_EN] = true;
    }

    Configuration m_config;
    QString       m_configPath;

    Q_INVOKABLE void saveToDisk();
    Q_INVOKABLE void reloadFromDisk();
    Q_INVOKABLE void applyToServices();

    // ── Getters ──────────────────────────────────────────────────────────────
    QString theme()              const { return m_config.theme; }
    QString accentColor()        const { return m_config.accentColor; }
    QString primaryColor()       const { return m_config.primaryColor; }
    double  fontScale()          const { return m_config.fontScale; }
    QString fontFamily()         const { return m_config.fontFamily; }
    bool    showFurigana()       const { return m_config.showFurigana; }
    int     searchDelayMs()      const { return m_config.searchDelayMs; }
    bool    searchOnTyping()     const { return m_config.searchOnTyping; }
    QString glossLanguages()     const { return m_config.glossLanguages; }
    QString fallbackLanguage()   const { return m_config.fallbackLanguage; }
    QString interfaceLanguage()  const { return m_config.interface; }
    int     newCardsPerDay()     const { return m_config.newCardsPerDay; }
    int     reviewsPerDay()      const { return m_config.reviewsPerDay; }
    double  desiredRetention()   const { return m_config.desiredRetention; }
    bool    enableFuzz()         const { return m_config.enableFuzz; }
    int     leechThreshold()     const { return m_config.leechThreshold; }
    int     maximumInterval()    const { return m_config.maximumInterval; }
    int     dayOffset()          const { return m_config.dayOffset; }
    QString appVersion()         const { return m_config.appVersion; }
    quint64 deviceId()           const { return static_cast<quint64>(m_config.deviceId); }
    bool    firstRun()           const { return m_config.firstRun; }
    bool    showRomaji()         const { return m_config.showRomaji; }
    int     orderMode()          const { return m_config.orderMode; }

    // ── Setters ──────────────────────────────────────────────────────────────
    void setTheme(const QString &v)             { if (v != m_config.theme)            { m_config.theme            = v; emit themeChanged(); } }
    void setAccentColor(const QString &v)       { if (v != m_config.accentColor)      { m_config.accentColor      = v; emit accentColorChanged(); } }
    void setPrimaryColor(const QString &v)      { if (v != m_config.primaryColor)     { m_config.primaryColor     = v; emit primaryColorChanged(); } }
    void setFontScale(double v)                 { if (v != m_config.fontScale)        { m_config.fontScale        = v; emit fontScaleChanged(); } }
    void setFontFamily(const QString &v)        { if (v != m_config.fontFamily)       { m_config.fontFamily       = v; emit fontFamilyChanged(); } }
    void setShowFurigana(bool v)                { if (v != m_config.showFurigana)     { m_config.showFurigana     = v; emit showFuriganaChanged(); } }
    void setSearchDelayMs(int v)                { if (v != m_config.searchDelayMs)    { m_config.searchDelayMs    = v; emit searchDelayMsChanged(); } }
    void setSearchOnTyping(bool v)              { if (v != m_config.searchOnTyping)   { m_config.searchOnTyping   = v; emit searchOnTypingChanged(); } }
    void setGlossLanguages(const QString &v)    { if (v != m_config.glossLanguages)   { m_config.glossLanguages   = v; emit glossLanguagesChanged(); } }
    void setFallbackLanguage(const QString &v)  { if (v != m_config.fallbackLanguage) { m_config.fallbackLanguage = v; emit fallbackLanguageChanged(); } }
    void setInterfaceLanguage(const QString &v) { if (v != m_config.interface)        { m_config.interface        = v; emit interfaceLanguageChanged(); } }
    void setNewCardsPerDay(int v)               { if (v != m_config.newCardsPerDay)   { m_config.newCardsPerDay   = v; emit newCardsPerDayChanged(); } }
    void setReviewsPerDay(int v)                { if (v != m_config.reviewsPerDay)    { m_config.reviewsPerDay    = v; emit reviewsPerDayChanged(); } }
    void setDesiredRetention(double v)          { if (v != m_config.desiredRetention) { m_config.desiredRetention = v; emit desiredRetentionChanged(); } }
    void setEnableFuzz(bool v)                  { if (v != m_config.enableFuzz)       { m_config.enableFuzz       = v; emit enableFuzzChanged(); } }
    void setLeechThreshold(int v)               { if (v != m_config.leechThreshold)   { m_config.leechThreshold   = v; emit leechThresholdChanged(); } }
    void setMaximumInterval(int v)              { if (v != m_config.maximumInterval)  { m_config.maximumInterval  = v; emit maximumIntervalChanged(); } }
    void setDayOffset(int v)                    { if (v != m_config.dayOffset)        { m_config.dayOffset        = v; emit dayOffsetChanged(); } }
    void setFirstRun(bool v)                    { if (v != m_config.firstRun)         { m_config.firstRun         = v; emit firstRunChanged(); } }
    void setShowRomaji(bool v)                  { if (v != m_config.showRomaji)       { m_config.showRomaji       = v; emit showRomajiChanged(); } }
    void setOrderMode(int v)                    { if (v != m_config.orderMode)        { m_config.orderMode        = v; emit orderModeChanged(); } }

    void setServices(SearchService *searchSvc, SrsService *srsSvc)
    {
        m_searchService = searchSvc;
        m_srsService    = srsSvc;
    }

signals:
    void themeChanged();
    void accentColorChanged();
    void primaryColorChanged();
    void fontScaleChanged();
    void fontFamilyChanged();
    void showFuriganaChanged();
    void searchDelayMsChanged();
    void searchOnTypingChanged();
    void glossLanguagesChanged();
    void fallbackLanguageChanged();
    void interfaceLanguageChanged();
    void newCardsPerDayChanged();
    void reviewsPerDayChanged();
    void desiredRetentionChanged();
    void enableFuzzChanged();
    void leechThresholdChanged();
    void maximumIntervalChanged();
    void dayOffsetChanged();
    void firstRunChanged();
    void showRomajiChanged();
    void orderModeChanged();

private:
    SearchService *m_searchService = nullptr;
    SrsService    *m_srsService    = nullptr;
};


// ─────────────────────────────────────────────────────────────
// API
// ─────────────────────────────────────────────────────────────

void saveConfiguration(const Configuration &config, const QString &filePath);
bool loadConfiguration(Configuration &config, const QString &filePath);
uint64_t generateDeviceId();
QVector<uint32_t> parseSteps(const QString &steps);

#endif // CONFIGURATION_H
