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

#define BIS_TYPE_ALBUM_PAGE (bis_album_page_get_type ())

BIS_AVAILABLE_IN_ALL
G_DECLARE_FINAL_TYPE (BisAlbumPage, bis_album_page, BIS, ALBUM_PAGE, GObject)

BIS_AVAILABLE_IN_ALL
GtkWidget *bis_album_page_get_child (BisAlbumPage *self);

BIS_AVAILABLE_IN_ALL
const char *bis_album_page_get_name (BisAlbumPage *self);
BIS_AVAILABLE_IN_ALL
void        bis_album_page_set_name (BisAlbumPage *self,
                                       const char     *name);

BIS_AVAILABLE_IN_ALL
gboolean bis_album_page_get_navigatable (BisAlbumPage *self);
BIS_AVAILABLE_IN_ALL
void     bis_album_page_set_navigatable (BisAlbumPage *self,
                                           gboolean        navigatable);

#define BIS_TYPE_ALBUM (bis_album_get_type())

BIS_AVAILABLE_IN_ALL
G_DECLARE_FINAL_TYPE (BisAlbum, bis_album, BIS, ALBUM, GtkWidget)

typedef enum {
  BIS_ALBUM_TRANSITION_TYPE_OVER,
  BIS_ALBUM_TRANSITION_TYPE_UNDER,
  BIS_ALBUM_TRANSITION_TYPE_SLIDE,
} BisAlbumTransitionType;

BIS_AVAILABLE_IN_ALL
GtkWidget *bis_album_new (void) G_GNUC_WARN_UNUSED_RESULT;

BIS_AVAILABLE_IN_ALL
BisAlbumPage *bis_album_append  (BisAlbum *self,
                                     GtkWidget  *child);
BIS_AVAILABLE_IN_ALL
BisAlbumPage *bis_album_prepend (BisAlbum *self,
                                     GtkWidget  *child);

BIS_AVAILABLE_IN_ALL
BisAlbumPage *bis_album_insert_child_after  (BisAlbum *self,
                                                 GtkWidget  *child,
                                                 GtkWidget  *sibling);
BIS_AVAILABLE_IN_ALL
void            bis_album_reorder_child_after (BisAlbum *self,
                                                  GtkWidget *child,
                                                  GtkWidget *sibling);

BIS_AVAILABLE_IN_ALL
void bis_album_remove (BisAlbum *self,
                         GtkWidget  *child);

BIS_AVAILABLE_IN_ALL
BisAlbumPage *bis_album_get_page (BisAlbum *self,
                                      GtkWidget  *child);

BIS_AVAILABLE_IN_ALL
gboolean bis_album_get_can_unfold (BisAlbum *self);
BIS_AVAILABLE_IN_ALL
void     bis_album_set_can_unfold (BisAlbum *self,
                                     gboolean    can_unfold);

BIS_AVAILABLE_IN_ALL
gboolean bis_album_get_folded (BisAlbum *self);

BIS_AVAILABLE_IN_ALL
BisFoldThresholdPolicy bis_album_get_fold_threshold_policy (BisAlbum             *self);
BIS_AVAILABLE_IN_ALL
void                   bis_album_set_fold_threshold_policy (BisAlbum             *self,
                                                              BisFoldThresholdPolicy  policy);

BIS_AVAILABLE_IN_ALL
gboolean bis_album_get_homogeneous (BisAlbum *self);
BIS_AVAILABLE_IN_ALL
void     bis_album_set_homogeneous (BisAlbum *self,
                                      gboolean    homogeneous);

BIS_AVAILABLE_IN_ALL
GtkWidget *bis_album_get_visible_child (BisAlbum *self);
BIS_AVAILABLE_IN_ALL
void       bis_album_set_visible_child (BisAlbum *self,
                                          GtkWidget  *visible_child);

BIS_AVAILABLE_IN_ALL
const char *bis_album_get_visible_child_name (BisAlbum *self);
BIS_AVAILABLE_IN_ALL
void        bis_album_set_visible_child_name (BisAlbum *self,
                                                const char *name);

BIS_AVAILABLE_IN_ALL
BisAlbumTransitionType bis_album_get_transition_type (BisAlbum               *self);
BIS_AVAILABLE_IN_ALL
void                     bis_album_set_transition_type (BisAlbum               *self,
                                                          BisAlbumTransitionType  transition);

BIS_AVAILABLE_IN_ALL
guint bis_album_get_mode_transition_duration (BisAlbum *self);
BIS_AVAILABLE_IN_ALL
void  bis_album_set_mode_transition_duration (BisAlbum *self,
                                                guint       duration);

BIS_AVAILABLE_IN_ALL
BisSpringParams *bis_album_get_child_transition_params (BisAlbum      *self);
BIS_AVAILABLE_IN_ALL
void             bis_album_set_child_transition_params (BisAlbum      *self,
                                                          BisSpringParams *params);

BIS_AVAILABLE_IN_ALL
gboolean bis_album_get_child_transition_running (BisAlbum *self);

BIS_AVAILABLE_IN_ALL
gboolean bis_album_get_can_navigate_back (BisAlbum *self);
BIS_AVAILABLE_IN_ALL
void     bis_album_set_can_navigate_back (BisAlbum *self,
                                            gboolean    can_navigate_back);

BIS_AVAILABLE_IN_ALL
gboolean bis_album_get_can_navigate_forward (BisAlbum *self);
BIS_AVAILABLE_IN_ALL
void     bis_album_set_can_navigate_forward (BisAlbum *self,
                                               gboolean    can_navigate_forward);

BIS_AVAILABLE_IN_ALL
GtkWidget *bis_album_get_adjacent_child (BisAlbum             *self,
                                           BisNavigationDirection  direction);
BIS_AVAILABLE_IN_ALL
gboolean   bis_album_navigate           (BisAlbum             *self,
                                           BisNavigationDirection  direction);

BIS_AVAILABLE_IN_ALL
GtkWidget *bis_album_get_child_by_name (BisAlbum *self,
                                          const char *name);

BIS_AVAILABLE_IN_ALL
GtkSelectionModel *bis_album_get_pages (BisAlbum *self) G_GNUC_WARN_UNUSED_RESULT;

G_END_DECLS
