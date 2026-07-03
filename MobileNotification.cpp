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

#include "MobileNotification.h"

#if defined(Q_OS_IOS)
#include "MobileNotification_ios.h"
#elif defined(Q_OS_ANDROID)
#include "MobileNotification_android.h"
#endif

#include <QCoreApplication>
#include <QQmlEngine>
#include <QJSEngine>

/* ************************************************************************** */

MobileNotification *MobileNotification::getInstance()
{
    static MobileNotification *instance = new MobileNotification(QCoreApplication::instance());
    return instance;
}

MobileNotification *MobileNotification::create(QQmlEngine *, QJSEngine *)
{
    MobileNotification *instance = getInstance();
    QJSEngine::setObjectOwnership(instance, QJSEngine::CppOwnership);
    return instance;
}

/* ************************************************************************** */

MobileNotification::MobileNotification(QObject *parent) : QObject(parent)
{
#if defined(Q_OS_IOS)
    mPlatformNotificationUtils = new IosNotificationUtils(this);
#elif defined(Q_OS_ANDROID)
    mPlatformNotificationUtils = new AndroidNotificationUtils(this);
#else
    mPlatformNotificationUtils = new PlatformNotificationUtils(this);
#endif

    // Forward the backend's tap signal straight through to our public signal
    connect(mPlatformNotificationUtils, &PlatformNotificationUtils::notificationClicked,
            this, &MobileNotification::notificationClicked);
}

/* ************************************************************************** */

void MobileNotification::registerChannel(int channel, const QString &name, int importance,
                                         quint32 argb, const QString &iconName, const QString &soundName, bool vibration)
{
    mPlatformNotificationUtils->registerChannel(channel, name, importance, argb, iconName, soundName, vibration);
}

void MobileNotification::notify(const QString &title, const QString &message, int channel,
                                int requestId, const QString &payload,
                                const MobileNotificationStyle &style)
{
    mPlatformNotificationUtils->notify(title, message, channel, requestId, payload, style);
}

void MobileNotification::cancel(int requestId)
{
    mPlatformNotificationUtils->cancel(requestId);
}

void MobileNotification::cancelAll()
{
    mPlatformNotificationUtils->cancelAll();
}

void MobileNotification::setSmallIconName(const QString &name)
{
    mPlatformNotificationUtils->setSmallIconName(name);
}

bool MobileNotification::areNotificationsEnabled()
{
    const bool enabled = mPlatformNotificationUtils->areNotificationsEnabled();
    if (enabled != m_notificationsEnabled)
    {
        m_notificationsEnabled = enabled;
        Q_EMIT notificationsEnabledChanged();
    }
    return enabled;
}

void MobileNotification::openNotificationSettings()
{
    mPlatformNotificationUtils->openNotificationSettings();
}

/* ************************************************************************** */

void MobileNotification::setBadgeNumber(int number)
{
    if (number < 0) number = 0;
    if (number == m_badgeNumber) return;

    m_badgeNumber = number;
    mPlatformNotificationUtils->setBadgeNumber(number);

    Q_EMIT badgeNumberChanged();
}

/* ************************************************************************** */
