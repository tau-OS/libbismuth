/*
 * Copyright (C) 2019 Alexander Mikhaylenko <exalm7659@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once

#if !defined(_BISMUTH_INSIDE) && !defined(BISMUTH_COMPILATION)
#error "Only <bismuth.h> can be included directly."
#endif

#include "bis-swipe-tracker.h"

G_BEGIN_DECLS

#define BIS_SWIPE_BORDER 32

void bis_swipe_tracker_reset (BisSwipeTracker *self);

G_END_DECLS
