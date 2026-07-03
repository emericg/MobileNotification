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

#ifndef MOBILENOTIFICATION_QML_DISPATCHER_H
#define MOBILENOTIFICATION_QML_DISPATCHER_H
/* ************************************************************************** */

#include <QtQml/qqmlregistration.h>
#include <QObject>
#include <QString>

/* ************************************************************************** */

/*!
 * \brief Instantiable QML helper element forwarding MobileNotification singleton's signals.
 *
 * MobileNotification is a QML singleton, so it cannot carry inline signal handlers.
 * This dispatcher mirrors its event signals 1:1 and forwards them, letting QML
 * code attach declarative handlers as if it were an instantiable type:
 *
 * \code
 * MobileNotification_dispatcher {
 *     onNotificationClicked: (requestId) => { ... }
 *     onBadgeNumberChanged:  () => { ... }
 * }
 * \endcode
 *
 * It only relays signals; calls (notify(), cancel(), setBadgeNumber(), ...) still
 * go through the MobileNotification singleton directly.
 *
 * Any number of dispatchers can coexist, each gets its own copy of every signal.
 */
class MobileNotification_QmlDispatcher : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(MobileNotification_dispatcher)

Q_SIGNALS:
    void badgeNumberChanged();
    void notificationsEnabledChanged();
    void notificationClicked(int requestId, const QString &payload);

public:
    explicit MobileNotification_QmlDispatcher(QObject *parent = nullptr);
};

/* ************************************************************************** */
#endif // MOBILENOTIFICATION_QML_DISPATCHER_H
