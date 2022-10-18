/*
 * Copyright (C) 2021 Purism SPC
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * Author: Alexander Mikhaylenko <alexander.mikhaylenko@puri.sm>
 */

#pragma once

#if !defined(_BISMUTH_INSIDE) && !defined(BISMUTH_COMPILATION)
#error "Only <bismuth.h> can be included directly."
#endif

#include "bis-version.h"

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define BIS_TYPE_BIN (bis_bin_get_type())

BIS_AVAILABLE_IN_ALL
G_DECLARE_DERIVABLE_TYPE (BisBin, bis_bin, BIS, BIN, GtkWidget)

struct _BisBinClass
{
  GtkWidgetClass parent_class;
};

BIS_AVAILABLE_IN_ALL
GtkWidget *bis_bin_new (void) G_GNUC_WARN_UNUSED_RESULT;

BIS_AVAILABLE_IN_ALL
GtkWidget *bis_bin_get_child (BisBin    *self);
BIS_AVAILABLE_IN_ALL
void       bis_bin_set_child (BisBin    *self,
                              GtkWidget *child);

G_END_DECLS
