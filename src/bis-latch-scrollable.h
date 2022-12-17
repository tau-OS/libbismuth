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

#define BIS_TYPE_LATCH_SCROLLABLE (bis_latch_scrollable_get_type())

BIS_AVAILABLE_IN_ALL
G_DECLARE_FINAL_TYPE (BisLatchScrollable, bis_latch_scrollable, BIS, LATCH_SCROLLABLE, GtkWidget)

BIS_AVAILABLE_IN_ALL
GtkWidget *bis_latch_scrollable_new (void) G_GNUC_WARN_UNUSED_RESULT;

BIS_AVAILABLE_IN_ALL
GtkWidget *bis_latch_scrollable_get_child (BisLatchScrollable *self);
BIS_AVAILABLE_IN_ALL
void       bis_latch_scrollable_set_child (BisLatchScrollable *self,
                                           GtkWidget          *child);

BIS_AVAILABLE_IN_ALL
int  bis_latch_scrollable_get_maximum_size (BisLatchScrollable *self);
BIS_AVAILABLE_IN_ALL
void bis_latch_scrollable_set_maximum_size (BisLatchScrollable *self,
                                            int                 maximum_size);

BIS_AVAILABLE_IN_ALL
int  bis_latch_scrollable_get_tightening_threshold (BisLatchScrollable *self);
BIS_AVAILABLE_IN_ALL
void bis_latch_scrollable_set_tightening_threshold (BisLatchScrollable *self,
                                                    int                 tightening_threshold);

G_END_DECLS
