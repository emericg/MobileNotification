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

#include "MobileNotification_ios.h"

#include <QString>

#import <UserNotifications/UserNotifications.h>
#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

/* ************************************************************************** */

@interface MobileNotificationDelegate : UIResponder <UIApplicationDelegate, UNUserNotificationCenterDelegate>
{
    IosNotificationUtils *g_notifier;
}
@end

@implementation MobileNotificationDelegate

- (id)initWithObject:(IosNotificationUtils *)notifier
{
    self = [super init];
    if (self)
    {
        g_notifier = notifier;
    }
    return self;
}

// Present notifications even when the app is in the foreground
- (void)userNotificationCenter:(UNUserNotificationCenter *)center
        willPresentNotification:(UNNotification *)notification
            withCompletionHandler:(void (^)(UNNotificationPresentationOptions options))completionHandler
{
    Q_UNUSED(center)
    Q_UNUSED(notification)
    completionHandler(UNNotificationPresentationOptionList | UNNotificationPresentationOptionBanner | UNNotificationPresentationOptionSound);
}

// Fired when the user taps a notification: read the requestId and forward it
- (void)userNotificationCenter:(UNUserNotificationCenter *)center
        didReceiveNotificationResponse:(UNNotificationResponse *)response
            withCompletionHandler:(void(^)(void))completionHandler
{
    Q_UNUSED(center)
    if (g_notifier)
    {
        NSDictionary *userInfo = response.notification.request.content.userInfo;
        NSNumber *rid = [userInfo objectForKey:@"ID"];
        NSString *payload = [userInfo objectForKey:@"PAYLOAD"];
        g_notifier->onNotificationClicked(rid ? [rid intValue] : 0,
                                          payload ? QString::fromNSString(payload) : QString());
    }
    completionHandler();
}
@end

/* ************************************************************************** */

IosNotificationUtils::IosNotificationUtils(QObject *parent) : PlatformNotificationUtils(parent)
{
    MobileNotificationDelegate *delegate = [[MobileNotificationDelegate alloc] initWithObject:this];

    // Retain the delegate (ARC) behind the opaque void* member
    m_notifdelegate = (void *)CFBridgingRetain(delegate);

    // Install it now, so taps are delivered for the whole app lifetime
    [UNUserNotificationCenter currentNotificationCenter].delegate = delegate;
}

IosNotificationUtils::~IosNotificationUtils()
{
    if (m_notifdelegate)
    {
        CFBridgingRelease(m_notifdelegate);
        m_notifdelegate = nullptr;
    }
}

/* ************************************************************************** */

void IosNotificationUtils::registerChannel(int channel, const QString &name, int importance, quint32 argb, const QString &iconName, const QString &soundName, bool vibration)
{
    Q_UNUSED(name)
    Q_UNUSED(argb)
    Q_UNUSED(iconName)
    Q_UNUSED(vibration)

    // iOS has no channel concept, so nothing to create.
    // - The channel id is reused as the notification's thread identifier in notify() for grouping.
    // - iOS always uses the app icon (iconName ignored) and has no custom-vibration API (vibration ignored).
    // - Sound and importance are per-notification on iOS, so remember them here and apply them in notify().

    if (soundName.isEmpty()) m_channelSounds.remove(channel);
    else m_channelSounds.insert(channel, soundName);

    m_channelImportance.insert(channel, importance);
}

void IosNotificationUtils::notify(const QString &title, const QString &message, int channel, int requestId, const QString &payload,
                                  const MobileNotificationStyle &style)
{
    // style.color is ignored: iOS has no per-notification accent color.
    // style.progress* / style.ongoing are ignored: iOS has no notification progress bar
    // and no ongoing/persistent flag (a Live Activity would be needed for progress).

    UNMutableNotificationContent *content = [[UNMutableNotificationContent alloc] init];
    content.title = title.toNSString();

    // iOS body already expands/wraps, so use the longer bigText when provided
    content.body = (style.bigText.isEmpty() ? message : style.bigText).toNSString();

    // Per-channel custom sound if one was registered, otherwise the default
    const QString soundName = m_channelSounds.value(channel);
    content.sound = soundName.isEmpty() ? [UNNotificationSound defaultSound]
                                        : [UNNotificationSound soundNamed:soundName.toNSString()];

    // A single image attachment (thumbnail collapsed, large expanded): prefer the
    // big picture, fall back to the large icon. iOS has no separate large-icon slot.
    const QString imagePath = style.bigPicture.isEmpty() ? style.largeIcon : style.bigPicture;
    if (!imagePath.isEmpty())
    {
        NSURL *url = [NSURL fileURLWithPath:imagePath.toNSString()];
        NSError *err = nil;
        UNNotificationAttachment *att =
            [UNNotificationAttachment attachmentWithIdentifier:@"image" URL:url options:nil error:&err];
        if (att) content.attachments = @[ att ];
        else if (err) NSLog(@"MobileNotification: attachment failed: %@", err);
    }

    content.threadIdentifier = QString::number(channel).toNSString();
    content.userInfo = @{ @"ID": @(requestId), @"PAYLOAD": payload.toNSString() };

    // Map the posting channel's importance to an interruptionLevel (iOS 15+).
    if (@available(iOS 15.0, *))
    {
        switch (m_channelImportance.value(channel, 3 /*ImportanceDefault*/))
        {
            case 1: // ImportanceMin
            case 2: // ImportanceLow
                content.interruptionLevel = UNNotificationInterruptionLevelPassive;
                break;
            case 5: // ImportanceUrgent
                content.interruptionLevel = UNNotificationInterruptionLevelTimeSensitive;
                break;
            case 3: // ImportanceDefault
            case 4: // ImportanceHigh
            default:
                content.interruptionLevel = UNNotificationInterruptionLevelActive;
                break;
        }
    }

    // Small non-zero delay, like the original AppUtils implementation
    UNTimeIntervalNotificationTrigger *trigger =
        [UNTimeIntervalNotificationTrigger triggerWithTimeInterval:0.33 repeats:NO];

    // Use the requestId as identifier so a repeat posts an update and cancel() can target it
    NSString *identifier = QString::number(requestId).toNSString();

    UNNotificationRequest *request =
        [UNNotificationRequest requestWithIdentifier:identifier content:content trigger:trigger];

    [[UNUserNotificationCenter currentNotificationCenter]
        addNotificationRequest:request withCompletionHandler:^(NSError *_Nullable error) {
            if (error) NSLog(@"MobileNotification: addNotificationRequest failed: %@", error);
        }];
}

void IosNotificationUtils::cancel(int requestId)
{
    NSArray<NSString *> *ids = @[ QString::number(requestId).toNSString() ];
    UNUserNotificationCenter *center = [UNUserNotificationCenter currentNotificationCenter];
    [center removePendingNotificationRequestsWithIdentifiers:ids];
    [center removeDeliveredNotificationsWithIdentifiers:ids];
}

void IosNotificationUtils::cancelAll()
{
    UNUserNotificationCenter *center = [UNUserNotificationCenter currentNotificationCenter];
    [center removeAllPendingNotificationRequests];
    [center removeAllDeliveredNotifications];
}

void IosNotificationUtils::setBadgeNumber(int number)
{
    if (@available(iOS 16.0, *))
    {
        [[UNUserNotificationCenter currentNotificationCenter] setBadgeCount:number withCompletionHandler:nil];
    }
    else
    {
        // applicationIconBadgeNumber must be touched on the main thread
        dispatch_async(dispatch_get_main_queue(), ^{
            [UIApplication sharedApplication].applicationIconBadgeNumber = number;
        });
    }
}

/* ************************************************************************** */

bool IosNotificationUtils::areNotificationsEnabled()
{
    // getNotificationSettingsWithCompletionHandler is async and its handler runs on
    // an internal queue (never the main queue), so a semaphore is safe to block on
    // even when called from the main thread.

    __block BOOL enabled = NO;
    dispatch_semaphore_t sem = dispatch_semaphore_create(0);

    [[UNUserNotificationCenter currentNotificationCenter]
        getNotificationSettingsWithCompletionHandler:^(UNNotificationSettings *settings) {
            enabled = (settings.authorizationStatus == UNAuthorizationStatusAuthorized ||
                       settings.authorizationStatus == UNAuthorizationStatusProvisional ||
                       settings.authorizationStatus == UNAuthorizationStatusEphemeral);
            dispatch_semaphore_signal(sem);
        }];

    dispatch_semaphore_wait(sem, DISPATCH_TIME_FOREVER);
    return enabled;
}

void IosNotificationUtils::openNotificationSettings()
{
    // Must run on the main thread
    dispatch_async(dispatch_get_main_queue(), ^{
        NSURL *url = [NSURL URLWithString:UIApplicationOpenSettingsURLString];
        if (url && [[UIApplication sharedApplication] canOpenURL:url])
        {
            [[UIApplication sharedApplication] openURL:url options:@{} completionHandler:nil];
        }
    });
}

/* ************************************************************************** */

void IosNotificationUtils::onNotificationClicked(int requestId, const QString &payload)
{
    qDebug() << "MobileNotification: notification tapped, requestId:" << requestId << "payload:" << payload;
    Q_EMIT notificationClicked(requestId, payload);
}

/* ************************************************************************** */
