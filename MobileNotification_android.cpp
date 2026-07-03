/*!
 * Copyright (c) 2026 Emeric Grange
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "MobileNotification_android.h"

#include <QCoreApplication>
#include <QJniObject>
#include <QJniEnvironment>
#include <jni.h>

/* ************************************************************************** */

AndroidNotificationUtils *AndroidNotificationUtils::mInstance = nullptr;

static const char *kNotificationClientClass = "io/emeric/mobilenotification/NotificationClient";

/* ************************************************************************** */

// JNI bridge: implements the 'native' method declared in NotificationClient.java.
// Registered at runtime via QJniEnvironment::registerNativeMethods() (Qt already owns JNI_OnLoad,
// so we must not define our own).
// Called on the Android UI thread when the host activity forwards a notification-tap intent;
// the queued signal/slot connection in MobileNotification then delivers it to the Qt thread.

extern "C"
{

static void jni_notificationClicked(JNIEnv */*env*/, jclass /*clazz*/, jint requestId, jstring payload)
{
    if (AndroidNotificationUtils::getInstance())
    {
        const QString qpayload = payload ? QJniObject(payload).toString() : QString();
        AndroidNotificationUtils::getInstance()->onNotificationClicked(requestId, qpayload);
    }
}

} // extern "C"

static void registerNativeMethods()
{
    const JNINativeMethod methods[] = {
        {"notificationClicked", "(ILjava/lang/String;)V", reinterpret_cast<void *>(jni_notificationClicked)},
    };

    QJniEnvironment env;
    if (!env.registerNativeMethods(kNotificationClientClass, methods,
                                   sizeof(methods) / sizeof(methods[0])))
    {
        qWarning() << "MobileNotification: failed to register NotificationClient native methods";
    }
}

/* ************************************************************************** */

AndroidNotificationUtils::AndroidNotificationUtils(QObject *parent) : PlatformNotificationUtils(parent)
{
    // We need the instance for the JNI callback
    mInstance = this;

    // Bind the NotificationClient 'native' method to our JNI bridge (once)
    static bool nativesRegistered = false;
    if (!nativesRegistered)
    {
        registerNativeMethods();
        nativesRegistered = true;
    }
}

AndroidNotificationUtils *AndroidNotificationUtils::getInstance()
{
    return mInstance;
}

/* ************************************************************************** */

void AndroidNotificationUtils::registerChannel(int channel, const QString &name, int importance, quint32 argb, const QString &iconName, const QString &soundName, bool vibration)
{
    QJniObject jsName = QJniObject::fromString(name);
    QJniObject jsIcon = QJniObject::fromString(iconName);
    QJniObject jsSound = QJniObject::fromString(soundName);
    QJniObject::callStaticMethod<void>(
        kNotificationClientClass, "registerChannel",
        "(Landroid/content/Context;ILjava/lang/String;IILjava/lang/String;Ljava/lang/String;Z)V",
        QNativeInterface::QAndroidApplication::context(),
        static_cast<jint>(channel),
        jsName.object<jstring>(),
        static_cast<jint>(importance),
        static_cast<jint>(argb),
        jsIcon.object<jstring>(),
        jsSound.object<jstring>(),
        static_cast<jboolean>(vibration));
}

void AndroidNotificationUtils::notify(const QString &title, const QString &message, int channel, int requestId, const QString &payload,
                                      const MobileNotificationStyle &style)
{
    QJniObject jsTitle = QJniObject::fromString(title);
    QJniObject jsMessage = QJniObject::fromString(message);
    QJniObject jsPayload = QJniObject::fromString(payload);
    QJniObject jsBigText = QJniObject::fromString(style.bigText);
    QJniObject jsLargeIcon = QJniObject::fromString(style.largeIcon);
    QJniObject jsBigPicture = QJniObject::fromString(style.bigPicture);
    QJniObject::callStaticMethod<void>(
        kNotificationClientClass, "notify",
        "(Landroid/content/Context;Ljava/lang/String;Ljava/lang/String;IILjava/lang/String;ILjava/lang/String;Ljava/lang/String;Ljava/lang/String;)V",
        QNativeInterface::QAndroidApplication::context(),
        jsTitle.object<jstring>(),
        jsMessage.object<jstring>(),
        static_cast<jint>(channel),
        static_cast<jint>(requestId),
        jsPayload.object<jstring>(),
        static_cast<jint>(style.color),
        jsBigText.object<jstring>(),
        jsLargeIcon.object<jstring>(),
        jsBigPicture.object<jstring>());
}

void AndroidNotificationUtils::cancel(int requestId)
{
    QJniObject::callStaticMethod<void>(
        kNotificationClientClass, "cancel",
        "(Landroid/content/Context;I)V",
        QNativeInterface::QAndroidApplication::context(),
        static_cast<jint>(requestId));
}

void AndroidNotificationUtils::cancelAll()
{
    QJniObject::callStaticMethod<void>(
        kNotificationClientClass, "cancelAll",
        "(Landroid/content/Context;)V",
        QNativeInterface::QAndroidApplication::context());
}

void AndroidNotificationUtils::setBadgeNumber(int number)
{
    QJniObject::callStaticMethod<void>(
        kNotificationClientClass, "setBadgeNumber",
        "(Landroid/content/Context;I)V",
        QNativeInterface::QAndroidApplication::context(),
        static_cast<jint>(number));
}

void AndroidNotificationUtils::setSmallIconName(const QString &name)
{
    QJniObject jsName = QJniObject::fromString(name);
    QJniObject::callStaticMethod<void>(
        kNotificationClientClass, "setSmallIconName",
        "(Ljava/lang/String;)V",
        jsName.object<jstring>());
}

bool AndroidNotificationUtils::areNotificationsEnabled()
{
    return QJniObject::callStaticMethod<jboolean>(
        kNotificationClientClass, "areNotificationsEnabled",
        "(Landroid/content/Context;)Z",
        QNativeInterface::QAndroidApplication::context());
}

void AndroidNotificationUtils::openNotificationSettings()
{
    QJniObject::callStaticMethod<void>(
        kNotificationClientClass, "openNotificationSettings",
        "(Landroid/content/Context;)V",
        QNativeInterface::QAndroidApplication::context());
}

/* ************************************************************************** */

void AndroidNotificationUtils::onNotificationClicked(int requestId, const QString &payload)
{
    qDebug() << "MobileNotification: notification tapped, requestId:" << requestId << "payload:" << payload;
    Q_EMIT notificationClicked(requestId, payload);
}

/* ************************************************************************** */
