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
#include "bis-about-window.h"
#include "bis-action-row.h"
#include "bis-animation.h"
#include "bis-animation-target.h"
#include "bis-animation-util.h"
#include "bis-application.h"
#include "bis-application-window.h"
#include "bis-avatar.h"
#include "bis-bin.h"
#include "bis-button-content.h"
#include "bis-carousel.h"
#include "bis-carousel-indicator-dots.h"
#include "bis-carousel-indicator-lines.h"
#include "bis-clamp.h"
#include "bis-clamp-layout.h"
#include "bis-clamp-scrollable.h"
#include "bis-combo-row.h"
#include "bis-deprecation-macros.h"
#include "bis-easing.h"
#include "bis-entry-row.h"
#include "bis-enum-list-model.h"
#include "bis-expander-row.h"
#include "bis-lapel.h"
#include "bis-fold-threshold-policy.h"
#include "bis-header-bar.h"
#include "bis-album.h"
#include "bis-main.h"
#include "bis-message-dialog.h"
#include "bis-navigation-direction.h"
#include "bis-password-entry-row.h"
#include "bis-preferences-group.h"
#include "bis-preferences-page.h"
#include "bis-preferences-row.h"
#include "bis-preferences-window.h"
#include "bis-split-button.h"
#include "bis-spring-animation.h"
#include "bis-spring-params.h"
#include "bis-hugger.h"
#include "bis-status-page.h"
#include "bis-style-manager.h"
#include "bis-swipe-tracker.h"
#include "bis-swipeable.h"
#include "bis-tab-bar.h"
#include "bis-tab-button.h"
#include "bis-tab-overview.h"
#include "bis-tab-view.h"
#include "bis-timed-animation.h"
#include "bis-toast-overlay.h"
#include "bis-toast.h"
#include "bis-view-stack.h"
#include "bis-view-switcher.h"
#include "bis-view-switcher-bar.h"
#include "bis-view-switcher-title.h"
#include "bis-window.h"
#include "bis-window-title.h"

#undef _BISMUTH_INSIDE

G_END_DECLS
