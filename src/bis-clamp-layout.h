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

#define BIS_TYPE_CLAMP_LAYOUT (bis_clamp_layout_get_type())

BIS_AVAILABLE_IN_ALL
G_DECLARE_FINAL_TYPE (BisClampLayout, bis_clamp_layout, BIS, CLAMP_LAYOUT, GtkLayoutManager)

BIS_AVAILABLE_IN_ALL
GtkLayoutManager *bis_clamp_layout_new (void) G_GNUC_WARN_UNUSED_RESULT;

BIS_AVAILABLE_IN_ALL
int  bis_clamp_layout_get_maximum_size (BisClampLayout *self);
BIS_AVAILABLE_IN_ALL
void bis_clamp_layout_set_maximum_size (BisClampLayout *self,
                                        int             maximum_size);

BIS_AVAILABLE_IN_ALL
int  bis_clamp_layout_get_tightening_threshold (BisClampLayout *self);
BIS_AVAILABLE_IN_ALL
void bis_clamp_layout_set_tightening_threshold (BisClampLayout *self,
                                                int             tightening_threshold);

G_END_DECLS
