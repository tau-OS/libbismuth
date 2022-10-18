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

G_BEGIN_DECLS

#define BIS_TYPE_ANIMATION_TARGET (bis_animation_target_get_type())

BIS_AVAILABLE_IN_ALL
GDK_DECLARE_INTERNAL_TYPE (BisAnimationTarget, bis_animation_target, BIS, ANIMATION_TARGET, GObject)


/**
 * BisAnimationTargetFunc:
 * @value: The animation value
 * @user_data: (nullable): The user data provided when creating the target
 *
 * Prototype for animation targets based on user callbacks.
 *
 * Since: 1.0
 */
typedef void (*BisAnimationTargetFunc) (double   value,
                                        gpointer user_data);

#define BIS_TYPE_CALLBACK_ANIMATION_TARGET (bis_callback_animation_target_get_type())

BIS_AVAILABLE_IN_ALL
GDK_DECLARE_INTERNAL_TYPE (BisCallbackAnimationTarget, bis_callback_animation_target, BIS, CALLBACK_ANIMATION_TARGET, BisAnimationTarget)

BIS_AVAILABLE_IN_ALL
BisAnimationTarget *bis_callback_animation_target_new (BisAnimationTargetFunc callback,
                                                       gpointer               user_data,
                                                       GDestroyNotify         destroy) G_GNUC_WARN_UNUSED_RESULT;

#define BIS_TYPE_PROPERTY_ANIMATION_TARGET (bis_property_animation_target_get_type())

BIS_AVAILABLE_IN_1_2
GDK_DECLARE_INTERNAL_TYPE (BisPropertyAnimationTarget, bis_property_animation_target, BIS, PROPERTY_ANIMATION_TARGET, BisAnimationTarget)

BIS_AVAILABLE_IN_1_2
BisAnimationTarget *bis_property_animation_target_new           (GObject    *object,
                                                                 const char *property_name) G_GNUC_WARN_UNUSED_RESULT;
BIS_AVAILABLE_IN_1_2
BisAnimationTarget *bis_property_animation_target_new_for_pspec (GObject    *object,
                                                                 GParamSpec *pspec) G_GNUC_WARN_UNUSED_RESULT;

BIS_AVAILABLE_IN_1_2
GObject    *bis_property_animation_target_get_object (BisPropertyAnimationTarget *self);
BIS_AVAILABLE_IN_1_2
GParamSpec *bis_property_animation_target_get_pspec  (BisPropertyAnimationTarget *self);

G_END_DECLS
