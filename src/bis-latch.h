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

#define BIS_TYPE_LATCH (bis_latch_get_type())

BIS_AVAILABLE_IN_ALL
G_DECLARE_FINAL_TYPE (BisLatch, bis_latch, BIS, LATCH, GtkWidget)

BIS_AVAILABLE_IN_ALL
GtkWidget *bis_latch_new (void) G_GNUC_WARN_UNUSED_RESULT;

BIS_AVAILABLE_IN_ALL
GtkWidget *bis_latch_get_child (BisLatch  *self);
BIS_AVAILABLE_IN_ALL
void       bis_latch_set_child (BisLatch  *self,
                                GtkWidget *child);

BIS_AVAILABLE_IN_ALL
int  bis_latch_get_maximum_size (BisLatch *self);
BIS_AVAILABLE_IN_ALL
void bis_latch_set_maximum_size (BisLatch *self,
                                 int       maximum_size);

BIS_AVAILABLE_IN_ALL
int  bis_latch_get_tightening_threshold (BisLatch *self);
BIS_AVAILABLE_IN_ALL
void bis_latch_set_tightening_threshold (BisLatch *self,
                                         int       tightening_threshold);

G_END_DECLS
