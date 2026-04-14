#pragma once

#include <QString>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QCoreApplication>
#include <QDebug>

#define APP_ASSET_VERSION "1.0.0"

struct AppPaths
{
    QString dataDir;
    QString configPath;
    QString dictPath;
    QString dictIndexPath;
    QString srsBasePath;   // sin extensión — SrsService construye los archivos concretos

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
        p.dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
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
        p.srsBasePath   = QDir(p.dataDir).filePath("profile");  // sin extensión

        return p;
    }

#ifdef Q_OS_ANDROID
    static void extractAssetsIfNeeded(const QString &targetDir)
    {
        static const QStringList kDataFiles = {
            "dict.kotoba", "dict.kotoba.idx",
            "gloss_en.invx", "gloss_es.invx", "gloss_de.invx",
            "gloss_fr.invx", "gloss_hu.invx", "gloss_nl.invx",
            "gloss_ru.invx", "gloss_slv.invx", "gloss_sv.invx",
            "jp.invx",
        };

        QDir().mkpath(targetDir);

        const QString versionFile    = QDir(targetDir).filePath(".asset_version");
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

        qDebug() << "Asset version mismatch:" << cachedVersion
                 << "→" << currentVersion << "— re-extracting";

        for (const QString &name : kDataFiles) {
            const QString dest = QDir(targetDir).filePath(name);
            if (QFile::exists(dest) && !QFile::remove(dest))
                qWarning() << "Could not remove stale asset:" << dest;
        }

        for (const QString &name : kDataFiles) {
            const QString dest = QDir(targetDir).filePath(name);
            const QString src  = QString("assets:/%1").arg(name);
            if (!QFile::exists(src)) { qWarning() << "Missing:" << src; continue; }
            if (!QFile::copy(src, dest)) { qWarning() << "Failed:" << src; return; }
            QFile::setPermissions(dest,
                QFile::ReadOwner | QFile::WriteOwner |
                QFile::ReadGroup | QFile::ReadOther);
            qDebug() << "Extracted:" << name;
        }

        QFile f(versionFile);
        if (f.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
            f.write(currentVersion.toUtf8());
    }
#endif
};