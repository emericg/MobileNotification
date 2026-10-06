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

#ifndef MOBILENOTIFICATION_ANDROID_H
#define MOBILENOTIFICATION_ANDROID_H
/* ************************************************************************** */

#include "MobileNotification_private.h"

/* ************************************************************************** */

/*!
 * \brief Android backend for MobileNotification (JNI to NotificationClient.java).
 *
 * Posts/cancels notifications and drives the app-icon badge through Android's
 * NotificationManager. It is a singleton: the JNI bridge in
 * MobileNotification_android.cpp calls back into getInstance() on a tap.
 *
 * Works from both the UI process and the (headless QCoreApplication) background
 * service process, using QAndroidApplication::context() for the Context.
 */
class AndroidNotificationUtils : public PlatformNotificationUtils
{
    static AndroidNotificationUtils *mInstance;

public:
    AndroidNotificationUtils(QObject *parent = nullptr);

    //! The process-wide instance (set by the constructor), used by the JNI bridge to reach this object
    static AndroidNotificationUtils *getInstance();

    void setSmallIconName(const QString &name) override;
    void registerChannel(int channel, const QString &name, int importance,
                         quint32 argb, const QString &iconName, const QString &soundName, bool vibration) override;
    void notify(const QString &title, const QString &message, int channel, int requestId, const QString &payload,
                const MobileNotificationStyle &style) override;
    void cancel(int requestId) override;
    void cancelAll() override;

    void setBadgeNumber(int number) override;

    bool areNotificationsEnabled() override;
    void openNotificationSettings() override;

    //! Called from JNI (NotificationClient.handleIntent) when the user taps a notification
    void onNotificationClicked(int requestId, const QString &payload);
};

/* ************************************************************************** */
#endif // MOBILENOTIFICATION_ANDROID_H
