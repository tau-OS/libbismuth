/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.> See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#if !defined(_BISMUTH_INSIDE) && !defined(BISMUTH_COMPILATION)
#error "Only <bismuth.h> can be included directly."
#endif

#include <gtk/gtk.h>

G_BEGIN_DECLS

gboolean bis_widget_focus_child (GtkWidget        *widget,
                                 GtkDirectionType  direction);

gboolean bis_widget_grab_focus_self  (GtkWidget *widget);
gboolean bis_widget_grab_focus_child (GtkWidget *widget);

void bis_widget_compute_expand (GtkWidget *widget,
                                gboolean  *hexpand_p,
                                gboolean  *vexpand_p);

void bis_widget_compute_expand_horizontal_only (GtkWidget *widget,
                                                gboolean  *hexpand_p,
                                                gboolean  *vexpand_p);

GtkSizeRequestMode bis_widget_get_request_mode (GtkWidget *widget);

void bis_widget_get_style_color (GtkWidget *widget,
                                 GdkRGBA   *rgba);

gboolean bis_widget_lookup_color (GtkWidget  *widget,
                                  const char *name,
                                  GdkRGBA    *rgba);

G_END_DECLS
