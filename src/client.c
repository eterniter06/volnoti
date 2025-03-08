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
#include <unistd.h>

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

    if(failure)
        exit(EXIT_FAILURE);
    else
        exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[])
{
    char *customIconPath = NULL;
    char *customLabel = NULL;
    char *customLabelFont = NULL;
    char *customLabelColor = "#E6E6E6";

    int value = 0;
    int valueType = VOL_UNMUTED;
    int debug = 0;

    opterr = 0;

    const char *options = "vhm:c:u:b:p:t:x:f:";
    int option;
    int iconSelected = 0;

    while((option = getopt(argc, argv, options)) != -1)
        switch(option)
        {
            case 'm':
                if(iconSelected)
                    break;
                valueType = VOL_MUTED;
                value = atoi(optarg);
                iconSelected = 1;
                break;

            case 'c':
                if(iconSelected)
                    break;
                valueType = MIC_MUTED;
                value = atoi(optarg);
                iconSelected = 1;
                break;

            case 'u':
                if(iconSelected)
                    break;
                valueType = MIC_UNMUTED;
                value = atoi(optarg);
                iconSelected = 1;
                break;

            case 'b':
                if(iconSelected)
                    break;
                valueType = BRIGHTNESS;
                value = atoi(optarg);
                iconSelected = 1;
                break;

            case 'p':
                if(iconSelected)
                    break;
                valueType = CUSTOM;
                customIconPath = optarg;
                iconSelected = 1;
                break;

            case 't':
                customLabel = optarg;
                break;

            case 'x':
                customLabelColor = optarg;
                break;

            case 'f':
                customLabelFont = optarg;
                break;

            case 'v':
                debug = 1;
                break;

            case '?':
                print_usage(argv[0], 1);

            default:
                print_usage(argv[0], 0);
        }

    if(valueType == CUSTOM || valueType == VOL_UNMUTED)
    {
        // Choose the first non-optional value
        for(int index = optind; index < argc; index++)
            if(sscanf(argv[index], "%d", &value))
                break;
    }

    DBusGConnection *bus = NULL;
    DBusGProxy *proxy = NULL;
    GError *error = NULL;

    // initialize GObject
    g_type_init();

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
    proxy = dbus_g_proxy_new_for_name(bus,
        VALUE_SERVICE_NAME,
        VALUE_SERVICE_OBJECT_PATH,
        VALUE_SERVICE_INTERFACE);

    if(proxy == NULL)
        handle_error("Couldn't get a proxy for D-Bus",
            "Unknown(dbus_g_proxy_new_for_name)",
            TRUE);

    print_debug_ok(debug);

    print_debug("Sending value...", debug);

    uk_ac_cam_db538_VolumeNotification_notify(
        proxy,
        value,
        valueType,
        customIconPath,
        customLabel,
        customLabelFont,
        customLabelColor,
        &error
    );

    if(error != NULL)
    {
        handle_error("Failed to send notification", error->message, FALSE);
        g_clear_error(&error);
        return EXIT_FAILURE;
    }

    print_debug_ok(debug);

    return EXIT_SUCCESS;
}
