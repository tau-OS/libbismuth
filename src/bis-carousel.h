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

#include "bis-spring-params.h"

G_BEGIN_DECLS

#define BIS_TYPE_CAROUSEL (bis_carousel_get_type())

BIS_AVAILABLE_IN_ALL
G_DECLARE_FINAL_TYPE (BisCarousel, bis_carousel, BIS, CAROUSEL, GtkWidget)

BIS_AVAILABLE_IN_ALL
GtkWidget *bis_carousel_new (void) G_GNUC_WARN_UNUSED_RESULT;

BIS_AVAILABLE_IN_ALL
void bis_carousel_prepend (BisCarousel *self,
                           GtkWidget   *child);
BIS_AVAILABLE_IN_ALL
void bis_carousel_append  (BisCarousel *self,
                           GtkWidget   *child);
BIS_AVAILABLE_IN_ALL
void bis_carousel_insert  (BisCarousel *self,
                           GtkWidget   *child,
                           int          position);

BIS_AVAILABLE_IN_ALL
void bis_carousel_reorder (BisCarousel *self,
                           GtkWidget   *child,
                           int          position);

BIS_AVAILABLE_IN_ALL
void bis_carousel_remove (BisCarousel *self,
                          GtkWidget   *child);

BIS_AVAILABLE_IN_ALL
void bis_carousel_scroll_to (BisCarousel *self,
                             GtkWidget   *widget,
                             gboolean     animate);

BIS_AVAILABLE_IN_ALL
GtkWidget *bis_carousel_get_nth_page (BisCarousel *self,
                                      guint        n);
BIS_AVAILABLE_IN_ALL
guint      bis_carousel_get_n_pages  (BisCarousel *self);
BIS_AVAILABLE_IN_ALL
double     bis_carousel_get_position (BisCarousel *self);

BIS_AVAILABLE_IN_ALL
gboolean bis_carousel_get_interactive (BisCarousel *self);
BIS_AVAILABLE_IN_ALL
void     bis_carousel_set_interactive (BisCarousel *self,
                                       gboolean     interactive);

BIS_AVAILABLE_IN_ALL
guint bis_carousel_get_spacing (BisCarousel *self);
BIS_AVAILABLE_IN_ALL
void  bis_carousel_set_spacing (BisCarousel *self,
                                guint        spacing);

BIS_AVAILABLE_IN_ALL
BisSpringParams *bis_carousel_get_scroll_params (BisCarousel     *self);
BIS_AVAILABLE_IN_ALL
void             bis_carousel_set_scroll_params (BisCarousel     *self,
                                                 BisSpringParams *params);

BIS_AVAILABLE_IN_ALL
gboolean bis_carousel_get_allow_mouse_drag (BisCarousel *self);
BIS_AVAILABLE_IN_ALL
void     bis_carousel_set_allow_mouse_drag (BisCarousel *self,
                                            gboolean     allow_mouse_drag);

BIS_AVAILABLE_IN_ALL
gboolean bis_carousel_get_allow_scroll_wheel (BisCarousel *self);
BIS_AVAILABLE_IN_ALL
void     bis_carousel_set_allow_scroll_wheel (BisCarousel *self,
                                              gboolean     allow_scroll_wheel);

BIS_AVAILABLE_IN_ALL
gboolean bis_carousel_get_allow_long_swipes (BisCarousel *self);
BIS_AVAILABLE_IN_ALL
void     bis_carousel_set_allow_long_swipes (BisCarousel *self,
                                             gboolean     allow_long_swipes);

BIS_AVAILABLE_IN_ALL
guint bis_carousel_get_reveal_duration (BisCarousel *self);
BIS_AVAILABLE_IN_ALL
void  bis_carousel_set_reveal_duration (BisCarousel *self,
                                        guint        reveal_duration);
G_END_DECLS
