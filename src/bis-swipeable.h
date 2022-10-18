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
#include "bis-navigation-direction.h"

G_BEGIN_DECLS

#define BIS_TYPE_SWIPEABLE (bis_swipeable_get_type ())

BIS_AVAILABLE_IN_ALL
G_DECLARE_INTERFACE (BisSwipeable, bis_swipeable, BIS, SWIPEABLE, GtkWidget)

/**
 * BisSwipeableInterface:
 * @parent: The parent interface.
 * @get_distance: Gets the swipe distance.
 * @get_snap_points: Gets the snap points.
 * @get_progress: Gets the current progress.
 * @get_cancel_progress: Gets the cancel progress.
 * @get_swipe_area: Gets the swipeable rectangle.
 *
 * An interface for swipeable widgets.
 *
 * Since: 1.0
 **/
struct _BisSwipeableInterface
{
  GTypeInterface parent;

  double  (*get_distance)        (BisSwipeable *self);
  double *(*get_snap_points)     (BisSwipeable *self,
                                  int          *n_snap_points);
  double  (*get_progress)        (BisSwipeable *self);
  double  (*get_cancel_progress) (BisSwipeable *self);
  void    (*get_swipe_area)      (BisSwipeable           *self,
                                  BisNavigationDirection  navigation_direction,
                                  gboolean                is_drag,
                                  GdkRectangle           *rect);

  /*< private >*/
  gpointer padding[4];
};

BIS_AVAILABLE_IN_ALL
double bis_swipeable_get_distance (BisSwipeable *self);

BIS_AVAILABLE_IN_ALL
double *bis_swipeable_get_snap_points (BisSwipeable *self,
                                       int          *n_snap_points) G_GNUC_WARN_UNUSED_RESULT;

BIS_AVAILABLE_IN_ALL
double bis_swipeable_get_progress (BisSwipeable *self);

BIS_AVAILABLE_IN_ALL
double bis_swipeable_get_cancel_progress (BisSwipeable *self);

BIS_AVAILABLE_IN_ALL
void bis_swipeable_get_swipe_area (BisSwipeable           *self,
                                   BisNavigationDirection  navigation_direction,
                                   gboolean                is_drag,
                                   GdkRectangle           *rect);

G_END_DECLS
