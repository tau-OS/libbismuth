/*
 * Copyright (C) 2019 Purism SPC
 * Copyright (C) 2021 Manuel Genovés <manuel.genoves@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once

#if !defined(_BISMUTH_INSIDE) && !defined(BISMUTH_COMPILATION)
#error "Only <bismuth.h> can be included directly."
#endif

#include "bis-version.h"

#include <gtk/gtk.h>

G_BEGIN_DECLS

BIS_AVAILABLE_IN_ALL
double bis_lerp (double a,
                 double b,
                 double t);

BIS_AVAILABLE_IN_ALL
gboolean bis_get_enable_animations (GtkWidget *widget);

G_END_DECLS
