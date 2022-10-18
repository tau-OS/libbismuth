/*
 * Copyright (C) 2019 Alexander Mikhaylenko <exalm7659@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once

#if !defined(_BISMUTH_INSIDE) && !defined(BISMUTH_COMPILATION)
#error "Only <bismuth.h> can be included directly."
#endif

#include "bis-version.h"

#include <gtk/gtk.h>
#include "bis-swipeable.h"

G_BEGIN_DECLS

#define BIS_TYPE_SWIPE_TRACKER (bis_swipe_tracker_get_type())

BIS_AVAILABLE_IN_ALL
G_DECLARE_FINAL_TYPE (BisSwipeTracker, bis_swipe_tracker, BIS, SWIPE_TRACKER, GObject)

BIS_AVAILABLE_IN_ALL
BisSwipeTracker *bis_swipe_tracker_new (BisSwipeable *swipeable) G_GNUC_WARN_UNUSED_RESULT;

BIS_AVAILABLE_IN_ALL
BisSwipeable *bis_swipe_tracker_get_swipeable (BisSwipeTracker *self);

BIS_AVAILABLE_IN_ALL
gboolean bis_swipe_tracker_get_enabled (BisSwipeTracker *self);
BIS_AVAILABLE_IN_ALL
void     bis_swipe_tracker_set_enabled (BisSwipeTracker *self,
                                        gboolean         enabled);

BIS_AVAILABLE_IN_ALL
gboolean bis_swipe_tracker_get_reversed (BisSwipeTracker *self);
BIS_AVAILABLE_IN_ALL
void     bis_swipe_tracker_set_reversed (BisSwipeTracker *self,
                                         gboolean         reversed);

BIS_AVAILABLE_IN_ALL
gboolean         bis_swipe_tracker_get_allow_mouse_drag (BisSwipeTracker *self);
BIS_AVAILABLE_IN_ALL
void             bis_swipe_tracker_set_allow_mouse_drag (BisSwipeTracker *self,
                                                         gboolean         allow_mouse_drag);

BIS_AVAILABLE_IN_ALL
gboolean bis_swipe_tracker_get_allow_long_swipes (BisSwipeTracker *self);
BIS_AVAILABLE_IN_ALL
void     bis_swipe_tracker_set_allow_long_swipes (BisSwipeTracker *self,
                                                  gboolean         allow_long_swipes);

BIS_AVAILABLE_IN_ALL
void bis_swipe_tracker_shift_position (BisSwipeTracker *self,
                                       double           delta);

G_END_DECLS
