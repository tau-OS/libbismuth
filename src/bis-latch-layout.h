/*
 * Copyright (C) 2018-2020 Purism SPC
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

#define BIS_TYPE_LATCH_LAYOUT (bis_latch_layout_get_type())

BIS_AVAILABLE_IN_ALL
G_DECLARE_FINAL_TYPE (BisLatchLayout, bis_latch_layout, BIS, LATCH_LAYOUT, GtkLayoutManager)

BIS_AVAILABLE_IN_ALL
GtkLayoutManager *bis_latch_layout_new (void) G_GNUC_WARN_UNUSED_RESULT;

BIS_AVAILABLE_IN_ALL
int  bis_latch_layout_get_maximum_size (BisLatchLayout *self);
BIS_AVAILABLE_IN_ALL
void bis_latch_layout_set_maximum_size (BisLatchLayout *self,
                                        int             maximum_size);

BIS_AVAILABLE_IN_ALL
int  bis_latch_layout_get_tightening_threshold (BisLatchLayout *self);
BIS_AVAILABLE_IN_ALL
void bis_latch_layout_set_tightening_threshold (BisLatchLayout *self,
                                                int             tightening_threshold);

G_END_DECLS
