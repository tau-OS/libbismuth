/*
 * Copyright (C) 2018 Purism SPC
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

#define BIS_TYPE_CLAMP (bis_clamp_get_type())

BIS_AVAILABLE_IN_ALL
G_DECLARE_FINAL_TYPE (BisClamp, bis_clamp, BIS, CLAMP, GtkWidget)

BIS_AVAILABLE_IN_ALL
GtkWidget *bis_clamp_new (void) G_GNUC_WARN_UNUSED_RESULT;

BIS_AVAILABLE_IN_ALL
GtkWidget *bis_clamp_get_child (BisClamp  *self);
BIS_AVAILABLE_IN_ALL
void       bis_clamp_set_child (BisClamp  *self,
                                GtkWidget *child);

BIS_AVAILABLE_IN_ALL
int  bis_clamp_get_maximum_size (BisClamp *self);
BIS_AVAILABLE_IN_ALL
void bis_clamp_set_maximum_size (BisClamp *self,
                                 int       maximum_size);

BIS_AVAILABLE_IN_ALL
int  bis_clamp_get_tightening_threshold (BisClamp *self);
BIS_AVAILABLE_IN_ALL
void bis_clamp_set_tightening_threshold (BisClamp *self,
                                         int       tightening_threshold);

G_END_DECLS
