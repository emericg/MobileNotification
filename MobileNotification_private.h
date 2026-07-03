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

#ifndef MOBILENOTIFICATION_PRIVATE_H
#define MOBILENOTIFICATION_PRIVATE_H
/* ************************************************************************** */

#include <QObject>
#include <QString>
#include <QDebug>

#include "MobileNotificationStyle.h"

/* ************************************************************************** */

/*!
 * \brief Internal platform backend behind MobileNotification.
 *
 * This is the module's private interface: the base class provides a dummy
 * default implementation.
 */
class PlatformNotificationUtils : public QObject
{
    Q_OBJECT

Q_SIGNALS:
    void notificationClicked(int requestId, const QString &payload);

public:
    PlatformNotificationUtils(QObject *parent = nullptr) : QObject(parent) { }
    virtual ~PlatformNotificationUtils() = default;

    virtual void registerChannel(int channel, const QString &name, int importance, quint32 argb, const QString &iconName, const QString &soundName, bool vibration) {
        qDebug() << "MobileNotification::registerChannel()" << channel << name << importance << Qt::hex << argb << "icon:" << iconName << "sound:" << soundName << "vibration:" << vibration;
    }

    virtual void setSmallIconName(const QString &name) {
        qDebug() << "MobileNotification::setSmallIconName()" << name;
    }

    virtual void notify(const QString &title, const QString &message, int channel, int requestId, const QString &payload,
                        const MobileNotificationStyle &style) {
        qDebug() << "MobileNotification::notify()" << requestId << "channel:" << channel << "-" << title << "-" << message
                 << "payload:" << payload << "color:" << Qt::hex << style.color << "bigText:" << style.bigText
                 << "largeIcon:" << style.largeIcon << "bigPicture:" << style.bigPicture;
    }

    virtual void cancel(int requestId) {
        qDebug() << "MobileNotification::cancel()" << requestId;
    }

    virtual void cancelAll() {
        qDebug() << "MobileNotification::cancelAll()";
    }

    virtual void setBadgeNumber(int number) {
        qDebug() << "MobileNotification::setBadgeNumber()" << number;
    }

    virtual bool areNotificationsEnabled() {
        qDebug() << "MobileNotification::areNotificationsEnabled()";
        return true;
    }

    virtual void openNotificationSettings() {
        qDebug() << "MobileNotification::openNotificationSettings()";
    }
};

/* ************************************************************************** */
#endif // MOBILENOTIFICATION_PRIVATE_H
