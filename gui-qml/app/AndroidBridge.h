#pragma once

#include <QObject>

class AndroidBridge : public QObject {
    Q_OBJECT
public:
    explicit AndroidBridge(QObject *parent = nullptr);

public slots:
    void releaseSplash();
};