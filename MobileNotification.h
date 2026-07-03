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

#ifndef MOBILENOTIFICATION_H
#define MOBILENOTIFICATION_H
/* ************************************************************************** */

#include <QtQml/qqmlregistration.h>

#include <QObject>
#include <QString>

#include "MobileNotification_private.h"

class QQmlEngine;
class QJSEngine;

/* ************************************************************************** */

/*!
 * \brief Cross-platform mobile notifications, exposed to QML as the `MobileNotification` type.
 *
 * This is the public entry point of the module. It wraps a per-platform
 * PlatformNotificationUtils backend and offers a uniform API to:
 *  - register notification channels (registerChannel())
 *  - post and cancel notifications (notify(), cancel(), cancelAll())
 *  - drive the application icon badge (setBadgeNumber(), clearBadge())
 *
 * This module is primarly intended to be used from C++ code.
 * While you can use the full feature set from a QML application, chances are,
 * a daemon service running in the background will be posting the notifications.
 * There just isn't much point to sending notification from a running UI.
 *
 * \note The module does NOT handle the OS notification permission:
 * requesting it needs a UI/Activity context and is best left to the calling application.
 * notify() assumes permission was already granted.
 *
 * \note On Android the posting instance (service) and the instance that receives notificationClicked()
 * (UI process, since a tap relaunches the Activity) live in different processes.
 */
class MobileNotification : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(bool notificationsEnabled READ notificationsEnabled NOTIFY notificationsEnabledChanged)
    Q_PROPERTY(int badgeNumber READ badgeNumber WRITE setBadgeNumber NOTIFY badgeNumberChanged)

Q_SIGNALS:
    /*!
     * \brief Emitted when the user taps one of our notifications.
     * \param requestId: The id the notification was posted with (see notify()).
     * \param payload: The opaque payload string the notification was posted with
     *                 (empty if none). Useful when the poster (e.g. a background
     *                 service) chose the requestId and the UI needs richer context
     *                 to route the tap (a device id, a deep-link route, JSON, ...).
     */
    void notificationClicked(int requestId, const QString &payload);

    void notificationsEnabledChanged();
    void badgeNumberChanged();

public:
    /*!
     * \brief Get the process-wide MobileNotification singleton instance.
     *
     * Use this to access the instance from C++. The same instance is shared with QML.
     */
    static MobileNotification *getInstance();

    /*!
     * \brief QML singleton factory.
     *
     * Called by the QML engine to obtain the MobileNotification singleton.
     * Returns getInstance(), so C++ and QML share the same single instance,
     * and flags it C++ owned so the JS engine never deletes it.
     */
    static MobileNotification *create(QQmlEngine *qmlEngine, QJSEngine *jsEngine);

    // Channels ////////////////////////////////////////////////////////////////

    /*!
     * \brief Channel importance, mapped to each platform's native levels.
     *
     * Mirrors Android's NotificationManager importance levels.
     * iOS has no channel concept, so it is essentially informational.
     */
    enum Importance {
        ImportanceMin = 1,      //!< No sound, no visual intrusion.
        ImportanceLow = 2,      //!< No sound.
        ImportanceDefault = 3,  //!< Makes a sound.
        ImportanceHigh = 4,     //!< Makes a sound and appears as a heads-up notification.
    };
    Q_ENUM(Importance)

    /*!
     * \brief Register (or update) a notification channel.
     * \param channel: Caller-chosen channel id, referenced later by notify().
     * \param name: Human-readable channel name shown in the OS settings.
     * \param importance: One of the Importance levels.
     * \param argb: Channel accent/light color as a packed 0xAARRGGBB value
     *              (passed as an int so the module keeps no Qt::Gui dependency).
     * \param iconName: Optional per-channel small (status-bar) icon; a drawable
     *              resource name in the HOST app's resources (res/drawable*).
     *              If empty (or the drawable is missing) it falls back to the
     *              app-wide icon set via setSmallIconName(), then the launcher icon.
     *              Android-only; ignored on iOS (which always uses the app icon).
     * \param soundName: Optional custom notification sound. Empty means the
     *              platform default. Android: a resource name in res/raw. iOS: a
     *              sound filename (with extension) bundled in the app.
     * \param vibration: Whether the channel vibrates. Android-only; iOS has no
     *              custom-vibration API (the system haptic follows the sound).
     *
     * The host app should register its channels once at startup.
     * On Android this creates/updates a NotificationChannel (Android O+).
     * On iOS it is mostly bookkeeping (no channel concept).
     *
     * \note On Android O+ sound/vibration/importance/light are locked in at first
     * creation and thereafter owned by the user; re-registering the same channel id
     * cannot change them (use a new id if you must).
     */
    Q_INVOKABLE void registerChannel(int channel, const QString &name, int importance,
                                     quint32 argb,
                                     const QString &iconName = QString(),
                                     const QString &soundName = QString(),
                                     bool vibration = false);

    // Notifications ///////////////////////////////////////////////////////////

    /*!
     * \brief Set the drawable resource name used as the small (status-bar) icon.
     * \param name: A drawable resource name present in the HOST application's resources (res/drawable*).
     * \note Call this once at startup, before notify().
     *
     * Android-only. iOS only uses the app icon.
     *
     * Defaults to "ic_stat_logo". The icon always lives in the host app, not in this module.
     * If the named drawable is missing, the launcher icon is used as a fallback.
     */
    Q_INVOKABLE void setSmallIconName(const QString &name);

    /*!
     * \brief Post (or update) a notification.
     * \param title: The notification title.
     * \param message: The notification body.
     * \param channel: The channel id to post on (must have been registered).
     * \param requestId: Caller-chosen id; posting again with the same id updates
     *                   the existing notification, and it is echoed back in
     *                   notificationClicked(). Also used as cancel() handle.
     * \param payload: Optional opaque string carried with the notification and
     *                 echoed back in notificationClicked() on tap (e.g. a device
     *                 id, a deep-link route, or a JSON blob). Empty if unused.
     * \param style: Optional visual/styling options (accent color, big text, large
     *               icon, big picture). See MobileNotificationStyle. From QML this
     *               is a JS object literal, e.g. { color: 0xFF2196F3, bigText: "..." }.
     *
     * \note Image paths in \a style are local file paths (not resource names or qrc URLs).
     * The realistic case being an image a service downloaded/cached to disk.
     */
    Q_INVOKABLE void notify(const QString &title, const QString &message,
                            int channel = 0, int requestId = 0, const QString &payload = QString(),
                            const MobileNotificationStyle &style = {});

    //! Cancel a previously posted notification by its requestId.
    Q_INVOKABLE void cancel(int requestId);

    //! Cancel all notifications posted by this application.
    Q_INVOKABLE void cancelAll();

    // App icon badge //////////////////////////////////////////////////////////

    //! Current app-icon badge count.
    int badgeNumber() const { return m_badgeNumber; }

    /*!
     * \brief Best-effort to set an application icon badge (and count).
     * \param number: The count to display; 0 clears the badge.
     *
     * iOS sets the icon badge natively.
     * Android has no universal launcher API, so this is best-effort (and launcher-dependent).
     */
    Q_INVOKABLE void setBadgeNumber(int number);

    //! Clear the app-icon badge
    Q_INVOKABLE void clearBadge() { setBadgeNumber(0); }

    // Settings / status ///////////////////////////////////////////////////////

    /*!
     * \brief Last-known notifications-enabled state (cached).
     *
     * Call areNotificationsEnabled() to refresh it, like when the app returns to the foreground.
     * The platform query is not free, and the state is only changed out-of-app by the user.
     */
    bool notificationsEnabled() const { return m_notificationsEnabled; }

    /*!
     * \brief Query whether the user currently allows notifications for this application.
     *
     * A read-only query, distinct from requesting the permission.
     * Lets a background service skip pointless posts, or the UI show a "muted" hint.
     *
     * Android: NotificationManager.areNotificationsEnabled().
     * iOS: the authorization status from the notification center.
     */
    Q_INVOKABLE bool areNotificationsEnabled();

    /*!
     * \brief Open the system notification-settings page for this app.
     *
     * A shortcut to let the user re-enable notifications they muted.
     * Android opens the app's notification settings.
     * iOS opens the app's settings page.
     */
    Q_INVOKABLE void openNotificationSettings();

private:
    int m_badgeNumber = 0;
    bool m_notificationsEnabled = true;

    //! Per-platform backend.
    PlatformNotificationUtils *mPlatformNotificationUtils = nullptr;

    // Singleton
    explicit MobileNotification(QObject *parent = nullptr);
};

/* ************************************************************************** */
#endif // MOBILENOTIFICATION_H
