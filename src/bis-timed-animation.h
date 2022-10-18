/*
 * Copyright (C) 2021 Manuel Genovés <manuel.genoves@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once

#if !defined(_BISMUTH_INSIDE) && !defined(BISMUTH_COMPILATION)
#error "Only <bismuth.h> can be included directly."
#endif

#include "bis-version.h"

#include <gtk/gtk.h>

#include "bis-animation.h"
#include "bis-easing.h"

G_BEGIN_DECLS

#define BIS_TYPE_TIMED_ANIMATION (bis_timed_animation_get_type())

BIS_AVAILABLE_IN_ALL
GDK_DECLARE_INTERNAL_TYPE (BisTimedAnimation, bis_timed_animation, BIS, TIMED_ANIMATION, BisAnimation)

BIS_AVAILABLE_IN_ALL
BisAnimation *bis_timed_animation_new (GtkWidget          *widget,
                                       double              from,
                                       double              to,
                                       guint               duration,
                                       BisAnimationTarget *target) G_GNUC_WARN_UNUSED_RESULT;

BIS_AVAILABLE_IN_ALL
double bis_timed_animation_get_value_from (BisTimedAnimation *self);
BIS_AVAILABLE_IN_ALL
void   bis_timed_animation_set_value_from (BisTimedAnimation *self,
                                           double             value);

BIS_AVAILABLE_IN_ALL
double bis_timed_animation_get_value_to (BisTimedAnimation *self);
BIS_AVAILABLE_IN_ALL
void   bis_timed_animation_set_value_to (BisTimedAnimation *self,
                                         double             value);

BIS_AVAILABLE_IN_ALL
guint bis_timed_animation_get_duration (BisTimedAnimation *self);
BIS_AVAILABLE_IN_ALL
void  bis_timed_animation_set_duration (BisTimedAnimation *self,
                                        guint              duration);

BIS_AVAILABLE_IN_ALL
BisEasing bis_timed_animation_get_easing (BisTimedAnimation *self);
BIS_AVAILABLE_IN_ALL
void      bis_timed_animation_set_easing (BisTimedAnimation *self,
                                          BisEasing          easing);

BIS_AVAILABLE_IN_ALL
guint bis_timed_animation_get_repeat_count (BisTimedAnimation *self);
BIS_AVAILABLE_IN_ALL
void  bis_timed_animation_set_repeat_count (BisTimedAnimation *self,
                                            guint              repeat_count);

BIS_AVAILABLE_IN_ALL
gboolean bis_timed_animation_get_reverse (BisTimedAnimation *self);
BIS_AVAILABLE_IN_ALL
void     bis_timed_animation_set_reverse (BisTimedAnimation *self,
                                          gboolean           reverse);

BIS_AVAILABLE_IN_ALL
gboolean bis_timed_animation_get_alternate (BisTimedAnimation *self);
BIS_AVAILABLE_IN_ALL
void     bis_timed_animation_set_alternate (BisTimedAnimation *self,
                                            gboolean           alternate);

G_END_DECLS
