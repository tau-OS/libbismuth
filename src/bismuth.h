/*
 * Copyright (C) 2017 Purism SPC
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

#if !GTK_CHECK_VERSION(4, 4, 0)
# error "libbismuth requires gtk4 >= 4.4.0"
#endif

#if !GLIB_CHECK_VERSION(2, 66, 0)
# error "libbismuth requires glib-2.0 >= 2.66.0"
#endif

#define _BISMUTH_INSIDE

#include "bis-version.h"
#include "bis-animation.h"
#include "bis-animation-target.h"
#include "bis-animation-util.h"
#include "bis-carousel.h"
#include "bis-carousel-indicator-dots.h"
#include "bis-carousel-indicator-lines.h"
#include "bis-clamp.h"
#include "bis-clamp-layout.h"
#include "bis-clamp-scrollable.h"
#include "bis-deprecation-macros.h"
#include "bis-easing.h"
#include "bis-enum-list-model.h"
#include "bis-lapel.h"
#include "bis-fold-threshold-policy.h"
#include "bis-album.h"
#include "bis-main.h"
#include "bis-navigation-direction.h"
#include "bis-spring-animation.h"
#include "bis-spring-params.h"
#include "bis-hugger.h"
#include "bis-swipe-tracker.h"
#include "bis-swipeable.h"
#include "bis-timed-animation.h"

#undef _BISMUTH_INSIDE

G_END_DECLS
