/*
 * Copyright (C) 2020 Purism SPC
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

#define BIS_TYPE_CLAMP_SCROLLABLE (bis_clamp_scrollable_get_type())

BIS_AVAILABLE_IN_ALL
G_DECLARE_FINAL_TYPE (BisClampScrollable, bis_clamp_scrollable, BIS, CLAMP_SCROLLABLE, GtkWidget)

BIS_AVAILABLE_IN_ALL
GtkWidget *bis_clamp_scrollable_new (void) G_GNUC_WARN_UNUSED_RESULT;

BIS_AVAILABLE_IN_ALL
GtkWidget *bis_clamp_scrollable_get_child (BisClampScrollable *self);
BIS_AVAILABLE_IN_ALL
void       bis_clamp_scrollable_set_child (BisClampScrollable *self,
                                           GtkWidget          *child);

BIS_AVAILABLE_IN_ALL
int  bis_clamp_scrollable_get_maximum_size (BisClampScrollable *self);
BIS_AVAILABLE_IN_ALL
void bis_clamp_scrollable_set_maximum_size (BisClampScrollable *self,
                                            int                 maximum_size);

BIS_AVAILABLE_IN_ALL
int  bis_clamp_scrollable_get_tightening_threshold (BisClampScrollable *self);
BIS_AVAILABLE_IN_ALL
void bis_clamp_scrollable_set_tightening_threshold (BisClampScrollable *self,
                                                    int                 tightening_threshold);

G_END_DECLS
