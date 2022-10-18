/*
 * Copyright (C) 2019 Purism SPC
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

#include "bis-animation-target.h"
#include "bis-enums.h"

G_BEGIN_DECLS

/**
 * BIS_DURATION_INFINITE:
 *
 * Indicates an [class@Animation] with an infinite duration.
 *
 * This value is mostly used internally.
 */

#define BIS_DURATION_INFINITE ((guint) 0xffffffff)

#define BIS_TYPE_ANIMATION (bis_animation_get_type())

BIS_AVAILABLE_IN_ALL
G_DECLARE_DERIVABLE_TYPE (BisAnimation, bis_animation, BIS, ANIMATION, GObject)

typedef enum {
  BIS_ANIMATION_IDLE,
  BIS_ANIMATION_PAUSED,
  BIS_ANIMATION_PLAYING,
  BIS_ANIMATION_FINISHED,
} BisAnimationState;

BIS_AVAILABLE_IN_ALL
GtkWidget *bis_animation_get_widget (BisAnimation *self);

BIS_AVAILABLE_IN_ALL
BisAnimationTarget *bis_animation_get_target (BisAnimation       *self);
BIS_AVAILABLE_IN_ALL
void                bis_animation_set_target (BisAnimation       *self,
                                              BisAnimationTarget *target);

BIS_AVAILABLE_IN_ALL
double bis_animation_get_value (BisAnimation *self);

BIS_AVAILABLE_IN_ALL
BisAnimationState bis_animation_get_state (BisAnimation *self);

BIS_AVAILABLE_IN_ALL
void bis_animation_play   (BisAnimation *self);
BIS_AVAILABLE_IN_ALL
void bis_animation_pause  (BisAnimation *self);
BIS_AVAILABLE_IN_ALL
void bis_animation_resume (BisAnimation *self);
BIS_AVAILABLE_IN_ALL
void bis_animation_reset  (BisAnimation *self);
BIS_AVAILABLE_IN_ALL
void bis_animation_skip   (BisAnimation *self);

G_END_DECLS
