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

#define BIS_TYPE_FLAP (bis_flap_get_type ())

BIS_AVAILABLE_IN_ALL
G_DECLARE_FINAL_TYPE (BisFlap, bis_flap, BIS, FLAP, GtkWidget)

typedef enum {
  BIS_FLAP_FOLD_POLICY_NEVER,
  BIS_FLAP_FOLD_POLICY_ALWAYS,
  BIS_FLAP_FOLD_POLICY_AUTO,
} BisFlapFoldPolicy;

typedef enum {
  BIS_FLAP_TRANSITION_TYPE_OVER,
  BIS_FLAP_TRANSITION_TYPE_UNDER,
  BIS_FLAP_TRANSITION_TYPE_SLIDE,
} BisFlapTransitionType;

BIS_AVAILABLE_IN_ALL
GtkWidget *bis_flap_new (void) G_GNUC_WARN_UNUSED_RESULT;

BIS_AVAILABLE_IN_ALL
GtkWidget *bis_flap_get_content (BisFlap   *self);
BIS_AVAILABLE_IN_ALL
void       bis_flap_set_content (BisFlap   *self,
                                 GtkWidget *content);

BIS_AVAILABLE_IN_ALL
GtkWidget *bis_flap_get_flap (BisFlap   *self);
BIS_AVAILABLE_IN_ALL
void       bis_flap_set_flap (BisFlap   *self,
                              GtkWidget *flap);

BIS_AVAILABLE_IN_ALL
GtkWidget *bis_flap_get_separator (BisFlap   *self);
BIS_AVAILABLE_IN_ALL
void       bis_flap_set_separator (BisFlap   *self,
                                   GtkWidget *separator);

BIS_AVAILABLE_IN_ALL
GtkPackType bis_flap_get_flap_position (BisFlap     *self);
BIS_AVAILABLE_IN_ALL
void        bis_flap_set_flap_position (BisFlap     *self,
                                        GtkPackType  position);

BIS_AVAILABLE_IN_ALL
gboolean bis_flap_get_reveal_flap (BisFlap  *self);
BIS_AVAILABLE_IN_ALL
void     bis_flap_set_reveal_flap (BisFlap  *self,
                                   gboolean  reveal_flap);

BIS_AVAILABLE_IN_ALL
BisSpringParams *bis_flap_get_reveal_params (BisFlap         *self);
BIS_AVAILABLE_IN_ALL
void             bis_flap_set_reveal_params (BisFlap         *self,
                                             BisSpringParams *params);

BIS_AVAILABLE_IN_ALL
double bis_flap_get_reveal_progress (BisFlap *self);

BIS_AVAILABLE_IN_ALL
BisFlapFoldPolicy bis_flap_get_fold_policy (BisFlap           *self);
BIS_AVAILABLE_IN_ALL
void              bis_flap_set_fold_policy (BisFlap           *self,
                                            BisFlapFoldPolicy  policy);

BIS_AVAILABLE_IN_ALL
BisFoldThresholdPolicy bis_flap_get_fold_threshold_policy (BisFlap                *self);
BIS_AVAILABLE_IN_ALL
void                   bis_flap_set_fold_threshold_policy (BisFlap                *self,
                                                           BisFoldThresholdPolicy  policy);

BIS_AVAILABLE_IN_ALL
guint bis_flap_get_fold_duration (BisFlap *self);
BIS_AVAILABLE_IN_ALL
void  bis_flap_set_fold_duration (BisFlap *self,
                                  guint    duration);

BIS_AVAILABLE_IN_ALL
gboolean bis_flap_get_folded (BisFlap *self);

BIS_AVAILABLE_IN_ALL
gboolean bis_flap_get_locked (BisFlap *self);
BIS_AVAILABLE_IN_ALL
void     bis_flap_set_locked (BisFlap  *self,
                              gboolean  locked);

BIS_AVAILABLE_IN_ALL
BisFlapTransitionType bis_flap_get_transition_type (BisFlap               *self);
BIS_AVAILABLE_IN_ALL
void                  bis_flap_set_transition_type (BisFlap               *self,
                                                    BisFlapTransitionType  transition_type);

BIS_AVAILABLE_IN_ALL
gboolean bis_flap_get_modal (BisFlap  *self);
BIS_AVAILABLE_IN_ALL
void     bis_flap_set_modal (BisFlap  *self,
                             gboolean  modal);

BIS_AVAILABLE_IN_ALL
gboolean bis_flap_get_swipe_to_open (BisFlap  *self);
BIS_AVAILABLE_IN_ALL
void     bis_flap_set_swipe_to_open (BisFlap  *self,
                                     gboolean  swipe_to_open);

BIS_AVAILABLE_IN_ALL
gboolean bis_flap_get_swipe_to_close (BisFlap  *self);
BIS_AVAILABLE_IN_ALL
void     bis_flap_set_swipe_to_close (BisFlap  *self,
                                      gboolean  swipe_to_close);

G_END_DECLS
