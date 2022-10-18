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
#include "bis-spring-params.h"

G_BEGIN_DECLS

#define BIS_TYPE_SPRING_ANIMATION (bis_spring_animation_get_type())

BIS_AVAILABLE_IN_ALL
GDK_DECLARE_INTERNAL_TYPE (BisSpringAnimation, bis_spring_animation, BIS, SPRING_ANIMATION, BisAnimation)

BIS_AVAILABLE_IN_ALL
BisAnimation *bis_spring_animation_new (GtkWidget          *widget,
                                        double              from,
                                        double              to,
                                        BisSpringParams    *spring_params,
                                        BisAnimationTarget *target) G_GNUC_WARN_UNUSED_RESULT;

BIS_AVAILABLE_IN_ALL
double bis_spring_animation_get_value_from (BisSpringAnimation *self);
BIS_AVAILABLE_IN_ALL
void   bis_spring_animation_set_value_from (BisSpringAnimation *self,
                                            double             value);

BIS_AVAILABLE_IN_ALL
double bis_spring_animation_get_value_to (BisSpringAnimation *self);
BIS_AVAILABLE_IN_ALL
void   bis_spring_animation_set_value_to (BisSpringAnimation *self,
                                          double             value);

BIS_AVAILABLE_IN_ALL
BisSpringParams *bis_spring_animation_get_spring_params (BisSpringAnimation *self);
BIS_AVAILABLE_IN_ALL
void             bis_spring_animation_set_spring_params (BisSpringAnimation *self,
                                                         BisSpringParams    *spring_params);

BIS_AVAILABLE_IN_ALL
double bis_spring_animation_get_initial_velocity (BisSpringAnimation *self);
BIS_AVAILABLE_IN_ALL
void   bis_spring_animation_set_initial_velocity (BisSpringAnimation *self,
                                                  double              velocity);

BIS_AVAILABLE_IN_ALL
double bis_spring_animation_get_epsilon (BisSpringAnimation *self);
BIS_AVAILABLE_IN_ALL
void   bis_spring_animation_set_epsilon (BisSpringAnimation *self,
                                         double              epsilon);

BIS_AVAILABLE_IN_ALL
gboolean bis_spring_animation_get_clamp (BisSpringAnimation *self);
BIS_AVAILABLE_IN_ALL
void     bis_spring_animation_set_clamp (BisSpringAnimation *self,
                                         gboolean            clamp);

BIS_AVAILABLE_IN_ALL
guint bis_spring_animation_get_estimated_duration (BisSpringAnimation *self);

BIS_AVAILABLE_IN_ALL
double bis_spring_animation_get_velocity (BisSpringAnimation *self);

G_END_DECLS
