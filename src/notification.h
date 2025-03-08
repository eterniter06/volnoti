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
#ifndef NOTIFICATION_H
#define NOTIFICATION_H

#include <gtk/gtk.h>
#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

typedef struct
{
    gfloat alpha;
    gint corner_radius;
} Settings;

typedef struct
{
    gchar *labelText;
    gchar *labelFontAndSize;
    gchar *labelColorRGB;
}TextBoxData;

typedef struct
{
    GObject parent;

    gint value;
    gint valueType;

    GtkWindow *notification;

    GdkPixbuf *icon_high;
    GdkPixbuf *icon_medium;
    GdkPixbuf *icon_low;
    GdkPixbuf *icon_off;
    GdkPixbuf *icon_muted;
    GdkPixbuf *icon_micon;
    GdkPixbuf *icon_micmuted;
    GdkPixbuf *icon_brightness;

    GdkPixbuf *image_progressbar_empty;
    GdkPixbuf *image_progressbar_full;
    GdkPixbuf *image_progressbar;
    gint width_progressbar;
    gint height_progressbar;

    gint time_left;
    gint timeout;
    gboolean debug;
    Settings settings;
} VolumeObject;


Settings get_default_settings();
GtkWindow *create_notification(Settings settings, TextBoxData textBoxData);
void move_notification(GtkWindow *win, int x, int y);
void set_notification_icon(GtkWindow *nw, GdkPixbuf *pixbuf);
void set_progressbar_image(GtkWindow *nw, GdkPixbuf *pixbuf);
void destroyNotification(VolumeObject *obj);

#endif /* NOTIFICATION_H */