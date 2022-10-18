/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "config.h"
#include "bis-clamp.h"

#include "bis-clamp-layout.h"
#include "bis-macros-private.h"
#include "bis-widget-utils-private.h"

/**
 * BisClamp:
 *
 * A widget constraining its child to a given size.
 *
 * <picture>
 *   <source srcset="clamp-wide-dark.png" media="(prefers-color-scheme: dark)">
 *   <img src="clamp-wide.png" alt="clamp-wide">
 * </picture>
 * <picture>
 *   <source srcset="clamp-narrow-dark.png" media="(prefers-color-scheme: dark)">
 *   <img src="clamp-narrow.png" alt="clamp-narrow">
 * </picture>
 *
 * The `BisClamp` widget constrains the size of the widget it contains to a
 * given maximum size. It will constrain the width if it is horizontal, or the
 * height if it is vertical. The expansion of the child from its minimum to its
 * maximum size is eased out for a smooth transition.
 *
 * If the child requires more than the requested maximum size, it will be
 * allocated the minimum size it can fit in instead.
 *
 * ## CSS nodes
 *
 * `BisClamp` has a single CSS node with name `clamp`.
 *
 * Its children will receive the style classes `.large` when the child reached
 * its maximum size, `.small` when the clamp allocates its full size to the
 * child, `.medium` in-between, or none if it hasn't computed its size yet.
 *
 * Since: 1.0
 */

enum {
  PROP_0,
  PROP_CHILD,
  PROP_MAXIMUM_SIZE,
  PROP_TIGHTENING_THRESHOLD,

  /* Overridden properties */
  PROP_ORIENTATION,

  LAST_PROP = PROP_TIGHTENING_THRESHOLD + 1,
};

struct _BisClamp
{
  GtkWidget parent_instance;

  GtkWidget *child;
  GtkOrientation orientation;
};

static GParamSpec *props[LAST_PROP];

static void bis_clamp_buildable_init (GtkBuildableIface *iface);

G_DEFINE_FINAL_TYPE_WITH_CODE (BisClamp, bis_clamp, GTK_TYPE_WIDGET,
                               G_IMPLEMENT_INTERFACE (GTK_TYPE_ORIENTABLE, NULL)
                               G_IMPLEMENT_INTERFACE (GTK_TYPE_BUILDABLE, bis_clamp_buildable_init))

static GtkBuildableIface *parent_buildable_iface;

static void
set_orientation (BisClamp       *self,
                 GtkOrientation  orientation)
{
  GtkLayoutManager *layout = gtk_widget_get_layout_manager (GTK_WIDGET (self));

  if (self->orientation == orientation)
    return;

  self->orientation = orientation;
  gtk_orientable_set_orientation (GTK_ORIENTABLE (layout), orientation);

  gtk_widget_queue_resize (GTK_WIDGET (self));

  g_object_notify (G_OBJECT (self), "orientation");
}

static void
bis_clamp_get_property (GObject    *object,
                        guint       prop_id,
                        GValue     *value,
                        GParamSpec *pspec)
{
  BisClamp *self = BIS_CLAMP (object);

  switch (prop_id) {
  case PROP_CHILD:
    g_value_set_object (value, bis_clamp_get_child (self));
    break;
  case PROP_MAXIMUM_SIZE:
    g_value_set_int (value, bis_clamp_get_maximum_size (self));
    break;
  case PROP_TIGHTENING_THRESHOLD:
    g_value_set_int (value, bis_clamp_get_tightening_threshold (self));
    break;
  case PROP_ORIENTATION:
    g_value_set_enum (value, self->orientation);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
bis_clamp_set_property (GObject      *object,
                        guint         prop_id,
                        const GValue *value,
                        GParamSpec   *pspec)
{
  BisClamp *self = BIS_CLAMP (object);

  switch (prop_id) {
  case PROP_CHILD:
    bis_clamp_set_child (self, g_value_get_object (value));
    break;
  case PROP_MAXIMUM_SIZE:
    bis_clamp_set_maximum_size (self, g_value_get_int (value));
    break;
  case PROP_TIGHTENING_THRESHOLD:
    bis_clamp_set_tightening_threshold (self, g_value_get_int (value));
    break;
  case PROP_ORIENTATION:
    set_orientation (self, g_value_get_enum (value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
bis_clamp_dispose (GObject *object)
{
  BisClamp *self = BIS_CLAMP (object);

  g_clear_pointer (&self->child, gtk_widget_unparent);

  G_OBJECT_CLASS (bis_clamp_parent_class)->dispose (object);
}

static void
bis_clamp_class_init (BisClampClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->get_property = bis_clamp_get_property;
  object_class->set_property = bis_clamp_set_property;
  object_class->dispose = bis_clamp_dispose;

  widget_class->compute_expand = bis_widget_compute_expand;

  g_object_class_override_property (object_class,
                                    PROP_ORIENTATION,
                                    "orientation");

  /**
   * BisClamp:child: (attributes org.gtk.Property.get=bis_clamp_get_child org.gtk.Property.set=bis_clamp_set_child)
   *
   * The child widget of the `BisClamp`.
   *
   * Since: 1.0
   */
  props[PROP_CHILD] =
    g_param_spec_object ("child", NULL, NULL,
                         GTK_TYPE_WIDGET,
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BisClamp:maximum-size: (attributes org.gtk.Property.get=bis_clamp_get_maximum_size org.gtk.Property.set=bis_clamp_set_maximum_size)
   *
   * The maximum size allocated to the child.
   *
   * It is the width if the clamp is horizontal, or the height if it is vertical.
   *
   * Since: 1.0
   */
  props[PROP_MAXIMUM_SIZE] =
    g_param_spec_int ("maximum-size", NULL, NULL,
                      0, G_MAXINT, 600,
                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BisClamp:tightening-threshold: (attributes org.gtk.Property.get=bis_clamp_get_tightening_threshold org.gtk.Property.set=bis_clamp_set_tightening_threshold)
   *
   * The size above which the child is clamped.
   *
   * Starting from this size, the clamp will tighten its grip on the child,
   * slowly allocating less and less of the available size up to the maximum
   * allocated size. Below that threshold and below the maximum size, the child
   * will be allocated all the available size.
   *
   * If the threshold is greater than the maximum size to allocate to the child,
   * the child will be allocated all the size up to the maximum.
   * If the threshold is lower than the minimum size to allocate to the child,
   * that size will be used as the tightening threshold.
   *
   * Effectively, tightening the grip on the child before it reaches its maximum
   * size makes transitions to and from the maximum size smoother when resizing.
   *
   * Since: 1.0
   */
  props[PROP_TIGHTENING_THRESHOLD] =
    g_param_spec_int ("tightening-threshold", NULL, NULL,
                      0, G_MAXINT, 400,
                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, LAST_PROP, props);

  gtk_widget_class_set_layout_manager_type (widget_class, BIS_TYPE_CLAMP_LAYOUT);
  gtk_widget_class_set_css_name (widget_class, "clamp");
  gtk_widget_class_set_accessible_role (widget_class, GTK_ACCESSIBLE_ROLE_GROUP);
}

static void
bis_clamp_init (BisClamp *self)
{
}

static void
bis_clamp_buildable_add_child (GtkBuildable *buildable,
                               GtkBuilder   *builder,
                               GObject      *child,
                               const char   *type)
{
  if (GTK_IS_WIDGET (child))
    bis_clamp_set_child (BIS_CLAMP (buildable), GTK_WIDGET (child));
  else
    parent_buildable_iface->add_child (buildable, builder, child, type);
}

static void
bis_clamp_buildable_init (GtkBuildableIface *iface)
{
  parent_buildable_iface = g_type_interface_peek_parent (iface);

  iface->add_child = bis_clamp_buildable_add_child;
}

/**
 * bis_clamp_new:
 *
 * Creates a new `BisClamp`.
 *
 * Returns: the newly created `BisClamp`
 *
 * Since: 1.0
 */
GtkWidget *
bis_clamp_new (void)
{
  return g_object_new (BIS_TYPE_CLAMP, NULL);
}

/**
 * bis_clamp_get_child: (attributes org.gtk.Method.get_property=child)
 * @self: a clamp
 *
 * Gets the child widget of @self.
 *
 * Returns: (nullable) (transfer none): the child widget of @self
 *
 * Since: 1.0
 */
GtkWidget *
bis_clamp_get_child (BisClamp  *self)
{
  g_return_val_if_fail (BIS_IS_CLAMP (self), NULL);

  return self->child;
}

/**
 * bis_clamp_set_child: (attributes org.gtk.Method.set_property=child)
 * @self: a clamp
 * @child: (nullable): the child widget
 *
 * Sets the child widget of @self.
 *
 * Since: 1.0
 */
void
bis_clamp_set_child (BisClamp  *self,
                     GtkWidget *child)
{
  g_return_if_fail (BIS_IS_CLAMP (self));
  g_return_if_fail (child == NULL || GTK_IS_WIDGET (child));

  if (self->child == child)
    return;

  g_clear_pointer (&self->child, gtk_widget_unparent);

  self->child = child;

  if (child)
    gtk_widget_set_parent (child, GTK_WIDGET (self));

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_CHILD]);
}

/**
 * bis_clamp_get_maximum_size: (attributes org.gtk.Method.get_property=maximum-size)
 * @self: a clamp
 *
 * Gets the maximum size allocated to the child.
 *
 * Returns: the maximum size to allocate to the child
 *
 * Since: 1.0
 */
int
bis_clamp_get_maximum_size (BisClamp *self)
{
  BisClampLayout *layout;

  g_return_val_if_fail (BIS_IS_CLAMP (self), 0);

  layout = BIS_CLAMP_LAYOUT (gtk_widget_get_layout_manager (GTK_WIDGET (self)));

  return bis_clamp_layout_get_maximum_size (layout);
}

/**
 * bis_clamp_set_maximum_size: (attributes org.gtk.Method.set_property=maximum-size)
 * @self: a clamp
 * @maximum_size: the maximum size
 *
 * Sets the maximum size allocated to the child.
 *
 * It is the width if the clamp is horizontal, or the height if it is vertical.
 *
 * Since: 1.0
 */
void
bis_clamp_set_maximum_size (BisClamp *self,
                            int       maximum_size)
{
  BisClampLayout *layout;

  g_return_if_fail (BIS_IS_CLAMP (self));

  layout = BIS_CLAMP_LAYOUT (gtk_widget_get_layout_manager (GTK_WIDGET (self)));

  if (bis_clamp_layout_get_maximum_size (layout) == maximum_size)
    return;

  bis_clamp_layout_set_maximum_size (layout, maximum_size);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_MAXIMUM_SIZE]);
}

/**
 * bis_clamp_get_tightening_threshold: (attributes org.gtk.Method.get_property=tightening-threshold)
 * @self: a clamp
 *
 * Gets the size above which the child is clamped.
 *
 * Returns: the size above which the child is clamped
 *
 * Since: 1.0
 */
int
bis_clamp_get_tightening_threshold (BisClamp *self)
{
  BisClampLayout *layout;

  g_return_val_if_fail (BIS_IS_CLAMP (self), 0);

  layout = BIS_CLAMP_LAYOUT (gtk_widget_get_layout_manager (GTK_WIDGET (self)));

  return bis_clamp_layout_get_tightening_threshold (layout);
}

/**
 * bis_clamp_set_tightening_threshold: (attributes org.gtk.Method.set_property=tightening-threshold)
 * @self: a clamp
 * @tightening_threshold: the tightening threshold
 *
 * Sets the size above which the child is clamped.
 *
 * Starting from this size, the clamp will tighten its grip on the child, slowly
 * allocating less and less of the available size up to the maximum allocated
 * size. Below that threshold and below the maximum size, the child will be
 * allocated all the available size.
 *
 * If the threshold is greater than the maximum size to allocate to the child,
 * the child will be allocated all the size up to the maximum. If the threshold
 * is lower than the minimum size to allocate to the child, that size will be
 * used as the tightening threshold.
 *
 * Effectively, tightening the grip on the child before it reaches its maximum
 * size makes transitions to and from the maximum size smoother when resizing.
 *
 * Since: 1.0
 */
void
bis_clamp_set_tightening_threshold (BisClamp *self,
                                    int       tightening_threshold)
{
  BisClampLayout *layout;

  g_return_if_fail (BIS_IS_CLAMP (self));

  layout = BIS_CLAMP_LAYOUT (gtk_widget_get_layout_manager (GTK_WIDGET (self)));

  if (bis_clamp_layout_get_tightening_threshold (layout) == tightening_threshold)
    return;

  bis_clamp_layout_set_tightening_threshold (layout, tightening_threshold);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_TIGHTENING_THRESHOLD]);
}
