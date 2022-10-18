/*
 * Copyright (C) 2019 Alexander Mikhaylenko <exalm7659@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "config.h"

#include "bis-swipeable.h"

/**
 * BisSwipeable:
 *
 * An interface for swipeable widgets.
 *
 * The `BisSwipeable` interface is implemented by all swipeable widgets.
 *
 * See [class@SwipeTracker] for details about implementing it.
 *
 * Since: 1.0
 */

G_DEFINE_INTERFACE (BisSwipeable, bis_swipeable, GTK_TYPE_WIDGET)

static void
bis_swipeable_default_init (BisSwipeableInterface *iface)
{
}

/**
 * bis_swipeable_get_distance:
 * @self: a swipeable
 *
 * Gets the swipe distance of @self.
 *
 * This corresponds to how many pixels 1 unit represents.
 *
 * Returns: the swipe distance in pixels
 *
 * Since: 1.0
 */
double
bis_swipeable_get_distance (BisSwipeable *self)
{
  BisSwipeableInterface *iface;

  g_return_val_if_fail (BIS_IS_SWIPEABLE (self), 0);

  iface = BIS_SWIPEABLE_GET_IFACE (self);
  g_return_val_if_fail (iface->get_distance != NULL, 0);

  return iface->get_distance (self);
}

/**
 * bis_swipeable_get_snap_points: (virtual get_snap_points)
 * @self: a swipeable
 * @n_snap_points: (out): location to return the number of the snap points
 *
 * Gets the snap points of @self.
 *
 * Each snap point represents a progress value that is considered acceptable to
 * end the swipe on.
 *
 * Returns: (array length=n_snap_points) (transfer full): the snap points
 *
 * Since: 1.0
 */
double *
bis_swipeable_get_snap_points (BisSwipeable *self,
                               int          *n_snap_points)
{
  BisSwipeableInterface *iface;

  g_return_val_if_fail (BIS_IS_SWIPEABLE (self), NULL);

  iface = BIS_SWIPEABLE_GET_IFACE (self);
  g_return_val_if_fail (iface->get_snap_points != NULL, NULL);

  return iface->get_snap_points (self, n_snap_points);
}

/**
 * bis_swipeable_get_progress:
 * @self: a swipeable
 *
 * Gets the current progress of @self.
 *
 * Returns: the current progress, unitless
 *
 * Since: 1.0
 */
double
bis_swipeable_get_progress (BisSwipeable *self)
{
  BisSwipeableInterface *iface;

  g_return_val_if_fail (BIS_IS_SWIPEABLE (self), 0);

  iface = BIS_SWIPEABLE_GET_IFACE (self);
  g_return_val_if_fail (iface->get_progress != NULL, 0);

  return iface->get_progress (self);
}

/**
 * bis_swipeable_get_cancel_progress:
 * @self: a swipeable
 *
 * Gets the progress @self will snap back to after the gesture is canceled.
 *
 * Returns: the cancel progress, unitless
 *
 * Since: 1.0
 */
double
bis_swipeable_get_cancel_progress (BisSwipeable *self)
{
  BisSwipeableInterface *iface;

  g_return_val_if_fail (BIS_IS_SWIPEABLE (self), 0);

  iface = BIS_SWIPEABLE_GET_IFACE (self);
  g_return_val_if_fail (iface->get_cancel_progress != NULL, 0);

  return iface->get_cancel_progress (self);
}

/**
 * bis_swipeable_get_swipe_area:
 * @self: a swipeable
 * @navigation_direction: the direction of the swipe
 * @is_drag: whether the swipe is caused by a dragging gesture
 * @rect: (out): a pointer to a rectangle to store the swipe area
 *
 * Gets the area @self can start a swipe from for the given direction and
 * gesture type.
 *
 * This can be used to restrict swipes to only be possible from a certain area,
 * for example, to only allow edge swipes, or to have a draggable element and
 * ignore swipes elsewhere.
 *
 * If not implemented, the default implementation returns the allocation of
 * @self, allowing swipes from anywhere.
 *
 * Since: 1.0
 */
void
bis_swipeable_get_swipe_area (BisSwipeable           *self,
                              BisNavigationDirection  navigation_direction,
                              gboolean                is_drag,
                              GdkRectangle           *rect)
{
  BisSwipeableInterface *iface;

  g_return_if_fail (BIS_IS_SWIPEABLE (self));
  g_return_if_fail (rect != NULL);

  iface = BIS_SWIPEABLE_GET_IFACE (self);

  if (iface->get_swipe_area) {
    iface->get_swipe_area (self, navigation_direction, is_drag, rect);
    return;
  }

  rect->x = 0;
  rect->y = 0;
  rect->width = gtk_widget_get_width (GTK_WIDGET (self));
  rect->height = gtk_widget_get_height (GTK_WIDGET (self));
}
