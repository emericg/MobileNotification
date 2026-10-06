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

#ifndef MOBILENOTIFICATION_IOS_H
#define MOBILENOTIFICATION_IOS_H
/* ************************************************************************** */

#include "MobileNotification_private.h"

#include <QHash>

/* ************************************************************************** */

/*!
 * \brief iOS backend for MobileNotification (UNUserNotificationCenter).
 *
 * Posts/cancels local notifications and drives the app-icon badge. iOS has no
 * channel concept, so registerChannel() is a no-op (the channel id is reused as
 * the notification's thread identifier for grouping).
 *
 * A UNUserNotificationCenterDelegate reads the requestId back from a tapped
 * notification and forwards it through onNotificationClicked().
 *
 * \note This header is pure C++ (the delegate is held as an opaque pointer) so
 * it can be included from the C++ MobileNotification.cpp; the implementation
 * lives in the Objective-C++ MobileNotification_ios.mm.
 */
class IosNotificationUtils : public PlatformNotificationUtils
{
    void *m_notifdelegate = nullptr; //!< retained NotificationDelegate (Obj-C)

    //! iOS has no channels; remember each of our channel's custom sound to apply it in notify()
    QHash<int, QString> m_channelSounds;

    //! Likewise remember each channel's importance, applied per notification as interruptionLevel
    QHash<int, int> m_channelImportance;

public:
    IosNotificationUtils(QObject *parent = nullptr);
    ~IosNotificationUtils();

    void registerChannel(int channel, const QString &name, int importance,
                         quint32 argb, const QString &iconName, const QString &soundName, bool vibration) override;
    void notify(const QString &title, const QString &message, int channel, int requestId, const QString &payload,
                const MobileNotificationStyle &style) override;
    void cancel(int requestId) override;
    void cancelAll() override;

    void setBadgeNumber(int number) override;

    bool areNotificationsEnabled() override;

    //! iOS has no dedicated notification-settings deep-link, so open the app's settings page
    void openNotificationSettings() override;

    //! Called from the notification-center delegate when the user taps a notification
    void onNotificationClicked(int requestId, const QString &payload);
};

/* ************************************************************************** */
#endif // MOBILENOTIFICATION_IOS_H
