#include "Configuration.h"

#include <QSettings>
#include <QFile>
#include <QStringList>
#include <QRegularExpression>
#include <random>
#include <cmath>

#include "../infrastructure/SearchService.h"
#include "../infrastructure/SrsService.h"

// ─────────────────────────────────────────────────────────────────────────────
// device id
// ─────────────────────────────────────────────────────────────────────────────

uint64_t generateDeviceId()
{
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dis;
    return dis(gen);
}

// ─────────────────────────────────────────────────────────────────────────────
// parse "1m 10m 1h 2d"
// ─────────────────────────────────────────────────────────────────────────────

QVector<uint32_t> parseSteps(const QString &steps)
{
    QVector<uint32_t> result;
    QStringList tokens = steps.split(" ", Qt::SkipEmptyParts);
    QRegularExpression re(R"((\d+)([smhd]))");

    for (const QString &t : tokens) {
        auto match = re.match(t.trimmed());
        if (!match.hasMatch()) continue;

        int value = match.captured(1).toInt();
        QString unit = match.captured(2);

        uint32_t seconds = 0;
        if      (unit == "s") seconds = value;
        else if (unit == "m") seconds = value * 60;
        else if (unit == "h") seconds = value * 3600;
        else if (unit == "d") seconds = value * 86400;

        if (seconds > 0) result.push_back(seconds);
    }
    return result;
}

// ─────────────────────────────────────────────────────────────────────────────
// parse gloss languages
// ─────────────────────────────────────────────────────────────────────────────

static void parseGlossLanguages(const QString &glossLangs, bool *langArray)
{
    memset(langArray, 0, KOTOBA_LANG_COUNT);
    QStringList langs = glossLangs.split(",", Qt::SkipEmptyParts);
    for (const QString &l : langs) {
        int8_t langId = str_to_lang(l.trimmed().toUtf8().constData());
        if (langId >= 0)
            langArray[langId] = true;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// validation
// ─────────────────────────────────────────────────────────────────────────────

static void validateConfig(Configuration &c)
{
    if (c.desiredRetention < 0.7 || c.desiredRetention > 0.97)
        c.desiredRetention = 0.9;

    if (c.maximumInterval <= 0)
        c.maximumInterval = 36500;

    if (c.newCardsPerDay < 0)
        c.newCardsPerDay = 20;

    if (c.reviewsPerDay < 0)
        c.reviewsPerDay = 200;

    if (c.leechThreshold < 1)
        c.leechThreshold = 8;

    if (c.dayOffset < 0 || c.dayOffset >= 86400)
        c.dayOffset = 14400;

    if (parseSteps(c.learningSteps).isEmpty())
        c.learningSteps = "1m 10m";

    if (parseSteps(c.relearningSteps).isEmpty())
        c.relearningSteps = "10m";

    if (c.fontScale < 0.7 || c.fontScale > 2.0)
        c.fontScale = 1.0;

    if (c.maxResults < 1)
        c.maxResults = SEARCH_MAX_RESULTS_DEFAULT;
    else if (c.maxResults > SEARCH_MAX_RESULTS)
        c.maxResults = SEARCH_MAX_RESULTS_DEFAULT;

    if (c.pageSize < 1)
        c.pageSize = 20;
    else if (c.pageSize > 100)
        c.pageSize = 100;

    if (c.orderMode < 0 || c.orderMode > 2)
        c.orderMode = 0;

    // autoAddRecall es bool — no requiere validación de rango
}

// ─────────────────────────────────────────────────────────────────────────────
// migration helper
// ─────────────────────────────────────────────────────────────────────────────

static void migrateConfig(Configuration &c, int storedVersion)
{
    if (storedVersion < 1) {
        if (c.fontScale <= 0) c.fontScale = 1.0;
        if (c.accentColor.isEmpty()) c.accentColor = "blue";
    }
    // futuras migraciones se agregan aquí...
}

// ─────────────────────────────────────────────────────────────────────────────
// save
// ─────────────────────────────────────────────────────────────────────────────

void saveConfiguration(const Configuration &c, const QString &filePath)
{
    QSettings s(filePath, QSettings::IniFormat);

    s.beginGroup("Meta");
    s.setValue("config_version", 1);
    s.endGroup();

    s.beginGroup("App");
    s.setValue("app_version",   c.appVersion);
    s.setValue("auto_save",     c.autoSave);
    s.setValue("check_updates", c.checkUpdates);
    s.setValue("first_run",     c.firstRun);
    s.endGroup();

    s.beginGroup("Sync");
    s.setValue("device_id",            static_cast<qulonglong>(c.deviceId));
    s.setValue("backup_enabled",       c.backupEnabled);
    s.setValue("backup_interval_days", c.backupIntervalDays);
    s.endGroup();

    s.beginGroup("Dictionary");
    s.setValue("highlight_matches", c.highlightMatches);
    s.setValue("max_results",       c.maxResults);
    s.setValue("page_size",         c.pageSize);
    s.setValue("search_delay_ms",   c.searchDelayMs);
    s.setValue("search_on_typing",  c.searchOnTyping);
    s.setValue("show_romaji",       c.showRomaji);
    s.endGroup();

    s.beginGroup("Language");
    s.setValue("fallback_language", c.fallbackLanguage);
    s.setValue("gloss_languages",   c.glossLanguages);
    s.setValue("interface",         c.interface);
    s.endGroup();

    s.beginGroup("Session");
    s.setValue("daily_new_cards",    c.dailyNewCards);
    s.setValue("daily_review_limit", c.dailyReviewLimit);
    s.endGroup();

    s.beginGroup("FSRS");
    s.setValue("desiredRetention", std::round(c.desiredRetention * 10000.0) / 10000.0);
    s.setValue("maximumInterval",  c.maximumInterval);
    s.setValue("newCardsPerDay",   c.newCardsPerDay);
    s.setValue("reviewsPerDay",    c.reviewsPerDay);
    s.setValue("leechThreshold",   c.leechThreshold);
    s.setValue("enableFuzz",       c.enableFuzz);
    s.setValue("learningSteps",    c.learningSteps);
    s.setValue("relearningSteps",  c.relearningSteps);
    s.setValue("dayOffset",        c.dayOffset);
    s.setValue("order_mode",       c.orderMode);
    s.setValue("autoAddRecall",    c.autoAddRecall);
    s.endGroup();

    s.beginGroup("UI");
    s.setValue("accent_color",  c.accentColor);
    s.setValue("primary_color", c.primaryColor);
    s.setValue("theme",         c.theme);
    s.setValue("font_family",   c.fontFamily);
    s.setValue("fontScale",     std::round(c.fontScale * 100.0) / 100.0);
    s.endGroup();

    s.sync();
}

// ─────────────────────────────────────────────────────────────────────────────
// load
// ─────────────────────────────────────────────────────────────────────────────

bool loadConfiguration(Configuration &c, const QString &filePath)
{
    int storedVersion = 0;

    if (!QFile::exists(filePath)) {
        c.deviceId = generateDeviceId();
        saveConfiguration(c, filePath);
        return false;
    }

    QSettings s(filePath, QSettings::IniFormat);

    storedVersion = s.value("Meta/config_version", 0).toInt();
    migrateConfig(c, storedVersion);

    s.beginGroup("App");
    c.appVersion   = s.value("app_version",   c.appVersion).toString();
    c.autoSave     = s.value("auto_save",      c.autoSave).toBool();
    c.checkUpdates = s.value("check_updates",  c.checkUpdates).toBool();
    c.firstRun     = s.value("first_run",      c.firstRun).toBool();
    s.endGroup();

    s.beginGroup("Sync");
    c.deviceId           = s.value("device_id",           (qulonglong)c.deviceId).toULongLong();
    c.backupEnabled      = s.value("backup_enabled",       c.backupEnabled).toBool();
    c.backupIntervalDays = s.value("backup_interval_days", c.backupIntervalDays).toInt();
    s.endGroup();

    s.beginGroup("Dictionary");
    c.highlightMatches = s.value("highlight_matches", c.highlightMatches).toBool();
    c.maxResults       = s.value("max_results",       c.maxResults).toInt();
    c.pageSize         = s.value("page_size",         c.pageSize).toInt();
    c.searchDelayMs    = s.value("search_delay_ms",   c.searchDelayMs).toInt();
    c.searchOnTyping   = s.value("search_on_typing",  c.searchOnTyping).toBool();
    c.showRomaji       = s.value("show_romaji",       c.showRomaji).toBool();
    s.endGroup();

    s.beginGroup("Language");
    c.fallbackLanguage = s.value("fallback_language", c.fallbackLanguage).toString();
    c.glossLanguages   = s.value("gloss_languages",   c.glossLanguages).toString();
    c.interface        = s.value("interface",         c.interface).toString();
    s.endGroup();

    s.beginGroup("Session");
    c.dailyNewCards    = s.value("daily_new_cards",    c.dailyNewCards).toInt();
    c.dailyReviewLimit = s.value("daily_review_limit", c.dailyReviewLimit).toInt();
    s.endGroup();

    s.beginGroup("FSRS");
    c.desiredRetention = std::round(s.value("desiredRetention", c.desiredRetention).toDouble() * 10000.0) / 10000.0;
    c.maximumInterval  = s.value("maximumInterval",  c.maximumInterval).toInt();
    c.newCardsPerDay   = s.value("newCardsPerDay",   c.newCardsPerDay).toInt();
    c.reviewsPerDay    = s.value("reviewsPerDay",    c.reviewsPerDay).toInt();
    c.leechThreshold   = s.value("leechThreshold",   c.leechThreshold).toInt();
    c.enableFuzz       = s.value("enableFuzz",       c.enableFuzz).toBool();
    c.learningSteps    = s.value("learningSteps",    c.learningSteps).toString();
    c.relearningSteps  = s.value("relearningSteps",  c.relearningSteps).toString();
    c.dayOffset        = s.value("dayOffset",        c.dayOffset).toInt();
    c.orderMode        = s.value("order_mode",       c.orderMode).toInt();
    c.autoAddRecall    = s.value("autoAddRecall",    c.autoAddRecall).toBool();
    s.endGroup();

    s.beginGroup("UI");
    c.accentColor  = s.value("accent_color",  c.accentColor).toString();
    c.primaryColor = s.value("primary_color", c.primaryColor).toString();
    c.theme        = s.value("theme",         c.theme).toString();
    c.fontFamily   = s.value("font_family",   c.fontFamily).toString();
    c.fontScale    = std::round(s.value("fontScale", c.fontScale).toDouble() * 100.0) / 100.0;
    s.endGroup();

    validateConfig(c);
    parseGlossLanguages(c.glossLanguages, c.languages);

    c.learningStepsParsed   = parseSteps(c.learningSteps);
    c.relearningStepsParsed = parseSteps(c.relearningSteps);

    if (c.firstRun) c.firstRun = false;
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// ConfigWrapper (expose to QML)
// ─────────────────────────────────────────────────────────────────────────────

void ConfigWrapper::saveToDisk()
{
    if (!m_configPath.isEmpty())
        saveConfiguration(m_config, m_configPath);
}

void ConfigWrapper::reloadFromDisk()
{
    if (m_configPath.isEmpty()) return;

    QString savedDict      = m_config.dictPath;
    QString savedDictIdx   = m_config.dictIndexPath;
    QString savedGlossEn   = m_config.glossEnPath;
    QString savedGlossEs   = m_config.glossEsPath;
    QString savedGlossDe   = m_config.glossDePath;
    QString savedGlossFr   = m_config.glossFrPath;
    QString savedGlossHu   = m_config.glossHuPath;
    QString savedGlossNl   = m_config.glossNlPath;
    QString savedGlossRu   = m_config.glossRuPath;
    QString savedGlossSlv  = m_config.glossSlvPath;
    QString savedGlossSv   = m_config.glossSvPath;
    QString savedJp        = m_config.jpPath;

    loadConfiguration(m_config, m_configPath);

    m_config.dictPath      = savedDict;
    m_config.dictIndexPath = savedDictIdx;
    m_config.glossEnPath   = savedGlossEn;
    m_config.glossEsPath   = savedGlossEs;
    m_config.glossDePath   = savedGlossDe;
    m_config.glossFrPath   = savedGlossFr;
    m_config.glossHuPath   = savedGlossHu;
    m_config.glossNlPath   = savedGlossNl;
    m_config.glossRuPath   = savedGlossRu;
    m_config.glossSlvPath  = savedGlossSlv;
    m_config.glossSvPath   = savedGlossSv;
    m_config.jpPath        = savedJp;

    emit themeChanged();
    emit accentColorChanged();
    emit primaryColorChanged();
    emit fontScaleChanged();
    emit fontFamilyChanged();
    emit searchDelayMsChanged();
    emit searchOnTypingChanged();
    emit glossLanguagesChanged();
    emit fallbackLanguageChanged();
    emit interfaceLanguageChanged();
    emit newCardsPerDayChanged();
    emit reviewsPerDayChanged();
    emit desiredRetentionChanged();
    emit enableFuzzChanged();
    emit leechThresholdChanged();
    emit maximumIntervalChanged();
    emit dayOffsetChanged();
    emit firstRunChanged();
    emit showRomajiChanged();
    emit orderModeChanged();
    emit autoAddRecallChanged();
}

void ConfigWrapper::applyToServices()
{
    if (m_searchService) {
        parseGlossLanguages(m_config.glossLanguages, m_config.languages);
        m_searchService->updateConfig(&m_config);
    }
    if (m_srsService)
        m_srsService->updateConfig(&m_config);
}