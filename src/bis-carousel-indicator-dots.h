/*
 * Copyright (C) 2020 Alexander Mikhaylenko <alexm@gnome.org>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once

#if !defined(_BISMUTH_INSIDE) && !defined(BISMUTH_COMPILATION)
#error "Only <bismuth.h> can be included directly."
#endif

#include "bis-version.h"

#include <gtk/gtk.h>
#include "bis-carousel.h"

G_BEGIN_DECLS

#define BIS_TYPE_CAROUSEL_INDICATOR_DOTS (bis_carousel_indicator_dots_get_type())

BIS_AVAILABLE_IN_ALL
G_DECLARE_FINAL_TYPE (BisCarouselIndicatorDots, bis_carousel_indicator_dots, BIS, CAROUSEL_INDICATOR_DOTS, GtkWidget)

BIS_AVAILABLE_IN_ALL
GtkWidget *bis_carousel_indicator_dots_new (void) G_GNUC_WARN_UNUSED_RESULT;

BIS_AVAILABLE_IN_ALL
BisCarousel *bis_carousel_indicator_dots_get_carousel (BisCarouselIndicatorDots *self);
BIS_AVAILABLE_IN_ALL
void         bis_carousel_indicator_dots_set_carousel (BisCarouselIndicatorDots *self,
                                                       BisCarousel              *carousel);

G_END_DECLS
