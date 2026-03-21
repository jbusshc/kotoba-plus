#include "Configuration.h"

#include <QSettings>
#include <QFile>
#include <QStringList>
#include <QRegularExpression>
#include <random>

// ─────────────────────────────────────────
// device id
// ─────────────────────────────────────────

uint64_t generateDeviceId()
{
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dis;
    return dis(gen);
}

// ─────────────────────────────────────────
// parse "1m 10m 1h 2d"
// ─────────────────────────────────────────

QVector<uint32_t> parseSteps(const QString &steps)
{
    QVector<uint32_t> result;

    QStringList tokens = steps.split(" ", Qt::SkipEmptyParts);
    QRegularExpression re(R"((\d+)([smhd]))");

    for (const QString &t : tokens)
    {
        auto match = re.match(t.trimmed());
        if (!match.hasMatch()) continue;

        int value = match.captured(1).toInt();
        QString unit = match.captured(2);

        uint32_t seconds = 0;

        if (unit == "s") seconds = value;
        else if (unit == "m") seconds = value * 60;
        else if (unit == "h") seconds = value * 3600;
        else if (unit == "d") seconds = value * 86400;

        if (seconds > 0)
            result.push_back(seconds);
    }

    return result;
}

static void parseGlossLanguages(const QString &glossLangs, bool *langArray)
{
    memset(langArray, 0, KOTOBA_LANG_COUNT); // limpiar array
    QStringList langs = glossLangs.split(",", Qt::SkipEmptyParts);
    // debug print
    qDebug() << "Parsing gloss languages:" << glossLangs;
    qDebug() << "Split into:" << langs;
    for (const QString &l : langs)
    {       
        int8_t langId = str_to_lang(l.trimmed().toUtf8().constData());
        qDebug() << "Language:" << l.trimmed() << "-> ID:" << langId;
        if (langId >= 0)
            langArray[langId] = true;
    }
}

// ─────────────────────────────────────────
// validation
// ─────────────────────────────────────────

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
}

// ─────────────────────────────────────────
// save
// ─────────────────────────────────────────

void saveConfiguration(const Configuration &c, const QString &filePath)
{
    QSettings s(filePath, QSettings::IniFormat);

    s.beginGroup("App");
    s.setValue("app_version", c.appVersion);
    s.setValue("auto_save", c.autoSave);
    s.setValue("check_updates", c.checkUpdates);
    s.endGroup();

    s.beginGroup("Sync");
    s.setValue("device_id", static_cast<qulonglong>(c.deviceId));
    s.setValue("backup_enabled", c.backupEnabled);
    s.setValue("backup_interval_days", c.backupIntervalDays);
    s.endGroup();

    s.beginGroup("Data");
    s.setValue("dict_index_path", c.dictIndexPath);
    s.setValue("dict_path", c.dictPath);
    s.setValue("gloss_en_path", c.glossEnPath);
    s.setValue("gloss_es_path", c.glossEsPath);
    s.setValue("jp_path", c.jpPath);
    s.setValue("srs_path", c.srsPath);
    s.endGroup();

    s.beginGroup("Dictionary");
    s.setValue("highlight_matches", c.highlightMatches);
    s.setValue("max_results", c.maxResults);
    s.setValue("page_size", c.pageSize);
    s.setValue("search_delay_ms", c.searchDelayMs);
    s.setValue("search_on_typing", c.searchOnTyping);
    s.endGroup();

    s.beginGroup("Language");
    s.setValue("fallback_language", c.fallbackLanguage);
    s.setValue("gloss_languages", c.glossLanguages);
    s.setValue("interface", c.interface);
    s.endGroup();

    s.beginGroup("Session");
    s.setValue("daily_new_cards", c.dailyNewCards);
    s.setValue("daily_review_limit", c.dailyReviewLimit);
    s.endGroup();

    s.beginGroup("FSRS");
    s.setValue("desiredRetention", c.desiredRetention);
    s.setValue("maximumInterval", c.maximumInterval);
    s.setValue("newCardsPerDay", c.newCardsPerDay);
    s.setValue("reviewsPerDay", c.reviewsPerDay);
    s.setValue("leechThreshold", c.leechThreshold);
    s.setValue("enableFuzz", c.enableFuzz);
    s.setValue("learningSteps", c.learningSteps);
    s.setValue("relearningSteps", c.relearningSteps);
    s.setValue("dayOffset", c.dayOffset);
    s.endGroup();

    s.beginGroup("UI");
    s.setValue("accent_color", c.accentColor);
    s.setValue("primary_color", c.primaryColor);
    s.setValue("theme", c.theme);
    s.setValue("font_family", c.fontFamily);
    s.setValue("fontScale", c.fontScale);
    s.setValue("show_furigana", c.showFurigana);
    s.endGroup();

    s.sync();
}

// ─────────────────────────────────────────
// load
// ─────────────────────────────────────────

bool loadConfiguration(Configuration &c, const QString &filePath)
{
    if (!QFile::exists(filePath)) {
        c.deviceId = generateDeviceId();
        saveConfiguration(c, filePath);
        return false;
    }

    QSettings s(filePath, QSettings::IniFormat);

    s.beginGroup("App");
    c.appVersion = s.value("app_version", c.appVersion).toString();
    c.autoSave = s.value("auto_save", c.autoSave).toBool();
    c.checkUpdates = s.value("check_updates", c.checkUpdates).toBool();
    s.endGroup();

    s.beginGroup("Sync");
    c.deviceId = s.value("device_id", (qulonglong)c.deviceId).toULongLong();
    c.backupEnabled = s.value("backup_enabled", c.backupEnabled).toBool();
    c.backupIntervalDays = s.value("backup_interval_days", c.backupIntervalDays).toInt();
    s.endGroup();

    s.beginGroup("Data");
    c.dictIndexPath = s.value("dict_index_path", c.dictIndexPath).toString();
    c.dictPath = s.value("dict_path", c.dictPath).toString();
    c.glossEnPath = s.value("gloss_en_path", c.glossEnPath).toString();
    c.glossEsPath = s.value("gloss_es_path", c.glossEsPath).toString();
    c.jpPath = s.value("jp_path", c.jpPath).toString();
    c.srsPath = s.value("srs_path", c.srsPath).toString();
    s.endGroup();

    s.beginGroup("Dictionary");
    c.highlightMatches = s.value("highlight_matches", c.highlightMatches).toBool();
    c.maxResults = s.value("max_results", c.maxResults).toInt();
    c.pageSize = s.value("page_size", c.pageSize).toInt();
    c.searchDelayMs = s.value("search_delay_ms", c.searchDelayMs).toInt();
    c.searchOnTyping = s.value("search_on_typing", c.searchOnTyping).toBool();
    s.endGroup();

    s.beginGroup("Language");
    c.fallbackLanguage = s.value("fallback_language", c.fallbackLanguage).toString();
    c.glossLanguages = s.value("gloss_languages", c.glossLanguages).toString();
    c.interface = s.value("interface", c.interface).toString();
    s.endGroup();

    s.beginGroup("Session");
    c.dailyNewCards = s.value("daily_new_cards", c.dailyNewCards).toInt();
    c.dailyReviewLimit = s.value("daily_review_limit", c.dailyReviewLimit).toInt();
    s.endGroup();

    s.beginGroup("FSRS");
    c.desiredRetention = s.value("desiredRetention", c.desiredRetention).toDouble();
    c.maximumInterval = s.value("maximumInterval", c.maximumInterval).toInt();
    c.newCardsPerDay = s.value("newCardsPerDay", c.newCardsPerDay).toInt();
    c.reviewsPerDay = s.value("reviewsPerDay", c.reviewsPerDay).toInt();
    c.leechThreshold = s.value("leechThreshold", c.leechThreshold).toInt();
    c.enableFuzz = s.value("enableFuzz", c.enableFuzz).toBool();
    c.learningSteps = s.value("learningSteps", c.learningSteps).toString();
    c.relearningSteps = s.value("relearningSteps", c.relearningSteps).toString();
    c.dayOffset = s.value("dayOffset", c.dayOffset).toInt();
    s.endGroup();

    s.beginGroup("UI");
    c.accentColor = s.value("accent_color", c.accentColor).toString();
    c.primaryColor = s.value("primary_color", c.primaryColor).toString();
    c.theme = s.value("theme", c.theme).toString();
    c.fontFamily = s.value("font_family", c.fontFamily).toString();
    c.fontScale = s.value("fontScale", c.fontScale).toDouble();
    c.showFurigana = s.value("show_furigana", c.showFurigana).toBool();
    s.endGroup();

    // ── VALIDACIÓN FINAL ──
    validateConfig(c);

    // ── parse gloss languages ──
    parseGlossLanguages(c.glossLanguages, c.languages);

    // ── cache steps ──
    c.learningStepsParsed = parseSteps(c.learningSteps);
    c.relearningStepsParsed = parseSteps(c.relearningSteps);

    return true;
}