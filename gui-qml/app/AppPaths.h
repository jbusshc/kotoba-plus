#pragma once

#include <QString>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QCoreApplication>
#include <QDebug>

// ─────────────────────────────────────────────────────────────────────────────
// AppPaths — single source of truth for all file paths across platforms.
//
// Desktop (Linux / macOS / Windows):
//   Data files (dict, gloss) live next to the executable.
//   Writable files (config, SRS profile) live in AppDataLocation.
//
// Android:
//   Data files are bundled as Qt assets (:/assets/data/) and must be
//   extracted to AppDataLocation on first run because the C library
//   (kotoba / fsrs) opens them with fopen(), which cannot read from
//   the APK asset store.
//   Writable files (config, SRS profile) also live in AppDataLocation,
//   which maps to the app's internal storage on Android.
// ─────────────────────────────────────────────────────────────────────────────

struct AppPaths
{
    QString dataDir;        // writable directory for all runtime files
    QString configPath;     // config.ini
    QString dictPath;       // dict.kotoba
    QString dictIndexPath;  // dict.kotoba.idx
    QString srsPath;        // profile.srs

    // Returns the path for a gloss/index file by language code.
    // lang: "en", "es", "de", "fr", "hu", "nl", "ru", "slv", "sv", "jp"
    QString glossPath(const QString &lang) const
    {
        if (lang == "jp")
            return QDir(dataDir).filePath("jp.invx");
        return QDir(dataDir).filePath(QString("gloss_%1.invx").arg(lang));
    }

    // ── Factory ──────────────────────────────────────────────────────────────
    static AppPaths resolve()
    {
        AppPaths p;

#ifdef Q_OS_ANDROID
        // On Android, AppDataLocation → /data/data/<package>/files
        // This is always writable and persists across launches.
        p.dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
#else
        // Desktop: data files sit next to the executable.
        // Config and SRS profile go to AppDataLocation so they survive
        // reinstalls and live outside the install directory.
        QString exeDir = QCoreApplication::applicationDirPath();

        // Prefer exe dir for data files (they are read-only bundled assets).
        // Fall back to cwd for development builds where there is no install.
        p.dataDir = exeDir;
        if (!QFile::exists(QDir(exeDir).filePath("dict.kotoba")))
            p.dataDir = QDir::currentPath();
#endif

        // Ensure the writable directory exists.
        QDir().mkpath(p.dataDir);

        p.configPath   = QDir(p.dataDir).filePath("config.ini");
        p.dictPath     = QDir(p.dataDir).filePath("dict.kotoba");
        p.dictIndexPath= QDir(p.dataDir).filePath("dict.kotoba.idx");
        p.srsPath      = QDir(p.dataDir).filePath("profile.srs");

        return p;
    }

#ifdef Q_OS_ANDROID
    // ── Asset extraction (Android only) ──────────────────────────────────────
    // Copies bundled assets from qrc:/assets/data/ to the writable dataDir
    // if they are not already present. Called once on startup.
    //
    // Expected asset layout in your .pro / CMakeLists:
    //   ANDROID_EXTRA_FILES (or qt_add_resources) should include:
    //     assets/data/dict.kotoba
    //     assets/data/dict.kotoba.idx
    //     assets/data/gloss_en.invx
    //     assets/data/gloss_es.invx
    //     ... etc.
    //     assets/data/jp.invx
    //
    // These are read-only inside the APK. We copy them out once.
    static void extractAssetsIfNeeded(const QString &targetDir)
    {
        static const QStringList kDataFiles = {
            "dict.kotoba",
            "dict.kotoba.idx",
            "gloss_en.invx",
            "gloss_es.invx",
            "gloss_de.invx",
            "gloss_fr.invx",
            "gloss_hu.invx",
            "gloss_nl.invx",
            "gloss_ru.invx",
            "gloss_slv.invx",
            "gloss_sv.invx",
            "jp.invx",
        };

        QDir().mkpath(targetDir);

        for (const QString &name : kDataFiles) {
            QString dest = QDir(targetDir).filePath(name);
            if (QFile::exists(dest)) {
                qDebug() << "Asset already extracted:" << name;
                continue;
            }

            QString src = QString("assets:/%1").arg(name);
            if (!QFile::exists(src)) {
                qWarning() << "Bundled asset not found:" << src;
                continue;
            }

            if (!QFile::copy(src, dest)) {
                qWarning() << "Failed to extract asset:" << src << "→" << dest;
            } else {
                // Qt copies preserve the read-only flag from the resource;
                // make it writable so the OS can manage it normally.
                QFile::setPermissions(dest,
                    QFile::ReadOwner | QFile::WriteOwner |
                    QFile::ReadGroup | QFile::ReadOther);
                qDebug() << "Extracted asset:" << name;
            }
        }
    }
#endif
};
