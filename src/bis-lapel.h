/*
 * Copyright (C) 2020 Felix Häcker <haeckerfelix@gnome.org>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once

#if !defined(_BISMUTH_INSIDE) && !defined(BISMUTH_COMPILATION)
#error "Only <bismuth.h> can be included directly."
#endif

#include "bis-version.h"

#include <gtk/gtk.h>
#include "bis-enums.h"
#include "bis-fold-threshold-policy.h"
#include "bis-spring-params.h"

G_BEGIN_DECLS

#define BIS_TYPE_LAPEL (bis_lapel_get_type ())

BIS_AVAILABLE_IN_ALL
G_DECLARE_FINAL_TYPE (BisLapel, bis_lapel, BIS, LAPEL, GtkWidget)

typedef enum {
  BIS_LAPEL_FOLD_POLICY_NEVER,
  BIS_LAPEL_FOLD_POLICY_ALWAYS,
  BIS_LAPEL_FOLD_POLICY_AUTO,
} BisLapelFoldPolicy;

typedef enum {
  BIS_LAPEL_TRANSITION_TYPE_OVER,
  BIS_LAPEL_TRANSITION_TYPE_UNDER,
  BIS_LAPEL_TRANSITION_TYPE_SLIDE,
} BisLapelTransitionType;

BIS_AVAILABLE_IN_ALL
GtkWidget *bis_lapel_new (void) G_GNUC_WARN_UNUSED_RESULT;

BIS_AVAILABLE_IN_ALL
GtkWidget *bis_lapel_get_content (BisLapel   *self);
BIS_AVAILABLE_IN_ALL
void       bis_lapel_set_content (BisLapel   *self,
                                 GtkWidget *content);

BIS_AVAILABLE_IN_ALL
GtkWidget *bis_lapel_get_lapel (BisLapel   *self);
BIS_AVAILABLE_IN_ALL
void       bis_lapel_set_lapel (BisLapel   *self,
                              GtkWidget *lapel);

BIS_AVAILABLE_IN_ALL
GtkWidget *bis_lapel_get_separator (BisLapel   *self);
BIS_AVAILABLE_IN_ALL
void       bis_lapel_set_separator (BisLapel   *self,
                                   GtkWidget *separator);

BIS_AVAILABLE_IN_ALL
GtkPackType bis_lapel_get_lapel_position (BisLapel     *self);
BIS_AVAILABLE_IN_ALL
void        bis_lapel_set_lapel_position (BisLapel     *self,
                                        GtkPackType  position);

BIS_AVAILABLE_IN_ALL
gboolean bis_lapel_get_reveal_lapel (BisLapel  *self);
BIS_AVAILABLE_IN_ALL
void     bis_lapel_set_reveal_lapel (BisLapel  *self,
                                   gboolean  reveal_lapel);

BIS_AVAILABLE_IN_ALL
BisSpringParams *bis_lapel_get_reveal_params (BisLapel         *self);
BIS_AVAILABLE_IN_ALL
void             bis_lapel_set_reveal_params (BisLapel         *self,
                                             BisSpringParams *params);

BIS_AVAILABLE_IN_ALL
double bis_lapel_get_reveal_progress (BisLapel *self);

BIS_AVAILABLE_IN_ALL
BisLapelFoldPolicy bis_lapel_get_fold_policy (BisLapel           *self);
BIS_AVAILABLE_IN_ALL
void              bis_lapel_set_fold_policy (BisLapel           *self,
                                            BisLapelFoldPolicy  policy);

BIS_AVAILABLE_IN_ALL
BisFoldThresholdPolicy bis_lapel_get_fold_threshold_policy (BisLapel                *self);
BIS_AVAILABLE_IN_ALL
void                   bis_lapel_set_fold_threshold_policy (BisLapel                *self,
                                                           BisFoldThresholdPolicy  policy);

BIS_AVAILABLE_IN_ALL
guint bis_lapel_get_fold_duration (BisLapel *self);
BIS_AVAILABLE_IN_ALL
void  bis_lapel_set_fold_duration (BisLapel *self,
                                  guint    duration);

BIS_AVAILABLE_IN_ALL
gboolean bis_lapel_get_folded (BisLapel *self);

BIS_AVAILABLE_IN_ALL
gboolean bis_lapel_get_locked (BisLapel *self);
BIS_AVAILABLE_IN_ALL
void     bis_lapel_set_locked (BisLapel  *self,
                              gboolean  locked);

BIS_AVAILABLE_IN_ALL
BisLapelTransitionType bis_lapel_get_transition_type (BisLapel               *self);
BIS_AVAILABLE_IN_ALL
void                  bis_lapel_set_transition_type (BisLapel               *self,
                                                    BisLapelTransitionType  transition_type);

BIS_AVAILABLE_IN_ALL
gboolean bis_lapel_get_modal (BisLapel  *self);
BIS_AVAILABLE_IN_ALL
void     bis_lapel_set_modal (BisLapel  *self,
                             gboolean  modal);

BIS_AVAILABLE_IN_ALL
gboolean bis_lapel_get_swipe_to_open (BisLapel  *self);
BIS_AVAILABLE_IN_ALL
void     bis_lapel_set_swipe_to_open (BisLapel  *self,
                                     gboolean  swipe_to_open);

BIS_AVAILABLE_IN_ALL
gboolean bis_lapel_get_swipe_to_close (BisLapel  *self);
BIS_AVAILABLE_IN_ALL
void     bis_lapel_set_swipe_to_close (BisLapel  *self,
                                      gboolean  swipe_to_close);

G_END_DECLS
