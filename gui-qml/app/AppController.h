#pragma once
#include <QObject>
#include <QString>

class AppController : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool appActive READ appActive NOTIFY appActiveChanged)

public:
    explicit AppController(QObject *parent = nullptr) : QObject(parent) {}

    bool appActive() const { return m_appActive; }

    void setAppActive(bool active) {
        if (m_appActive == active) return;
        m_appActive = active;
        emit appActiveChanged(active);
    }

signals:
    void appReady();
    void statusChanged(const QString &message);
    void appActiveChanged(bool active);

public slots:
    void notifyReady()                 { emit appReady(); }
    void setStatus(const QString &msg) { emit statusChanged(msg); }

private:
    bool m_appActive = true;
};