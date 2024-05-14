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
#include <stdio.h>
#include <glib.h>
#include <dbus/dbus-glib.h>
#include <unistd.h>

#include "common.h"
#include "gopt.h"

#include "value-client-stub.h"

#define MAX_PROGRESSBAR_VALUE 101

static void print_usage(const char *filename, int failure)
{
    g_print("Usage: %s [-v] [-m] <value>\n"
            " -h\t--help\t\thelp\n"
            " -v\t--verbose\tverbose\n"
            " <value>\t\tint 0-%d (%d will not show a progressbar)\n"
            " \n"
            " These options may be followed by an integer for the progressbar:\n"
            " -m\t--mute\t\tvolume muted\n"
            " -c\t--micmute\tmicrophone muted\n"
            " -u\t--micunmute\tmicrophone unmuted\n"
            " -p\t--custom\tcustom icon\n"
            " Usage examples:\n"
            " \t$ volnoti-show -m\n"
            " \t$ volnoti-show -c 20\n"
            " \t$ volnoti-show -p /home/chad/svgs/play.svg 20\n"
            " Note that -p must be followed by a path and then the corresponding integer value.\n"
            " \n"
            " These options must be followed by a integer for the progressbar:\n"
            " -b\t--brightness\tdisplay brightness\n"
            " Usage examples:\n"
            " \t$ volnoti-show -b 76\n",
            filename, MAX_PROGRESSBAR_VALUE, MAX_PROGRESSBAR_VALUE);

    if (failure)
        exit(EXIT_FAILURE);
    else
        exit(EXIT_SUCCESS);
}

int main(int argc, const char *argv[])
{
    void *options = gopt_sort(
        &argc,
        argv,
        gopt_start(
            gopt_option('h', 0, gopt_shorts('h', '?'), gopt_longs("help", "HELP")),
            gopt_option('m', 0, gopt_shorts('m'), gopt_longs("mute")),
            gopt_option('c', 0, gopt_shorts('c'), gopt_longs("micmute")),
            gopt_option('u', 0, gopt_shorts('u'), gopt_longs("micunmute")),
            gopt_option('b', 0, gopt_shorts('b'), gopt_longs("brightness")),
            gopt_option('p', 0, gopt_shorts('p'), gopt_longs("custom")),
            gopt_option('v', GOPT_REPEAT, gopt_shorts('v'), gopt_longs("verbose"))));

    int help = gopt(options, 'h');
    int debug = gopt(options, 'v');
    int muted = gopt(options, 'm') ? 1 : gopt(options, 'c') ? 2
                                     : gopt(options, 'u')   ? 3
                                                            : 0;
    int brightness = gopt(options, 'b');
    int custom = gopt(options, 'p');

    gopt_free(options);

    if (help)
        print_usage(argv[0], FALSE);

    gint value = 0;
    char *custom_icon_path = NULL;

    if (muted)
    {
        if (argc > 2)
            print_usage(argv[0], TRUE);

        else if (argc == 2)
        {
            if (sscanf(argv[1], "%d", &value) != 1)
                print_usage(argv[0], TRUE);

            if (value > MAX_PROGRESSBAR_VALUE || value < 0)
                print_usage(argv[0], TRUE);
        }
        else
            value = 0;
    }
    else if (custom)
    {
        if (argc != 2 && argc != 3)
            print_usage(argv[0], TRUE);

        custom_icon_path = argv[1];
        if (argc == 3)
        {
            if (sscanf(argv[2], "%d", &value) != 1)
                print_usage(argv[0], TRUE);

            if (value > MAX_PROGRESSBAR_VALUE || value < 0)
                print_usage(argv[0], TRUE);
        }

        print_debug(argv[1], debug);
    }
    else
    {
        if (argc != 2)
            print_usage(argv[0], TRUE);

        if (sscanf(argv[1], "%d", &value) != 1)
            print_usage(argv[0], TRUE);

        if (value > MAX_PROGRESSBAR_VALUE || value < 0)
            print_usage(argv[0], TRUE);
    }

    DBusGConnection *bus = NULL;
    DBusGProxy *proxy = NULL;
    GError *error = NULL;

    // initialize GObject
    g_type_init();

    // connect to D-Bus
    print_debug("Connecting to D-Bus...", debug);
    bus = dbus_g_bus_get(DBUS_BUS_SESSION, &error);

    if (error != NULL)
        handle_error("Couldn't connect to D-Bus",
                     error->message,
                     TRUE);

    print_debug_ok(debug);

    // get the proxy
    print_debug("Getting proxy...", debug);
    proxy = dbus_g_proxy_new_for_name(bus,
                                      VALUE_SERVICE_NAME,
                                      VALUE_SERVICE_OBJECT_PATH,
                                      VALUE_SERVICE_INTERFACE);

    if (proxy == NULL)
        handle_error("Couldn't get a proxy for D-Bus",
                     "Unknown(dbus_g_proxy_new_for_name)",
                     TRUE);

    print_debug_ok(debug);

    print_debug("Sending value...", debug);
    uk_ac_cam_db538_VolumeNotification_notify(proxy, value, muted, brightness, custom, custom_icon_path, &error);

    if (error != NULL)
    {
        handle_error("Failed to send notification", error->message, FALSE);
        g_clear_error(&error);
        return EXIT_FAILURE;
    }

    print_debug_ok(debug);

    return EXIT_SUCCESS;
}
