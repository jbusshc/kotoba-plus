#pragma once

#include <QString>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QCoreApplication>
#include <QDebug>

// Bump this string whenever you ship new/changed asset files.
// Any device that has a different version cached will re-extract everything.
#define APP_ASSET_VERSION "1.0.0"

struct AppPaths
{
    QString dataDir;
    QString configPath;
    QString dictPath;
    QString dictIndexPath;
    QString srsPath;

    QString glossPath(const QString &lang) const
    {
        if (lang == "jp")
            return QDir(dataDir).filePath("jp.invx");
        return QDir(dataDir).filePath(QString("gloss_%1.invx").arg(lang));
    }

    static AppPaths resolve()
    {
        AppPaths p;

#ifdef Q_OS_ANDROID
        p.dataDir = QStandardPaths::writableLocation(
            QStandardPaths::AppDataLocation);
#else
        QString exeDir = QCoreApplication::applicationDirPath();
        p.dataDir = exeDir;
        if (!QFile::exists(QDir(exeDir).filePath("dict.kotoba")))
            p.dataDir = QDir::currentPath();
#endif

        QDir().mkpath(p.dataDir);

        p.configPath    = QDir(p.dataDir).filePath("config.ini");
        p.dictPath      = QDir(p.dataDir).filePath("dict.kotoba");
        p.dictIndexPath = QDir(p.dataDir).filePath("dict.kotoba.idx");
        p.srsPath       = QDir(p.dataDir).filePath("profile.srs");

        return p;
    }

#ifdef Q_OS_ANDROID
    // ── Versioned asset extraction (Android only) ─────────────────────────
    // Re-extracts all bundled assets whenever APP_ASSET_VERSION doesn't match
    // the cached version stamp in dataDir/.asset_version.
    //
    // How to trigger a re-extract on users' devices:
    //   1. Modify the asset file(s) in your repo.
    //   2. Bump APP_ASSET_VERSION above (e.g. "1.0.0" → "1.1.0").
    //   3. Ship the new APK.  Done — old cached files are replaced on next launch.
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

        // ── Version check ─────────────────────────────────────────────────
        const QString versionFile =
            QDir(targetDir).filePath(".asset_version");
        const QString currentVersion = QString(APP_ASSET_VERSION);

        QString cachedVersion;
        if (QFile::exists(versionFile)) {
            QFile f(versionFile);
            if (f.open(QIODevice::ReadOnly | QIODevice::Text))
                cachedVersion = QString::fromUtf8(f.readAll()).trimmed();
        }

        if (cachedVersion == currentVersion) {
            qDebug() << "Assets up-to-date (version" << currentVersion << ")";
            return;
        }

        qDebug() << "Asset version mismatch:"
                 << cachedVersion << "→" << currentVersion
                 << "— re-extracting all assets";

        // Remove stale files so nothing old lingers if the asset list shrank.
        for (const QString &name : kDataFiles) {
            const QString dest = QDir(targetDir).filePath(name);
            if (QFile::exists(dest) && !QFile::remove(dest))
                qWarning() << "Could not remove stale asset:" << dest;
        }

        // ── Extract ───────────────────────────────────────────────────────
        for (const QString &name : kDataFiles) {
            const QString dest = QDir(targetDir).filePath(name);
            const QString src  = QString("assets:/%1").arg(name);

            if (!QFile::exists(src)) {
                qWarning() << "Bundled asset not found:" << src;
                continue;
            }

            if (!QFile::copy(src, dest)) {
                qWarning() << "Failed to extract asset:" << src << "→" << dest;
                // On partial failure, don't write the version stamp —
                // next launch will retry the full extraction.
                return;
            }

            QFile::setPermissions(dest,
                QFile::ReadOwner | QFile::WriteOwner |
                QFile::ReadGroup | QFile::ReadOther);

            qDebug() << "Extracted:" << name;
        }

        // ── Write version stamp ───────────────────────────────────────────
        QFile f(versionFile);
        if (f.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
            f.write(currentVersion.toUtf8());
            qDebug() << "Asset version stamp written:" << currentVersion;
        } else {
            qWarning() << "Could not write version stamp:" << versionFile;
        }
    }
#endif
};