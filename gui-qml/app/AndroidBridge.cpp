#include "AndroidBridge.h"

#ifdef Q_OS_ANDROID
#include <QJniObject>
#include <QNativeInterface>
#endif

AndroidBridge::AndroidBridge(QObject *parent)
    : QObject(parent)
{}

void AndroidBridge::releaseSplash() {
#ifdef Q_OS_ANDROID
    QJniObject activity = QNativeInterface::QAndroidApplication::context();
    if (activity.isValid()) {
        activity.callMethod<void>("releaseSplash");
    }
#endif
}