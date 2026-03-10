#include "Configuration.h"

#include <QSettings>
#include <QFile>
#include <QDebug>

#include <random>


uint64_t generateDeviceId()
{
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dis;
    return dis(gen);
}


void saveConfiguration(const Configuration &config, const QString &filePath)
{
    QSettings settings(filePath, QSettings::IniFormat);

    settings.beginGroup("App");
    settings.setValue("app_version", config.appVersion);
    settings.setValue("auto_save", config.autoSave);
    settings.setValue("check_updates", config.checkUpdates);
    settings.setValue("device_id", static_cast<qulonglong>(config.deviceId));
    settings.endGroup();


    settings.beginGroup("UI");
    settings.setValue("theme", config.theme);
    settings.setValue("primary_color", config.primaryColor);
    settings.setValue("accent_color", config.accentColor);
    settings.setValue("font_family", config.fontFamily);
    settings.setValue("font_size", config.fontSize);
    settings.setValue("show_furigana", config.showFurigana);
    settings.setValue("compact_mode", config.compactMode);
    settings.setValue("animations", config.animations);
    settings.endGroup();


    settings.beginGroup("Language");
    settings.setValue("interface", config.interface);
    settings.setValue("gloss_languages", config.glossLanguages);
    settings.setValue("fallback_language", config.fallbackLanguage);
    settings.endGroup();


    settings.beginGroup("Dictionary");
    settings.setValue("max_results", config.maxResults);
    settings.setValue("search_on_typing", config.searchOnTyping);
    settings.setValue("search_delay_ms", config.searchDelayMs);
    settings.setValue("remember_last_query", config.rememberLastQuery);
    settings.setValue("highlight_matches", config.highlightMatches);
    settings.setValue("page_size", config.searchPageSize);
    settings.endGroup();


    settings.beginGroup("SRS");
    settings.setValue("daily_new_cards", config.dailyNewCards);
    settings.setValue("daily_review_limit", config.dailyReviewLimit);
    settings.setValue("learning_steps", config.learningSteps);
    settings.setValue("graduating_interval", config.graduatingInterval);
    settings.setValue("easy_interval", config.easyInterval);
    settings.setValue("starting_ease", config.startingEase);
    settings.setValue("bury_related", config.buryRelated);
    settings.setValue("show_answer_auto", config.showAnswerAuto);
    settings.endGroup();


    settings.beginGroup("Audio");
    settings.setValue("enabled", config.audioEnabled);
    settings.setValue("volume", config.volume);
    settings.setValue("auto_play", config.autoPlay);
    settings.endGroup();


    settings.beginGroup("Data");
    settings.setValue("dict_path", config.dictPath);
    settings.setValue("dict_index_path", config.dictIndexPath);
    settings.setValue("srs_path", config.srsPath);
    settings.setValue("jp_path", config.jpPath);
    settings.setValue("gloss_en_path", config.glossEnPath);
    settings.setValue("gloss_es_path", config.glossEsPath);
    settings.setValue("backup_enabled", config.backupEnabled);
    settings.setValue("backup_interval_days", config.backupIntervalDays);
    settings.endGroup();

    settings.sync();
}



bool loadConfiguration(Configuration &config, const QString &filePath)
{
    if (!QFile::exists(filePath)) {

        config.deviceId = generateDeviceId();

        qDebug() << "Creating new config with device id:" << config.deviceId;

        saveConfiguration(config, filePath);
        return false;
    }

    QSettings settings(filePath, QSettings::IniFormat);


    settings.beginGroup("App");

    config.firstRun = false;

    config.appVersion =
            settings.value("app_version", config.appVersion).toString();

    config.autoSave =
            settings.value("auto_save", config.autoSave).toBool();

    config.checkUpdates =
            settings.value("check_updates", config.checkUpdates).toBool();

    config.deviceId =
        settings.value("device_id",
                    static_cast<qulonglong>(config.deviceId)).toULongLong();

    settings.endGroup();


    settings.beginGroup("Language");

    config.interface =
            settings.value("interface", config.interface).toString();

    config.glossLanguages =
            settings.value("gloss_languages", config.glossLanguages).toString();

    config.fallbackLanguage =
            settings.value("fallback_language", config.fallbackLanguage).toString();

    settings.endGroup();


    QStringList langs = config.glossLanguages.split(",", Qt::SkipEmptyParts);


    for (int i = 0; i < KOTOBA_LANG_COUNT; ++i)
        config.languages[i] = false;


    for (QString lang : langs)
    {
        lang = lang.trimmed();

        if (lang == "en") config.languages[KOTOBA_LANG_EN] = true;
        else if (lang == "es") config.languages[KOTOBA_LANG_ES] = true;
        else if (lang == "fr") config.languages[KOTOBA_LANG_FR] = true;
        else if (lang == "de") config.languages[KOTOBA_LANG_DE] = true;
        else if (lang == "ru") config.languages[KOTOBA_LANG_RU] = true;
    }


    bool any = false;

    for (int i = 0; i < KOTOBA_LANG_COUNT; ++i)
        if (config.languages[i])
            any = true;

    if (!any)
        config.languages[KOTOBA_LANG_EN] = true;


    return true;
}