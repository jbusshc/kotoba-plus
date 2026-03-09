#include "Configuration.h"
#include <QSettings>
#include <QFile>
#include <QStandardPaths>
#include <QDir>



void saveConfiguration(const Configuration &config, const QString &filePath)
{
    QSettings settings(filePath, QSettings::IniFormat);
    
    // General
    settings.beginGroup("General");
    settings.setValue("first_run", config.firstRun);
    settings.setValue("app_version", config.appVersion);
    settings.setValue("auto_save", config.autoSave);
    settings.setValue("check_updates", config.checkUpdates);
    settings.endGroup();
    
    // UI
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
    
    // Language
    settings.beginGroup("Language");
    settings.setValue("interface", config.interface);
    settings.setValue("gloss_languages", config.glossLanguages);
    settings.setValue("fallback_language", config.fallbackLanguage);
    settings.endGroup();
    
    // Dictionary
    settings.beginGroup("Dictionary");
    settings.setValue("max_results", config.maxResults);
    settings.setValue("search_on_typing", config.searchOnTyping);
    settings.setValue("search_delay_ms", config.searchDelayMs);
    settings.setValue("remember_last_query", config.rememberLastQuery);
    settings.setValue("highlight_matches", config.highlightMatches);
    settings.setValue("page_size", config.searchPageSize);
    settings.endGroup();
    
    // SRS
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
    
    // Audio
    settings.beginGroup("Audio");
    settings.setValue("enabled", config.audioEnabled);
    settings.setValue("volume", config.volume);
    settings.setValue("auto_play", config.autoPlay);
    settings.endGroup();
    
    // Data
    settings.beginGroup("Data");
    settings.setValue("dict_path", config.dictPath);
    settings.setValue("dict_index_path", config.dictIndexPath);
    settings.setValue("srs_path", config.srsPath);
    settings.setValue("srs_event_log_path", config.srsEventLogPath);
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
        saveConfiguration(config, filePath);
        return false;
    }

    QSettings settings(filePath, QSettings::IniFormat);

    // ----------------- General -----------------
    settings.beginGroup("General");
    config.firstRun = false; // si el archivo existe, no es la primera ejecución
    config.appVersion = settings.value("app_version", config.appVersion).toString();
    config.autoSave = settings.value("auto_save", config.autoSave).toBool();
    config.checkUpdates = settings.value("check_updates", config.checkUpdates).toBool();
    settings.endGroup();

    // ----------------- UI -----------------
    settings.beginGroup("UI");
    config.theme = settings.value("theme", config.theme).toString();
    config.primaryColor = settings.value("primary_color", config.primaryColor).toString();
    config.accentColor = settings.value("accent_color", config.accentColor).toString();
    config.fontFamily = settings.value("font_family", config.fontFamily).toString();
    config.fontSize = settings.value("font_size", config.fontSize).toInt();
    config.showFurigana = settings.value("show_furigana", config.showFurigana).toBool();
    config.compactMode = settings.value("compact_mode", config.compactMode).toBool();
    config.animations = settings.value("animations", config.animations).toBool();
    settings.endGroup();

    // ----------------- Language -----------------
    settings.beginGroup("Language");
    config.interface = settings.value("interface", config.interface).toString();
    config.fallbackLanguage = settings.value("fallback_language", config.fallbackLanguage).toString();
    QStringList langs = settings.value("gloss_languages", config.glossLanguages).toStringList();
    settings.endGroup();

    // ----------------- Dictionary -----------------
    settings.beginGroup("Dictionary");
    config.maxResults = settings.value("max_results", config.maxResults).toInt();
    config.searchOnTyping = settings.value("search_on_typing", config.searchOnTyping).toBool();
    config.searchDelayMs = settings.value("search_delay_ms", config.searchDelayMs).toInt();
    config.rememberLastQuery = settings.value("remember_last_query", config.rememberLastQuery).toBool();
    config.highlightMatches = settings.value("highlight_matches", config.highlightMatches).toBool();
    config.searchPageSize = settings.value("page_size", config.searchPageSize).toInt();
    settings.endGroup();

    // ----------------- SRS -----------------
    settings.beginGroup("SRS");
    config.dailyNewCards = settings.value("daily_new_cards", config.dailyNewCards).toInt();
    config.dailyReviewLimit = settings.value("daily_review_limit", config.dailyReviewLimit).toInt();
    config.learningSteps = settings.value("learning_steps", config.learningSteps).toString();
    config.graduatingInterval = settings.value("graduating_interval", config.graduatingInterval).toInt();
    config.easyInterval = settings.value("easy_interval", config.easyInterval).toInt();
    config.startingEase = settings.value("starting_ease", config.startingEase).toDouble();
    config.buryRelated = settings.value("bury_related", config.buryRelated).toBool();
    config.showAnswerAuto = settings.value("show_answer_auto", config.showAnswerAuto).toBool();
    settings.endGroup();

    // ----------------- Audio -----------------
    settings.beginGroup("Audio");
    config.audioEnabled = settings.value("enabled", config.audioEnabled).toBool();
    config.volume = settings.value("volume", config.volume).toInt();
    config.autoPlay = settings.value("auto_play", config.autoPlay).toBool();
    settings.endGroup();

    // ----------------- Data -----------------
    settings.beginGroup("Data");
    config.dictPath = settings.value("dict_path", config.dictPath).toString();
    config.dictIndexPath = settings.value("dict_index_path", config.dictIndexPath).toString();
    config.srsPath = settings.value("srs_path", config.srsPath).toString();
    config.srsEventLogPath = settings.value("srs_event_log_path", config.srsEventLogPath).toString();
    config.jpPath = settings.value("jp_path", config.jpPath).toString();
    config.glossEnPath = settings.value("gloss_en_path", config.glossEnPath).toString();
    config.glossEsPath = settings.value("gloss_es_path", config.glossEsPath).toString();
    config.backupEnabled = settings.value("backup_enabled", config.backupEnabled).toBool();
    config.backupIntervalDays = settings.value("backup_interval_days", config.backupIntervalDays).toInt();
    settings.endGroup();

    // ----------------- Procesar idiomas -----------------
    // Limpia todos los idiomas
    for (int i = 0; i < KOTOBA_LANG_COUNT; ++i)
        config.languages[i] = false;

    printf("Loaded config: interface=%s, gloss_languages=%s\n", config.interface.toStdString().c_str(), config.glossLanguages.toStdString().c_str());

    // Separar la lista de idiomas y limpiar espacios
    for (QString &lang : langs) {
        lang = lang.trimmed();
        if (lang == "en") config.languages[KOTOBA_LANG_EN] = true;
        else if (lang == "es") config.languages[KOTOBA_LANG_ES] = true;
        else if (lang == "fr") config.languages[KOTOBA_LANG_FR] = true;
        else if (lang == "de") config.languages[KOTOBA_LANG_DE] = true;
        else if (lang == "ru") config.languages[KOTOBA_LANG_RU] = true;
        else if (lang == "pt") config.languages[KOTOBA_LANG_PT] = true;
        else if (lang == "it") config.languages[KOTOBA_LANG_IT] = true;
        else if (lang == "nl") config.languages[KOTOBA_LANG_NL] = true;
        else if (lang == "hu") config.languages[KOTOBA_LANG_HU] = true;
        else if (lang == "sv") config.languages[KOTOBA_LANG_SV] = true;
        else if (lang == "cs") config.languages[KOTOBA_LANG_CS] = true;
        else if (lang == "pl") config.languages[KOTOBA_LANG_PL] = true;
        else if (lang == "ro") config.languages[KOTOBA_LANG_RO] = true;
        else if (lang == "he") config.languages[KOTOBA_LANG_HE] = true;
        else if (lang == "ar") config.languages[KOTOBA_LANG_AR] = true;
        else if (lang == "tr") config.languages[KOTOBA_LANG_TR] = true;
        else if (lang == "th") config.languages[KOTOBA_LANG_TH] = true;
        else if (lang == "vi") config.languages[KOTOBA_LANG_VI] = true;
        else if (lang == "id") config.languages[KOTOBA_LANG_ID] = true;
        else if (lang == "ms") config.languages[KOTOBA_LANG_MS] = true;
        else if (lang == "ko") config.languages[KOTOBA_LANG_KO] = true;
        else if (lang == "zh" || lang == "zh_cn" || lang == "zh_tw") config.languages[KOTOBA_LANG_ZH] = true;
        else if (lang == "fa") config.languages[KOTOBA_LANG_FA] = true;
        else if (lang == "eo") config.languages[KOTOBA_LANG_EO] = true;
        else if (lang == "slv") config.languages[KOTOBA_LANG_SLV] = true;
    }

    // Fallback seguro si no se activó ninguno
    bool anyActive = false;
    for (int i = 0; i < KOTOBA_LANG_COUNT; ++i)
        if (config.languages[i]) { anyActive = true; break; }
    if (!anyActive)
        config.languages[KOTOBA_LANG_EN] = true;

    return true;
}
