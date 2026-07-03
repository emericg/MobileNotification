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

#ifndef MOBILENOTIFICATION_STYLE_H
#define MOBILENOTIFICATION_STYLE_H
/* ************************************************************************** */

#include <QtQml/qqmlregistration.h>
#include <qobjectdefs.h>

#include <QString>

/* ************************************************************************** */

/*!
 * \brief Optional per-notification visual/styling options.
 *
 * A plain value type (Q_GADGET) grouping the appearance parameters of notify().
 * Color stays a packed int (not QColor) so the module keeps no Qt::Gui dependency.
 *
 * Exposed to QML as the `notificationStyle` value type, so QML can pass a JS object literal:
 * MobileNotification.notify("Title", "Body", 1, 42, "payload", { color: 0xFF2196F3, bigText: "..." })
 */
struct MobileNotificationStyle
{
    Q_GADGET
    QML_VALUE_TYPE(notificationStyle)
    QML_STRUCTURED_VALUE

    Q_PROPERTY(quint32 color MEMBER color)
    Q_PROPERTY(QString bigText MEMBER bigText)
    Q_PROPERTY(QString largeIcon MEMBER largeIcon)
    Q_PROPERTY(QString bigPicture MEMBER bigPicture)

public:
    //! Accent color as a packed 0xAARRGGBB value; 0 = platform default. Android-only.
    quint32 color = 0;

    //! Long, expandable body text. Android: BigTextStyle. iOS: replaces the body.
    QString bigText;

    //! Absolute file path to a local image shown as the large/right-side icon.
    QString largeIcon;

    //! Absolute file path to a local image shown large when expanded.
    QString bigPicture;
};

/* ************************************************************************** */
#endif // MOBILENOTIFICATION_STYLE_H
