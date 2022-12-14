/*
 * Copyright (C) 2018-2020 Purism SPC
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "config.h"
#include "bis-latch-layout.h"

#include <math.h>

#include "bis-animation-util.h"
#include "bis-easing.h"
#include "bis-macros-private.h"

/**
 * BisLatchLayout:
 *
 * A layout manager constraining its children to a given size.
 *
 * <picture>
 *   <source srcset="latch-wide-dark.png" media="(prefers-color-scheme: dark)">
 *   <img src="latch-wide.png" alt="latch-wide">
 * </picture>
 * <picture>
 *   <source srcset="latch-narrow-dark.png" media="(prefers-color-scheme: dark)">
 *   <img src="latch-narrow.png" alt="latch-narrow">
 * </picture>
 *
 * `BisLatchLayout` constraints the size of the widgets it contains to a given
 * maximum size. It will constrain the width if it is horizontal, or the height
 * if it is vertical. The expansion of the children from their minimum to their
 * maximum size is eased out for a smooth transition.
 *
 * If a child requires more than the requested maximum size, it will be
 * allocated the minimum size it can fit in instead.
 *
 * Each child will get the style  classes .large when it reached its maximum
 * size, .small when it's allocated the full size, .medium in-between, or none
 * if it hasn't been allocated yet.
 *
 * Since: 1.0
 */

#define BIS_EASE_OUT_TAN_CUBIC 3

enum {
  PROP_0,
  PROP_MAXIMUM_SIZE,
  PROP_TIGHTENING_THRESHOLD,

  /* Overridden properties */
  PROP_ORIENTATION,

  LAST_PROP = PROP_TIGHTENING_THRESHOLD + 1,
};

struct _BisLatchLayout
{
  GtkLayoutManager parent_instance;

  int maximum_size;
  int tightening_threshold;

  GtkOrientation orientation;
};

static GParamSpec *props[LAST_PROP];

G_DEFINE_FINAL_TYPE_WITH_CODE (BisLatchLayout, bis_latch_layout, GTK_TYPE_LAYOUT_MANAGER,
                               G_IMPLEMENT_INTERFACE (GTK_TYPE_ORIENTABLE, NULL))

static void
set_orientation (BisLatchLayout *self,
                 GtkOrientation  orientation)
{
  if (self->orientation == orientation)
    return;

  self->orientation = orientation;

  gtk_layout_manager_layout_changed (GTK_LAYOUT_MANAGER (self));

  g_object_notify (G_OBJECT (self), "orientation");
}

static void
bis_latch_layout_get_property (GObject    *object,
                               guint       prop_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  BisLatchLayout *self = BIS_LATCH_LAYOUT (object);

  switch (prop_id) {
  case PROP_MAXIMUM_SIZE:
    g_value_set_int (value, bis_latch_layout_get_maximum_size (self));
    break;
  case PROP_TIGHTENING_THRESHOLD:
    g_value_set_int (value, bis_latch_layout_get_tightening_threshold (self));
    break;
  case PROP_ORIENTATION:
    g_value_set_enum (value, self->orientation);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
bis_latch_layout_set_property (GObject      *object,
                               guint         prop_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  BisLatchLayout *self = BIS_LATCH_LAYOUT (object);

  switch (prop_id) {
  case PROP_MAXIMUM_SIZE:
    bis_latch_layout_set_maximum_size (self, g_value_get_int (value));
    break;
  case PROP_TIGHTENING_THRESHOLD:
    bis_latch_layout_set_tightening_threshold (self, g_value_get_int (value));
    break;
  case PROP_ORIENTATION:
    set_orientation (self, g_value_get_enum (value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static inline double
inverse_lerp (double a,
              double b,
              double t)
{
  return (t - a) / (b - a);
}

static int
latch_size_from_child (BisLatchLayout *self,
                       int             min,
                       int             nat)
{
  int max = 0, lower = 0, upper = 0;
  double progress;

  lower = MAX (MIN (self->tightening_threshold, self->maximum_size), min);
  max = MAX (lower, self->maximum_size);
  upper = lower + BIS_EASE_OUT_TAN_CUBIC * (max - lower);

  if (nat <= lower)
    progress = 0;
  else if (nat >= max)
    progress = 1;
  else {
    double ease = inverse_lerp (lower, max, nat);

    progress = 1 + cbrt (ease - 1); // inverse ease out cubic
  }

  return ceil (bis_lerp (lower, upper, progress));
}

static int
child_size_from_latch (BisLatchLayout *self,
                       GtkWidget      *child,
                       int             for_size,
                       int            *child_maximum,
                       int            *lower_threshold)
{
  int min = 0, nat = 0, max = 0, lower = 0, upper = 0;
  double progress;

  gtk_widget_measure (child, self->orientation, -1, &min, &nat, NULL, NULL);

  lower = MAX (MIN (self->tightening_threshold, self->maximum_size), min);
  max = MAX (lower, self->maximum_size);
  upper = lower + BIS_EASE_OUT_TAN_CUBIC * (max - lower);

  if (child_maximum)
    *child_maximum = max;
  if (lower_threshold)
    *lower_threshold = lower;

  if (for_size < 0)
    return MIN (nat, max);

  if (for_size <= lower)
    return for_size;

  if (for_size >= upper)
    return max;

  progress = inverse_lerp (lower, upper, for_size);

  return bis_lerp (lower, max, bis_easing_ease (BIS_EASE_OUT_CUBIC, progress));
}

static GtkSizeRequestMode
bis_latch_layout_get_request_mode (GtkLayoutManager *manager,
                                   GtkWidget        *widget)
{
  BisLatchLayout *self = BIS_LATCH_LAYOUT (manager);

  return self->orientation == GTK_ORIENTATION_HORIZONTAL ?
    GTK_SIZE_REQUEST_HEIGHT_FOR_WIDTH :
    GTK_SIZE_REQUEST_WIDTH_FOR_HEIGHT;
}

static void
bis_latch_layout_measure (GtkLayoutManager *manager,
                          GtkWidget        *widget,
                          GtkOrientation    orientation,
                          int               for_size,
                          int              *minimum,
                          int              *natural,
                          int              *minimum_baseline,
                          int              *natural_baseline)
{
  BisLatchLayout *self = BIS_LATCH_LAYOUT (manager);
  GtkWidget *child;

  for (child = gtk_widget_get_first_child (widget);
       child != NULL;
       child = gtk_widget_get_next_sibling (child)) {
    int child_min = 0;
    int child_nat = 0;
    int child_min_baseline = -1;
    int child_nat_baseline = -1;

    if (!gtk_widget_should_layout (child))
      continue;

    if (self->orientation == orientation) {
      gtk_widget_measure (child, orientation, for_size,
                          &child_min, &child_nat,
                          &child_min_baseline, &child_nat_baseline);

      child_nat = latch_size_from_child (self, child_min, child_nat);
    } else {
      int child_size = child_size_from_latch (self, child, for_size, NULL, NULL);

      gtk_widget_measure (child, orientation, child_size,
                          &child_min, &child_nat,
                          &child_min_baseline, &child_nat_baseline);
    }

    *minimum = MAX (*minimum, child_min);
    *natural = MAX (*natural, child_nat);

    if (child_min_baseline > -1)
      *minimum_baseline = MAX (*minimum_baseline, child_min_baseline);
    if (child_nat_baseline > -1)
      *natural_baseline = MAX (*natural_baseline, child_nat_baseline);
  }
}

static void
bis_latch_layout_allocate (GtkLayoutManager *manager,
                           GtkWidget        *widget,
                           int               width,
                           int               height,
                           int               baseline)
{
  BisLatchLayout *self = BIS_LATCH_LAYOUT (manager);
  GtkWidget *child;

  for (child = gtk_widget_get_first_child (widget);
       child != NULL;
       child = gtk_widget_get_next_sibling (child)) {
    GtkAllocation child_allocation;
    int child_maximum = 0, lower_threshold = 0;
    int child_latched_size;

    if (!gtk_widget_should_layout (child)) {
      gtk_widget_remove_css_class (child, "small");
      gtk_widget_remove_css_class (child, "medium");
      gtk_widget_remove_css_class (child, "large");

      return;
    }

    if (self->orientation == GTK_ORIENTATION_HORIZONTAL) {
      child_allocation.width = child_size_from_latch (self, child, width,
                                                      &child_maximum,
                                                      &lower_threshold);
      child_allocation.height = height;

      child_latched_size = child_allocation.width;
    } else {
      child_allocation.width = width;
      child_allocation.height = child_size_from_latch (self, child, height,
                                                       &child_maximum,
                                                       &lower_threshold);

      child_latched_size = child_allocation.height;
    }

    if (child_latched_size >= child_maximum) {
      gtk_widget_remove_css_class (child, "small");
      gtk_widget_remove_css_class (child, "medium");
      gtk_widget_add_css_class (child, "large");
    } else if (child_latched_size <= lower_threshold) {
      gtk_widget_add_css_class (child, "small");
      gtk_widget_remove_css_class (child, "medium");
      gtk_widget_remove_css_class (child, "large");
    } else {
      gtk_widget_remove_css_class (child, "small");
      gtk_widget_add_css_class (child, "medium");
      gtk_widget_remove_css_class (child, "large");
    }

    /* Always center the child on the side of the orientation. */
    if (self->orientation == GTK_ORIENTATION_HORIZONTAL) {
      child_allocation.x = (width - child_allocation.width) / 2;
      child_allocation.y = 0;
    } else {
      child_allocation.x = 0;
      child_allocation.y = (height - child_allocation.height) / 2;
    }

    gtk_widget_size_allocate (child, &child_allocation, baseline);
  }
}

static void
bis_latch_layout_class_init (BisLatchLayoutClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkLayoutManagerClass *layout_manager_class = GTK_LAYOUT_MANAGER_CLASS (klass);

  object_class->get_property = bis_latch_layout_get_property;
  object_class->set_property = bis_latch_layout_set_property;

  layout_manager_class->get_request_mode = bis_latch_layout_get_request_mode;
  layout_manager_class->measure = bis_latch_layout_measure;
  layout_manager_class->allocate = bis_latch_layout_allocate;

  g_object_class_override_property (object_class,
                                    PROP_ORIENTATION,
                                    "orientation");

  /**
   * BisLatchLayout:maximum-size: (attributes org.gtk.Property.get=bis_latch_layout_get_maximum_size org.gtk.Property.set=bis_latch_layout_set_maximum_size)
   *
   * The maximum size to allocate to the children.
   *
   * It is the width if the layout is horizontal, or the height if it is
   * vertical.
   *
   * Since: 1.0
   */
  props[PROP_MAXIMUM_SIZE] =
    g_param_spec_int ("maximum-size", NULL, NULL,
                      0, G_MAXINT, 600,
                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BisLatchLayout:tightening-threshold: (attributes org.gtk.Property.get=bis_latch_layout_get_tightening_threshold org.gtk.Property.set=bis_latch_layout_set_tightening_threshold)
   *
   * The size above which the children are latched.
   *
   * Starting from this size, the layout will tighten its grip on the children,
   * slowly allocating less and less of the available size up to the maximum
   * allocated size. Below that threshold and below the maximum size, the
   * children will be allocated all the available size.
   *
   * If the threshold is greater than the maximum size to allocate to the
   * children, they will be allocated the whole size up to the maximum. If the
   * threshold is lower than the minimum size to allocate to the children, that
   * size will be used as the tightening threshold.
   *
   * Effectively, tightening the grip on a child before it reaches its maximum
   * size makes transitions to and from the maximum size smoother when resizing.
   *
   * Since: 1.0
   */
  props[PROP_TIGHTENING_THRESHOLD] =
    g_param_spec_int ("tightening-threshold", NULL, NULL,
                      0, G_MAXINT, 400,
                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, LAST_PROP, props);
}

static void
bis_latch_layout_init (BisLatchLayout *self)
{
  self->maximum_size = 600;
  self->tightening_threshold = 400;
}

/**
 * bis_latch_layout_new:
 *
 * Creates a new `BisLatchLayout`.
 *
 * Returns: the newly created `BisLatchLayout`
 *
 * Since: 1.0
 */
GtkLayoutManager *
bis_latch_layout_new (void)
{
  return g_object_new (BIS_TYPE_LATCH_LAYOUT, NULL);
}

/**
 * bis_latch_layout_get_maximum_size: (attributes org.gtk.Method.get_property=maximum-size)
 * @self: a latch layout
 *
 * Gets the maximum size allocated to the children.
 *
 * Returns: the maximum size to allocate to the children
 *
 * Since: 1.0
 */
int
bis_latch_layout_get_maximum_size (BisLatchLayout *self)
{
  g_return_val_if_fail (BIS_IS_LATCH_LAYOUT (self), 0);

  return self->maximum_size;
}

/**
 * bis_latch_layout_set_maximum_size: (attributes org.gtk.Method.set_property=maximum-size)
 * @self: a latch layout
 * @maximum_size: the maximum size
 *
 * Sets the maximum size allocated to the children.
 *
 * It is the width if the layout is horizontal, or the height if it is vertical.
 *
 * Since: 1.0
 */
void
bis_latch_layout_set_maximum_size (BisLatchLayout *self,
                                   int             maximum_size)
{
  g_return_if_fail (BIS_IS_LATCH_LAYOUT (self));

  if (self->maximum_size == maximum_size)
    return;

  self->maximum_size = maximum_size;

  gtk_layout_manager_layout_changed (GTK_LAYOUT_MANAGER (self));

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_MAXIMUM_SIZE]);
}

/**
 * bis_latch_layout_get_tightening_threshold: (attributes org.gtk.Method.get_property=tightening-threshold)
 * @self: a latch layout
 *
 * Gets the size above which the children are latched.
 *
 * Returns: the size above which the children are latched
 *
 * Since: 1.0
 */
int
bis_latch_layout_get_tightening_threshold (BisLatchLayout *self)
{
  g_return_val_if_fail (BIS_IS_LATCH_LAYOUT (self), 0);

  return self->tightening_threshold;
}

/**
 * bis_latch_layout_set_tightening_threshold: (attributes org.gtk.Method.set_property=tightening-threshold)
 * @self: a latch layout
 * @tightening_threshold: the tightening threshold
 *
 * Sets the size above which the children are latched.
 *
 * Starting from this size, the layout will tighten its grip on the children,
 * slowly allocating less and less of the available size up to the maximum
 * allocated size. Below that threshold and below the maximum size, the children
 * will be allocated all the available size.
 *
 * If the threshold is greater than the maximum size to allocate to the
 * children, they will be allocated the whole size up to the maximum. If the
 * threshold is lower than the minimum size to allocate to the children, that
 * size will be used as the tightening threshold.
 *
 * Effectively, tightening the grip on a child before it reaches its maximum
 * size makes transitions to and from the maximum size smoother when resizing.
 *
 * Since: 1.0
 */
void
bis_latch_layout_set_tightening_threshold (BisLatchLayout *self,
                                           int             tightening_threshold)
{
  g_return_if_fail (BIS_IS_LATCH_LAYOUT (self));

  if (self->tightening_threshold == tightening_threshold)
    return;

  self->tightening_threshold = tightening_threshold;

  gtk_layout_manager_layout_changed (GTK_LAYOUT_MANAGER (self));

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_TIGHTENING_THRESHOLD]);
}
