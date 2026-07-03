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

package io.emeric.mobilenotification;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.media.AudioAttributes;
import android.net.Uri;
import android.os.Build;
import android.provider.Settings;

import java.util.HashMap;

/*!
 * \brief App-agnostic notification helper for the MobileNotification module.
 *
 * All methods are static and driven from C++ over JNI.

 * Channels are registered at runtime by the host application, so nothing here
 * is hardcoded to a specific app.
 *
 * The small notification icon is resolved by name ("ic_stat_logo") from the host
 * application's own resources; if absent, the app launcher icon is used as a fallback.
 */
public class NotificationClient
{
    // Intent extras carrying the tapped notification's requestId and payload back to the app
    public static final String EXTRA_REQUEST_ID = "io.emeric.mobilenotification.REQUEST_ID";
    public static final String EXTRA_PAYLOAD = "io.emeric.mobilenotification.PAYLOAD";

    // A reserved notification id used for the (best-effort) app-icon badge
    private static final int BADGE_NOTIFICATION_ID = 0x7FFFFFFF;
    private static final String BADGE_CHANNEL_ID = "mobilenotification_badge";

    // Native callback implemented in C++, fired when the user clock on a notification
    public static native void notificationClicked(int requestId, String payload);

    private static class ChannelInfo {
        String id;
        String name;
        int importance;
        int argb;
        String iconName; // optional per-channel small icon (null/empty fallback to app-wide icon)
    }

    private static final HashMap<Integer, ChannelInfo> sChannels = new HashMap<>();

    /* ********************************************************************** */

    private static int mapImportance(int importance) {
        // Our Importance enum values (1..4) already match Android's IMPORTANCE_* constants, clamp just in case
        if (importance < NotificationManager.IMPORTANCE_MIN) return NotificationManager.IMPORTANCE_DEFAULT;
        if (importance > NotificationManager.IMPORTANCE_HIGH) return NotificationManager.IMPORTANCE_HIGH;
        return importance;
    }

    private static String channelId(int channel) {
        return "mobilenotification_" + channel;
    }

    // Drawable resource name ("ic_stat_logo" by default)
    // That file must be in the resources of the application using MobileNotification
    private static volatile String sSmallIconName = "ic_stat_logo";

    public static void setSmallIconName(final String name) {
        if (name != null && !name.isEmpty()) {
            sSmallIconName = name;
        }
    }

    // Resolve a drawable resource name to its id, or 0 if absent
    // That file must be in the resources of the application using MobileNotification
    private static int drawableId(final Context context, final String name) {
        if (name == null || name.isEmpty()) return 0;
        return context.getResources().getIdentifier(name, "drawable", context.getPackageName());
    }

    // App-wide icon: 1) name from setSmallIconName(), 2) launcher icon, 3) generic system icon
    private static int resolveSmallIcon(final Context context) {
        int id = drawableId(context, sSmallIconName);
        if (id != 0) return id;

        try {
            id = context.getApplicationInfo().icon;
            if (id != 0) return id;
        } catch (Exception e) {
            // ignore
        }
        return android.R.drawable.ic_dialog_info;
    }

    private static int resolveSmallIcon(final Context context, final int channel) {
        // Per-channel icon if set and present, otherwise fall back to the app-wide icon
        ChannelInfo ci = sChannels.get(channel);
        int id = (ci != null) ? drawableId(context, ci.iconName) : 0;
        if (id != 0) return id;

        return resolveSmallIcon(context);
    }

    private static NotificationManager notificationManager(final Context context) {
        return (NotificationManager)context.getSystemService(Context.NOTIFICATION_SERVICE);
    }

    /* ********************************************************************** */

    public static void registerChannel(final Context context, final int channel,
                                       final String name, final int importance, final int argb,
                                       final String iconName, final String soundName,
                                       final boolean vibration) {
        ChannelInfo ci = new ChannelInfo();
        ci.id = channelId(channel);
        ci.name = name;
        ci.importance = mapImportance(importance);
        ci.argb = argb;
        ci.iconName = iconName;
        sChannels.put(channel, ci);

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            NotificationChannel nc = new NotificationChannel(ci.id, ci.name, ci.importance);
            nc.enableLights(true);
            nc.setLightColor(ci.argb);
            nc.enableVibration(vibration);
            nc.setLockscreenVisibility(Notification.VISIBILITY_PUBLIC);

            // Custom sound (a res/raw resource name) if provided; otherwise keep the channel default
            if (soundName != null && !soundName.isEmpty()) {
                int soundId = context.getResources().getIdentifier(soundName, "raw", context.getPackageName());
                if (soundId != 0) {
                    Uri soundUri = Uri.parse("android.resource://" + context.getPackageName() + "/" + soundId);
                    AudioAttributes attrs = new AudioAttributes.Builder()
                            .setUsage(AudioAttributes.USAGE_NOTIFICATION)
                            .setContentType(AudioAttributes.CONTENT_TYPE_SONIFICATION)
                            .build();
                    nc.setSound(soundUri, attrs);
                }
            }

            notificationManager(context).createNotificationChannel(nc);
        }
    }

    private static ChannelInfo ensureChannel(final Context context, final int channel) {
        ChannelInfo ci = sChannels.get(channel);
        if (ci == null) {
            // Lazily create a sane default channel if the app posted on an unregistered id
            registerChannel(context, channel, "Notifications", NotificationManager.IMPORTANCE_DEFAULT, 0xFFFFFFFF, null, null, false);
            ci = sChannels.get(channel);
        }
        return ci;
    }

    /* ********************************************************************** */

    public static void notify(final Context context, final String title, final String message,
                              final int channel, final int requestId, final String payload,
                              final int color, final String bigText, final String largeIcon,
                              final String bigPicture) {
        try {
            ChannelInfo ci = ensureChannel(context, channel);

            String packageName = context.getPackageName();
            Intent resultIntent = context.getPackageManager().getLaunchIntentForPackage(packageName);
            resultIntent.setFlags(Intent.FLAG_ACTIVITY_SINGLE_TOP | Intent.FLAG_ACTIVITY_CLEAR_TOP);
            resultIntent.putExtra(EXTRA_REQUEST_ID, requestId);
            resultIntent.putExtra(EXTRA_PAYLOAD, payload);

            // Distinct PendingIntent per requestId so extras don't get clobbered between notifications
            PendingIntent resultPendingIntent = PendingIntent.getActivity(
                    context, requestId, resultIntent,
                    PendingIntent.FLAG_UPDATE_CURRENT | PendingIntent.FLAG_IMMUTABLE);

            Notification.Builder builder = new Notification.Builder(context, ci.id);

            builder.setSmallIcon(resolveSmallIcon(context, channel));
            // Accent color tinting the small icon + header; 0 (COLOR_DEFAULT) means none
            builder.setColor(color);
            builder.setContentTitle(title);
            builder.setContentText(message);
            builder.setContentIntent(resultPendingIntent);
            builder.setWhen(System.currentTimeMillis());
            builder.setShowWhen(true);
            // (Sound/vibration/lights come from the channel on Android O+, not the builder)
            builder.setOnlyAlertOnce(true);
            builder.setAutoCancel(true);

            // Large icon (right-side thumbnail) from a local file path
            if (largeIcon != null && !largeIcon.isEmpty()) {
                Bitmap icon = BitmapFactory.decodeFile(largeIcon);
                if (icon != null) builder.setLargeIcon(icon);
            }

            // Expanded style: a big picture takes precedence over big text
            if (bigPicture != null && !bigPicture.isEmpty()) {
                Bitmap pic = BitmapFactory.decodeFile(bigPicture);
                if (pic != null) {
                    Notification.BigPictureStyle style = new Notification.BigPictureStyle().bigPicture(pic);
                    if (bigText != null && !bigText.isEmpty()) style.setSummaryText(bigText);
                    builder.setStyle(style);
                }
            } else if (bigText != null && !bigText.isEmpty()) {
                builder.setStyle(new Notification.BigTextStyle().bigText(bigText));
            }

            notificationManager(context).notify(requestId, builder.build());
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    public static void cancel(final Context context, final int requestId) {
        try {
            notificationManager(context).cancel(requestId);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    public static void cancelAll(final Context context) {
        try {
            notificationManager(context).cancelAll();
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    /* ********************************************************************** */

    public static boolean areNotificationsEnabled(final Context context) {
        try {
            return notificationManager(context).areNotificationsEnabled();
        } catch (Exception e) {
            e.printStackTrace();
            return true;
        }
    }

    public static void openNotificationSettings(final Context context) {
        try {
            Intent intent = new Intent(Settings.ACTION_APP_NOTIFICATION_SETTINGS);
            intent.putExtra(Settings.EXTRA_APP_PACKAGE, context.getPackageName());
            // Needed because 'context' may be an application/service context, not an activity
            intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            context.startActivity(intent);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    /* ********************************************************************** */

    /*!
     * \brief Best-effort to set an application icon badge
     *
     * Android has no universal launcher API, so this is best-effort (and launcher-dependent).
     * The count comes from an active notification carrying setNumber().
     * We keep a single reserved, low-priority, ongoing notification whose number
     * is the badge count, and cancel it when the count drops to 0.
     */
    public static void setBadgeNumber(final Context context, final int number) {
        try {
            NotificationManager nm = notificationManager(context);

            if (number <= 0) {
                nm.cancel(BADGE_NOTIFICATION_ID);
                return;
            }

            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
                NotificationChannel nc = new NotificationChannel(
                        BADGE_CHANNEL_ID, "Badge", NotificationManager.IMPORTANCE_LOW);
                nc.setShowBadge(true);
                nm.createNotificationChannel(nc);
            }

            Notification.Builder builder = new Notification.Builder(context, BADGE_CHANNEL_ID);

            builder.setSmallIcon(resolveSmallIcon(context));
            builder.setNumber(number);
            builder.setOngoing(true);
            builder.setShowWhen(false);
            builder.setLocalOnly(true);
            builder.setBadgeIconType(Notification.BADGE_ICON_SMALL);

            nm.notify(BADGE_NOTIFICATION_ID, builder.build());
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    /* ********************************************************************** */

    /*!
     * \brief Forward a launch/new intent so a notification tap reaches the app.
     * \return true if the intent carried one of our notifications' requestId.
     *
     * Call this from the host Activity's onCreate() and onNewIntent() with the
     * received Intent. It reads EXTRA_REQUEST_ID and fires the native callback,
     * which the module turns into MobileNotification::notificationClicked().
     */
    public static boolean handleIntent(final Intent intent) {
        if (intent == null || !intent.hasExtra(EXTRA_REQUEST_ID)) return false;

        int requestId = intent.getIntExtra(EXTRA_REQUEST_ID, 0);
        String payload = intent.getStringExtra(EXTRA_PAYLOAD);
        if (payload == null) payload = "";
        try {
            notificationClicked(requestId, payload);
        } catch (UnsatisfiedLinkError e) {
            // Native side not registered yet (module singleton not instantiated in this process)
            e.printStackTrace();
        }
        return true;
    }
}
