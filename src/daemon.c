/**
 *  Volnoti - Lightweight Volume Notification
 *  Copyright (C) 2011  David Brazdil <db538@cam.ac.uk>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <glib.h>
#include <dbus/dbus-glib.h>

#include "common.h"
#include "gopt.h"
#include "notification.h"

#define IMAGE_PATH PREFIX

static VolumeObject *lastNotification = NULL;

typedef struct
{
    GObjectClass parent;
} VolumeObjectClass;

GType volume_object_get_type(void);
gboolean volume_object_notify(VolumeObject *obj,
    gint value,
    gint valueType,
    gchar *custom_icon_path,
    gchar *custom_label_text,
    gchar *custom_label_font_family_and_size,
    gchar *custom_label_font_color,
    GError **error
);

#define VOLUME_TYPE_OBJECT \
    (volume_object_get_type())

#define VOLUME_OBJECT(object)             \
    (G_TYPE_CHECK_INSTANCE_CAST((object), \
                                VOLUME_TYPE_OBJECT, VolumeObject))

#define VOLUME_OBJECT_CLASS(klass)    \
    (G_TYPE_CHECK_CLASS_CAST((klass), \
                             VOLUME_TYPE_OBJECT, VolumeObjectClass))

#define VOLUME_IS_OBJECT(object)          \
    (G_TYPE_CHECK_INSTANCE_TYPE((object), \
                                VOLUME_TYPE_OBJECT))

#define VOLUME_IS_OBJECT_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE((klass), \
                             VOLUME_TYPE_OBJECT))

#define VOLUME_OBJECT_GET_CLASS(obj)  \
    (G_TYPE_INSTANCE_GET_CLASS((obj), \
                               VOLUME_TYPE_OBJECT, VolumeObjectClass))

G_DEFINE_TYPE(VolumeObject, volume_object, G_TYPE_OBJECT)

#include "value-daemon-stub.h"

static void volume_object_init(VolumeObject *obj)
{
    g_assert(obj != NULL);
    obj->notification = NULL;
}

static void volume_object_class_init(VolumeObjectClass *klass)
{
    g_assert(klass != NULL);

    dbus_g_object_type_install_info(VOLUME_TYPE_OBJECT,
        &dbus_glib_volume_object_object_info);
}

static gboolean
time_handler(VolumeObject *obj)
{
    g_assert(obj != NULL);

    obj->time_left--;

    if(obj->time_left <= 0)
    {
        destroyNotification(obj);
        print_debug_ok(obj->debug);
        return FALSE;
    }

    return TRUE;
}

GdkPixbuf *getNotificationIconFromValueType(gint valueType, gint value, gchar *custom_icon_path, VolumeObject *obj)
{
    switch(valueType)
    {
        case CUSTOM:
            GError *local_error = NULL;
            GdkPixbuf *custom_icon = gdk_pixbuf_new_from_file(custom_icon_path, &local_error);

            if(local_error != NULL)
                handle_error("Couldn't load custom icon.", local_error->message, TRUE);

            return custom_icon;

        case BRIGHTNESS:
            return obj->icon_brightness;

        case VOL_MUTED:
            return obj->icon_muted;

        case MIC_MUTED:
            return obj->icon_micmuted;

        case MIC_UNMUTED:
            return obj->icon_micon;

        case VOL_UNMUTED:
            return
                value > 75 ? obj->icon_high
                : value >= 50 ? obj->icon_medium
                : value >= 25 ? obj->icon_low
                : obj->icon_off;

        default:
            return obj->icon_off;
    }
}

gboolean volume_object_notify(VolumeObject *obj,
    gint value,
    gint valueType,
    gchar *custom_icon_path,
    gchar *custom_label_text,
    gchar *custom_label_font_family_and_size,
    gchar *custom_label_font_color,
    GError **error)
{
    g_assert(obj != NULL);

    destroyNotification(lastNotification);
    lastNotification = obj;

    gboolean myuted = valueType == VOL_MUTED;
    gboolean micmuted = valueType == MIC_MUTED;
    gboolean micunmuted = valueType == MIC_UNMUTED;
    gboolean brightness = valueType == BRIGHTNESS;
    gboolean custom = valueType == CUSTOM;
    obj->valueType = valueType;
    obj->value = value;

    if(obj->notification == NULL)
    {
        print_debug("Creating new notification...", obj->debug);

        TextBoxData textBoxData;
        textBoxData.labelText = custom_label_text;
        textBoxData.labelFontAndSize = custom_label_font_family_and_size;
        textBoxData.labelColorRGB = custom_label_font_color;

        obj->notification = create_notification(obj->settings, textBoxData);
        gtk_widget_realize(GTK_WIDGET(obj->notification));
        obj->timeoutSourceId = g_timeout_add(100, (GSourceFunc) time_handler, (gpointer) obj);
        print_debug_ok(obj->debug);
    }

    GdkPixbuf *notificationIcon = getNotificationIconFromValueType(valueType, value, custom_icon_path, obj);
    set_notification_icon(GTK_WINDOW(obj->notification), notificationIcon);

    gboolean show_progressbar = obj->value >= 0 && obj->value <= 100;

    // prepare and set progress bar
    if(show_progressbar)
    {
        gint width_full = obj->width_progressbar * obj->value / 100;

        gdk_pixbuf_copy_area(
            obj->image_progressbar_full,
            0, 0,
            width_full,
            obj->height_progressbar,
            obj->image_progressbar,
            0, 0
        );

        gdk_pixbuf_copy_area(
            obj->image_progressbar_empty,
            width_full,
            0,
            obj->width_progressbar - width_full,
            obj->height_progressbar,
            obj->image_progressbar,
            width_full,
            0
        );

        set_progressbar_image(GTK_WINDOW(obj->notification), obj->image_progressbar);
    }
    else
    {
        set_progressbar_image(GTK_WINDOW(obj->notification), NULL);
    }

    obj->time_left = obj->timeout;
    gtk_widget_show_all(GTK_WIDGET(obj->notification));

    return TRUE;
}


static void print_usage(const char *filename, int failure)
{
    Settings settings = get_default_settings();
    g_print("Usage: %s [arguments]\n"
        " -h\t\t--help\t\t\thelp\n"
        " -v\t\t--verbose\t\tverbose\n"
        " -n\t\t--no-daemon\t\tdo not daemonize\n"
        "\n"
        "Configuration:\n"
        " -t <float>\t--timeout <float>\tnotification timeout in seconds with one optional decimal place\n"
        " -a <float>\t--alpha <float>\t\ttransparency level (0.0 - 1.0, default %.2f)\n"
        " -r <int>\t--corner-radius <int>\tradius of the round corners in pixels (default %d)\n",
        filename, settings.alpha, settings.corner_radius);

    if(failure)
        exit(EXIT_FAILURE);
    else
        exit(EXIT_SUCCESS);
}

GdkPixbuf *createPixbufFromFilename(const char *filename)
{
#define FILENAMELENGTH 513
    char filePath[FILENAMELENGTH];
    filePath[0] = '\0';
    strncat(filePath, IMAGE_PATH, FILENAMELENGTH - 1);
    strncat(filePath, filename, FILENAMELENGTH - 1);
#undef FILENAMELENGTH

    GError *error = NULL;
    GdkPixbuf *icon = gdk_pixbuf_new_from_file(filePath, &error);

    if(error)
    {
#define ERRORMSGLEN 513
        char failedLoadMessage[ERRORMSGLEN];
        failedLoadMessage[0] = '\0';
        strncat(failedLoadMessage, "Couldn't load ", ERRORMSGLEN - 1);
        strncat(failedLoadMessage, filePath, ERRORMSGLEN - 1);
        strncat(failedLoadMessage, ".", ERRORMSGLEN - 1);
#undef ERRORMSGLEN

        handle_error(failedLoadMessage, error->message, TRUE);
    }
    return icon;
}

int main(int argc, char *argv[])
{
    Settings settings = get_default_settings();
    int timeout = 30; // in ms

    void *options = gopt_sort(&argc, (const char **) argv, gopt_start(gopt_option('h', 0, gopt_shorts('h', '?'), gopt_longs("help", "HELP")), gopt_option('n', 0, gopt_shorts('n'), gopt_longs("no-daemon")), gopt_option('t', GOPT_ARG, gopt_shorts('t'), gopt_longs("timeout")), gopt_option('a', GOPT_ARG, gopt_shorts('a'), gopt_longs("alpha")), gopt_option('r', GOPT_ARG, gopt_shorts('r'), gopt_longs("corner-radius")), gopt_option('v', GOPT_REPEAT, gopt_shorts('v'), gopt_longs("verbose"))));

    int help = gopt(options, 'h');
    int debug = gopt(options, 'v');
    int no_daemon = gopt(options, 'n');

    float timeout_in; // cmd argument. Unused if unsupplied. Uninitialization is safe (for now)

    if(gopt(options, 't'))
    {
        if(sscanf(gopt_arg_i(options, 't', 0), "%f", &timeout_in) == 1 && timeout_in > 0.0f)
            timeout = (int) (timeout_in * 10);
        else
            print_usage(argv[0], TRUE);
    }

    if(gopt(options, 'a'))
    {
        if(sscanf(gopt_arg_i(options, 'a', 0), "%f", &settings.alpha) != 1 || settings.alpha < 0.0f || settings.alpha > 1.0f)
            print_usage(argv[0], TRUE);
    }

    if(gopt(options, 'r'))
    {
        if(sscanf(gopt_arg_i(options, 'r', 0), "%d", &settings.corner_radius) != 1)
            print_usage(argv[0], TRUE);
    }

    gopt_free(options);

    if(help)
        print_usage(argv[0], FALSE);

    DBusGConnection *bus = NULL;
    DBusGProxy *bus_proxy = NULL;
    VolumeObject *status = NULL;
    GMainLoop *main_loop = NULL;
    GError *error = NULL;
    guint result;

    // initialize GObject and GTK
    g_type_init();
    g_log_set_always_fatal(G_LOG_LEVEL_ERROR | G_LOG_LEVEL_CRITICAL);
    gtk_init(&argc, &argv);

    // create main loop
    main_loop = g_main_loop_new(NULL, FALSE);

    if(main_loop == NULL)
        handle_error("Couldn't create GMainLoop", "Unknown(OOM?)", TRUE);

    // connect to D-Bus
    print_debug("Connecting to D-Bus...", debug);
    bus = dbus_g_bus_get(DBUS_BUS_SESSION, &error);

    if(error != NULL)
        handle_error("Couldn't connect to D-Bus",
            error->message,
            TRUE);

    print_debug_ok(debug);

    // get the proxy
    print_debug("Getting proxy...", debug);
    bus_proxy = dbus_g_proxy_new_for_name(bus,
        DBUS_SERVICE_DBUS,
        DBUS_PATH_DBUS,
        DBUS_INTERFACE_DBUS);

    if(bus_proxy == NULL)
        handle_error("Couldn't get a proxy for D-Bus",
            "Unknown(dbus_g_proxy_new_for_name)",
            TRUE);

    print_debug_ok(debug);

    // register the service
    print_debug("Registering the service...", debug);

    if(!dbus_g_proxy_call(bus_proxy,
        "RequestName",
        &error,

        G_TYPE_STRING,
        VALUE_SERVICE_NAME,
        G_TYPE_UINT,
        0,
        G_TYPE_INVALID,

        G_TYPE_UINT,
        &result,
        G_TYPE_INVALID))
        handle_error("D-Bus.RequestName RPC failed",
            error->message,
            TRUE);

    if(result != 1)
        handle_error("Failed to get the primary well-known name. Possible cause: An instance of volnoti may already be running",
            "RequestName result != 1", TRUE);

    print_debug_ok(debug);

    // create the Volume object
    print_debug("Preparing data...", debug);
    status = g_object_new(VOLUME_TYPE_OBJECT, NULL);

    if(status == NULL)
        handle_error("Failed to create one VolumeObject instance.",
            "Unknown(OOM?)", TRUE);

    status->debug = debug;
    status->timeout = timeout;
    status->settings = settings;

    status->icon_high = createPixbufFromFilename("volume_high.svg");
    status->icon_medium = createPixbufFromFilename("volume_medium.svg");
    status->icon_low = createPixbufFromFilename("volume_low.svg");
    status->icon_off = createPixbufFromFilename("volume_off.svg");
    status->icon_muted = createPixbufFromFilename("volume_muted.svg");
    status->icon_micmuted = createPixbufFromFilename("mic_muted.svg");
    status->icon_micon = createPixbufFromFilename("mic_on.svg");
    status->icon_brightness = createPixbufFromFilename("brightness.svg");

    // progress bar
    status->image_progressbar_empty = createPixbufFromFilename("progressbar_empty.png");
    status->image_progressbar_full = createPixbufFromFilename("progressbar_full.png");

    // check that the images are of the same size
    if(gdk_pixbuf_get_width(status->image_progressbar_empty) != gdk_pixbuf_get_width(status->image_progressbar_full) ||
        gdk_pixbuf_get_height(status->image_progressbar_empty) != gdk_pixbuf_get_height(status->image_progressbar_full) ||
        gdk_pixbuf_get_bits_per_sample(status->image_progressbar_empty) != gdk_pixbuf_get_bits_per_sample(status->image_progressbar_full))
        handle_error("Progress bar images aren't of the same size or don't have the same number of bits per sample.", "Unknown(OOM?)", TRUE);

    // create pixbuf for combined image
    status->width_progressbar = gdk_pixbuf_get_width(status->image_progressbar_empty);
    status->height_progressbar = gdk_pixbuf_get_height(status->image_progressbar_empty);
    status->image_progressbar = gdk_pixbuf_new(GDK_COLORSPACE_RGB,
        TRUE,
        gdk_pixbuf_get_bits_per_sample(status->image_progressbar_empty),
        status->width_progressbar,
        status->height_progressbar);

    print_debug_ok(debug);

    // register the Volume object
    print_debug("Registering volume object...", debug);
    dbus_g_connection_register_g_object(bus,
        VALUE_SERVICE_OBJECT_PATH,
        G_OBJECT(status));
    print_debug_ok(debug);

    // daemonize
    if(!no_daemon)
    {
        print_debug("Daemonizing...\n", debug);

        if(daemon(0, 0) != 0)
            handle_error("failed to daemonize", "unknown", FALSE);
    }

    // Run forever
    print_debug("Running the main loop...\n", debug);
    g_main_loop_run(main_loop);
    return EXIT_FAILURE;
}
