/*
 * Copyright (C) 2019 Purism SPC
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

G_BEGIN_DECLS

#define BIS_TYPE_SQUEEZER_PAGE (bis_squeezer_page_get_type ())

BIS_AVAILABLE_IN_ALL
G_DECLARE_FINAL_TYPE (BisSqueezerPage, bis_squeezer_page, BIS, SQUEEZER_PAGE, GObject)

BIS_AVAILABLE_IN_ALL
GtkWidget *bis_squeezer_page_get_child (BisSqueezerPage *self);

BIS_AVAILABLE_IN_ALL
gboolean bis_squeezer_page_get_enabled (BisSqueezerPage *self);
BIS_AVAILABLE_IN_ALL
void     bis_squeezer_page_set_enabled (BisSqueezerPage *self,
                                        gboolean         enabled);

#define BIS_TYPE_SQUEEZER (bis_squeezer_get_type ())

BIS_AVAILABLE_IN_ALL
G_DECLARE_FINAL_TYPE (BisSqueezer, bis_squeezer, BIS, SQUEEZER, GtkWidget)

typedef enum {
  BIS_SQUEEZER_TRANSITION_TYPE_NONE,
  BIS_SQUEEZER_TRANSITION_TYPE_CROSSFADE,
} BisSqueezerTransitionType;

BIS_AVAILABLE_IN_ALL
GtkWidget *bis_squeezer_new (void) G_GNUC_WARN_UNUSED_RESULT;

BIS_AVAILABLE_IN_ALL
BisSqueezerPage *bis_squeezer_add    (BisSqueezer *self,
                                      GtkWidget   *child);
BIS_AVAILABLE_IN_ALL
void             bis_squeezer_remove (BisSqueezer *self,
                                      GtkWidget   *child);

BIS_AVAILABLE_IN_ALL
BisSqueezerPage *bis_squeezer_get_page (BisSqueezer *self,
                                        GtkWidget   *child);

BIS_AVAILABLE_IN_ALL
GtkWidget *bis_squeezer_get_visible_child (BisSqueezer *self);

BIS_AVAILABLE_IN_ALL
gboolean bis_squeezer_get_homogeneous (BisSqueezer *self);
BIS_AVAILABLE_IN_ALL
void     bis_squeezer_set_homogeneous (BisSqueezer *self,
                                       gboolean     homogeneous);

BIS_AVAILABLE_IN_ALL
BisFoldThresholdPolicy bis_squeezer_get_switch_threshold_policy (BisSqueezer             *self);
BIS_AVAILABLE_IN_ALL
void                   bis_squeezer_set_switch_threshold_policy (BisSqueezer            *self,
                                                                 BisFoldThresholdPolicy  policy);

BIS_AVAILABLE_IN_ALL
gboolean bis_squeezer_get_allow_none (BisSqueezer *self);
BIS_AVAILABLE_IN_ALL
void     bis_squeezer_set_allow_none (BisSqueezer *self,
                                      gboolean     allow_none);

BIS_AVAILABLE_IN_ALL
guint bis_squeezer_get_transition_duration (BisSqueezer *self);
BIS_AVAILABLE_IN_ALL
void  bis_squeezer_set_transition_duration (BisSqueezer *self,
                                            guint        duration);

BIS_AVAILABLE_IN_ALL
BisSqueezerTransitionType bis_squeezer_get_transition_type (BisSqueezer               *self);
BIS_AVAILABLE_IN_ALL
void                      bis_squeezer_set_transition_type (BisSqueezer               *self,
                                                            BisSqueezerTransitionType  transition);

BIS_AVAILABLE_IN_ALL
gboolean bis_squeezer_get_transition_running (BisSqueezer *self);

BIS_AVAILABLE_IN_ALL
gboolean bis_squeezer_get_interpolate_size (BisSqueezer *self);
BIS_AVAILABLE_IN_ALL
void     bis_squeezer_set_interpolate_size (BisSqueezer *self,
                                            gboolean     interpolate_size);

BIS_AVAILABLE_IN_ALL
float bis_squeezer_get_xalign (BisSqueezer *self);
BIS_AVAILABLE_IN_ALL
void  bis_squeezer_set_xalign (BisSqueezer *self,
                               float        xalign);

BIS_AVAILABLE_IN_ALL
float bis_squeezer_get_yalign (BisSqueezer *self);
BIS_AVAILABLE_IN_ALL
void  bis_squeezer_set_yalign (BisSqueezer *self,
                               float        yalign);

BIS_AVAILABLE_IN_ALL
GtkSelectionModel *bis_squeezer_get_pages (BisSqueezer *self) G_GNUC_WARN_UNUSED_RESULT;

G_END_DECLS
