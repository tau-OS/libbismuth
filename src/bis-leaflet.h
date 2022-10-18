/*
 * Copyright (C) 2018 Purism SPC
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
#include "bis-navigation-direction.h"
#include "bis-spring-params.h"

G_BEGIN_DECLS

#define BIS_TYPE_LEAFLET_PAGE (bis_leaflet_page_get_type ())

BIS_AVAILABLE_IN_ALL
G_DECLARE_FINAL_TYPE (BisLeafletPage, bis_leaflet_page, BIS, LEAFLET_PAGE, GObject)

BIS_AVAILABLE_IN_ALL
GtkWidget *bis_leaflet_page_get_child (BisLeafletPage *self);

BIS_AVAILABLE_IN_ALL
const char *bis_leaflet_page_get_name (BisLeafletPage *self);
BIS_AVAILABLE_IN_ALL
void        bis_leaflet_page_set_name (BisLeafletPage *self,
                                       const char     *name);

BIS_AVAILABLE_IN_ALL
gboolean bis_leaflet_page_get_navigatable (BisLeafletPage *self);
BIS_AVAILABLE_IN_ALL
void     bis_leaflet_page_set_navigatable (BisLeafletPage *self,
                                           gboolean        navigatable);

#define BIS_TYPE_LEAFLET (bis_leaflet_get_type())

BIS_AVAILABLE_IN_ALL
G_DECLARE_FINAL_TYPE (BisLeaflet, bis_leaflet, BIS, LEAFLET, GtkWidget)

typedef enum {
  BIS_LEAFLET_TRANSITION_TYPE_OVER,
  BIS_LEAFLET_TRANSITION_TYPE_UNDER,
  BIS_LEAFLET_TRANSITION_TYPE_SLIDE,
} BisLeafletTransitionType;

BIS_AVAILABLE_IN_ALL
GtkWidget *bis_leaflet_new (void) G_GNUC_WARN_UNUSED_RESULT;

BIS_AVAILABLE_IN_ALL
BisLeafletPage *bis_leaflet_append  (BisLeaflet *self,
                                     GtkWidget  *child);
BIS_AVAILABLE_IN_ALL
BisLeafletPage *bis_leaflet_prepend (BisLeaflet *self,
                                     GtkWidget  *child);

BIS_AVAILABLE_IN_ALL
BisLeafletPage *bis_leaflet_insert_child_after  (BisLeaflet *self,
                                                 GtkWidget  *child,
                                                 GtkWidget  *sibling);
BIS_AVAILABLE_IN_ALL
void            bis_leaflet_reorder_child_after (BisLeaflet *self,
                                                  GtkWidget *child,
                                                  GtkWidget *sibling);

BIS_AVAILABLE_IN_ALL
void bis_leaflet_remove (BisLeaflet *self,
                         GtkWidget  *child);

BIS_AVAILABLE_IN_ALL
BisLeafletPage *bis_leaflet_get_page (BisLeaflet *self,
                                      GtkWidget  *child);

BIS_AVAILABLE_IN_ALL
gboolean bis_leaflet_get_can_unfold (BisLeaflet *self);
BIS_AVAILABLE_IN_ALL
void     bis_leaflet_set_can_unfold (BisLeaflet *self,
                                     gboolean    can_unfold);

BIS_AVAILABLE_IN_ALL
gboolean bis_leaflet_get_folded (BisLeaflet *self);

BIS_AVAILABLE_IN_ALL
BisFoldThresholdPolicy bis_leaflet_get_fold_threshold_policy (BisLeaflet             *self);
BIS_AVAILABLE_IN_ALL
void                   bis_leaflet_set_fold_threshold_policy (BisLeaflet             *self,
                                                              BisFoldThresholdPolicy  policy);

BIS_AVAILABLE_IN_ALL
gboolean bis_leaflet_get_homogeneous (BisLeaflet *self);
BIS_AVAILABLE_IN_ALL
void     bis_leaflet_set_homogeneous (BisLeaflet *self,
                                      gboolean    homogeneous);

BIS_AVAILABLE_IN_ALL
GtkWidget *bis_leaflet_get_visible_child (BisLeaflet *self);
BIS_AVAILABLE_IN_ALL
void       bis_leaflet_set_visible_child (BisLeaflet *self,
                                          GtkWidget  *visible_child);

BIS_AVAILABLE_IN_ALL
const char *bis_leaflet_get_visible_child_name (BisLeaflet *self);
BIS_AVAILABLE_IN_ALL
void        bis_leaflet_set_visible_child_name (BisLeaflet *self,
                                                const char *name);

BIS_AVAILABLE_IN_ALL
BisLeafletTransitionType bis_leaflet_get_transition_type (BisLeaflet               *self);
BIS_AVAILABLE_IN_ALL
void                     bis_leaflet_set_transition_type (BisLeaflet               *self,
                                                          BisLeafletTransitionType  transition);

BIS_AVAILABLE_IN_ALL
guint bis_leaflet_get_mode_transition_duration (BisLeaflet *self);
BIS_AVAILABLE_IN_ALL
void  bis_leaflet_set_mode_transition_duration (BisLeaflet *self,
                                                guint       duration);

BIS_AVAILABLE_IN_ALL
BisSpringParams *bis_leaflet_get_child_transition_params (BisLeaflet      *self);
BIS_AVAILABLE_IN_ALL
void             bis_leaflet_set_child_transition_params (BisLeaflet      *self,
                                                          BisSpringParams *params);

BIS_AVAILABLE_IN_ALL
gboolean bis_leaflet_get_child_transition_running (BisLeaflet *self);

BIS_AVAILABLE_IN_ALL
gboolean bis_leaflet_get_can_navigate_back (BisLeaflet *self);
BIS_AVAILABLE_IN_ALL
void     bis_leaflet_set_can_navigate_back (BisLeaflet *self,
                                            gboolean    can_navigate_back);

BIS_AVAILABLE_IN_ALL
gboolean bis_leaflet_get_can_navigate_forward (BisLeaflet *self);
BIS_AVAILABLE_IN_ALL
void     bis_leaflet_set_can_navigate_forward (BisLeaflet *self,
                                               gboolean    can_navigate_forward);

BIS_AVAILABLE_IN_ALL
GtkWidget *bis_leaflet_get_adjacent_child (BisLeaflet             *self,
                                           BisNavigationDirection  direction);
BIS_AVAILABLE_IN_ALL
gboolean   bis_leaflet_navigate           (BisLeaflet             *self,
                                           BisNavigationDirection  direction);

BIS_AVAILABLE_IN_ALL
GtkWidget *bis_leaflet_get_child_by_name (BisLeaflet *self,
                                          const char *name);

BIS_AVAILABLE_IN_ALL
GtkSelectionModel *bis_leaflet_get_pages (BisLeaflet *self) G_GNUC_WARN_UNUSED_RESULT;

G_END_DECLS
