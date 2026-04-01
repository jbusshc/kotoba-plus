#pragma once
#include <QObject>
#include <QString>

class AppController : public QObject {
    Q_OBJECT
public:
    explicit AppController(QObject *parent = nullptr) : QObject(parent) {}

signals:
    void appReady();
    void statusChanged(const QString &message);

public slots:
    void notifyReady()             { emit appReady(); }
    void setStatus(const QString &msg) { emit statusChanged(msg); }
};