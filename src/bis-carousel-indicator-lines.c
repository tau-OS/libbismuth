/*
 * Copyright (C) 2020 Alexander Mikhaylenko <alexm@gnome.org>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "config.h"

#include "bis-carousel-indicator-lines.h"

#include "bis-macros-private.h"
#include "bis-swipeable.h"
#include "bis-timed-animation.h"
#include "bis-widget-utils-private.h"

#include <math.h>

#define LINE_WIDTH 3
#define LINE_LENGTH 35
#define LINE_SPACING 5
#define LINE_OPACITY 0.3
#define LINE_OPACITY_ACTIVE 0.9
#define LINE_MARGIN 2

/**
 * BisCarouselIndicatorLines:
 *
 * A lines indicator for [class@Carousel].
 *
 * <picture>
 *   <source srcset="carousel-indicator-dots-lines.png" media="(prefers-color-scheme: dark)">
 *   <img src="carousel-indicator-lines.png" alt="carousel-indicator-lines">
 * </picture>
 *
 * The `BisCarouselIndicatorLines` widget shows a set of lines for each page of
 * a given [class@Carousel]. The carousel's active page is shown as another line
 * that moves between them to match the carousel's position.
 *
 * See also [class@CarouselIndicatorDots].
 *
 * ## CSS nodes
 *
 * `BisCarouselIndicatorLines` has a single CSS node with name
 * `carouselindicatorlines`.
 *
 * Since: 1.0
 */

struct _BisCarouselIndicatorLines
{
  GtkWidget parent_instance;

  BisCarousel *carousel;
  GtkOrientation orientation;

  BisAnimation *animation;
  GBinding *duration_binding;
};

G_DEFINE_FINAL_TYPE_WITH_CODE (BisCarouselIndicatorLines, bis_carousel_indicator_lines, GTK_TYPE_WIDGET,
                               G_IMPLEMENT_INTERFACE (GTK_TYPE_ORIENTABLE, NULL))

enum {
  PROP_0,
  PROP_CAROUSEL,

  /* GtkOrientable */
  PROP_ORIENTATION,
  LAST_PROP = PROP_CAROUSEL + 1,
};

static GParamSpec *props[LAST_PROP];

static void
snapshot_lines (GtkWidget      *widget,
                GtkSnapshot    *snapshot,
                GtkOrientation  orientation,
                double          position,
                double         *sizes,
                guint           n_pages)
{
  GdkRGBA color;
  int i, widget_length, widget_thickness;
  double indicator_length, full_size, line_size;
  double x = 0, y = 0, pos;

  gtk_widget_get_color (widget, &color);
  color.alpha *= LINE_OPACITY;

  line_size = LINE_LENGTH + LINE_SPACING;
  indicator_length = -LINE_SPACING;
  for (i = 0; i < n_pages; i++)
    indicator_length += line_size * sizes[i];

  if (orientation == GTK_ORIENTATION_HORIZONTAL) {
    widget_length = gtk_widget_get_width (widget);
    widget_thickness = gtk_widget_get_height (widget);
  } else {
    widget_length = gtk_widget_get_height (widget);
    widget_thickness = gtk_widget_get_width (widget);
  }

  /* Ensure the indicators are aligned to pixel grid when not animating */
  full_size = round (indicator_length / line_size) * line_size;
  if ((widget_length - (int) full_size) % 2 == 0)
    widget_length--;

  if (orientation == GTK_ORIENTATION_HORIZONTAL) {
    x = (widget_length - indicator_length) / 2.0;
    y = (widget_thickness - LINE_WIDTH) / 2;
  } else {
    x = (widget_thickness - LINE_WIDTH) / 2;
    y = (widget_length - indicator_length) / 2.0;
  }

  pos = 0;
  for (i = 0; i < n_pages; i++) {
    double length;
    graphene_rect_t rectangle;

    length = (LINE_LENGTH + LINE_SPACING) * sizes[i] - LINE_SPACING;

    if (length > 0) {
      if (orientation == GTK_ORIENTATION_HORIZONTAL)
        graphene_rect_init (&rectangle, x + pos, y, length, LINE_WIDTH);
      else
        graphene_rect_init (&rectangle, x, y + pos, LINE_WIDTH, length);
    }

    gtk_snapshot_append_color (snapshot, &color, &rectangle);

    pos += (LINE_LENGTH + LINE_SPACING) * sizes[i];
  }

  bis_widget_get_style_color (widget, &color);
  color.alpha *= LINE_OPACITY_ACTIVE;

  pos = position * (LINE_LENGTH + LINE_SPACING);

  if (orientation == GTK_ORIENTATION_HORIZONTAL)
    gtk_snapshot_append_color (snapshot, &color,
                               &GRAPHENE_RECT_INIT (x + pos, y, LINE_LENGTH, LINE_WIDTH));
  else
    gtk_snapshot_append_color (snapshot, &color,
                               &GRAPHENE_RECT_INIT (x, y + pos, LINE_WIDTH, LINE_LENGTH));
}

static void
animation_cb (double     value,
              GtkWidget *self)
{
  gtk_widget_queue_resize (self);
}

static void
bis_carousel_indicator_lines_measure (GtkWidget      *widget,
                                      GtkOrientation  orientation,
                                      int             for_size,
                                      int            *minimum,
                                      int            *natural,
                                      int            *minimum_baseline,
                                      int            *natural_baseline)
{
  BisCarouselIndicatorLines *self = BIS_CAROUSEL_INDICATOR_LINES (widget);
  int size = 0;

  if (orientation == self->orientation) {
    int i, n_points = 0;
    double indicator_length, line_size;
    double *points = NULL, *sizes;

    if (self->carousel)
      points = bis_swipeable_get_snap_points (BIS_SWIPEABLE (self->carousel), &n_points);

    sizes = g_new0 (double, n_points);

    if (n_points > 0)
      sizes[0] = points[0] + 1;
    for (i = 1; i < n_points; i++)
      sizes[i] = points[i] - points[i - 1];

    line_size = LINE_LENGTH + LINE_SPACING;
    indicator_length = 0;
    for (i = 0; i < n_points; i++)
      indicator_length += line_size * sizes[i];

    size = ceil (indicator_length);

    g_free (points);
    g_free (sizes);
  } else {
    size = LINE_WIDTH;
  }

  size += 2 * LINE_MARGIN;

  if (minimum)
    *minimum = size;

  if (natural)
    *natural = size;

  if (minimum_baseline)
    *minimum_baseline = -1;

  if (natural_baseline)
    *natural_baseline = -1;
}

static void
bis_carousel_indicator_lines_snapshot (GtkWidget   *widget,
                                       GtkSnapshot *snapshot)
{
  BisCarouselIndicatorLines *self = BIS_CAROUSEL_INDICATOR_LINES (widget);
  int i, n_points;
  double position;
  double *points, *sizes;

  if (!self->carousel)
    return;

  points = bis_swipeable_get_snap_points (BIS_SWIPEABLE (self->carousel), &n_points);
  position = bis_carousel_get_position (self->carousel);

  if (n_points < 2) {
    g_free (points);

    return;
  }

  if (self->orientation == GTK_ORIENTATION_HORIZONTAL &&
      gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL)
    position = points[n_points - 1] - position;

  sizes = g_new0 (double, n_points);

  sizes[0] = points[0] + 1;
  for (i = 1; i < n_points; i++)
    sizes[i] = points[i] - points[i - 1];

  snapshot_lines (widget, snapshot, self->orientation, position, sizes, n_points);

  g_free (sizes);
  g_free (points);
}

static void
bis_carousel_dispose (GObject *object)
{
  BisCarouselIndicatorLines *self = BIS_CAROUSEL_INDICATOR_LINES (object);

  bis_carousel_indicator_lines_set_carousel (self, NULL);

  g_clear_object (&self->animation);

  G_OBJECT_CLASS (bis_carousel_indicator_lines_parent_class)->dispose (object);
}

static void
bis_carousel_indicator_lines_get_property (GObject    *object,
                                           guint       prop_id,
                                           GValue     *value,
                                           GParamSpec *pspec)
{
  BisCarouselIndicatorLines *self = BIS_CAROUSEL_INDICATOR_LINES (object);

  switch (prop_id) {
  case PROP_CAROUSEL:
    g_value_set_object (value, bis_carousel_indicator_lines_get_carousel (self));
    break;

  case PROP_ORIENTATION:
    g_value_set_enum (value, self->orientation);
    break;

  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
bis_carousel_indicator_lines_set_property (GObject      *object,
                                           guint         prop_id,
                                           const GValue *value,
                                           GParamSpec   *pspec)
{
  BisCarouselIndicatorLines *self = BIS_CAROUSEL_INDICATOR_LINES (object);

  switch (prop_id) {
  case PROP_CAROUSEL:
    bis_carousel_indicator_lines_set_carousel (self, g_value_get_object (value));
    break;

  case PROP_ORIENTATION:
    {
      GtkOrientation orientation = g_value_get_enum (value);
      if (orientation != self->orientation) {
        self->orientation = orientation;
        gtk_widget_queue_resize (GTK_WIDGET (self));
        g_object_notify (G_OBJECT (self), "orientation");
      }
    }
    break;

  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
bis_carousel_indicator_lines_class_init (BisCarouselIndicatorLinesClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = bis_carousel_dispose;
  object_class->get_property = bis_carousel_indicator_lines_get_property;
  object_class->set_property = bis_carousel_indicator_lines_set_property;

  widget_class->measure = bis_carousel_indicator_lines_measure;
  widget_class->snapshot = bis_carousel_indicator_lines_snapshot;

  /**
   * BisCarouselIndicatorLines:carousel: (attributes org.gtk.Property.get=bis_carousel_indicator_lines_get_carousel org.gtk.Property.set=bis_carousel_indicator_lines_set_carousel)
   *
   * The displayed carousel.
   *
   * Since: 1.0
   */
  props[PROP_CAROUSEL] =
    g_param_spec_object ("carousel", NULL, NULL,
                         BIS_TYPE_CAROUSEL,
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_override_property (object_class,
                                    PROP_ORIENTATION,
                                    "orientation");

  g_object_class_install_properties (object_class, LAST_PROP, props);

  gtk_widget_class_set_css_name (widget_class, "carouselindicatorlines");
}

static void
bis_carousel_indicator_lines_init (BisCarouselIndicatorLines *self)
{
  BisAnimationTarget *target
    = bis_callback_animation_target_new ((BisAnimationTargetFunc) animation_cb,
                                         self, NULL);

  self->animation =
    bis_timed_animation_new (GTK_WIDGET (self), 0, 1, 0, target);
}

/**
 * bis_carousel_indicator_lines_new:
 *
 * Creates a new `BisCarouselIndicatorLines`.
 *
 * Returns: the newly created `BisCarouselIndicatorLines`
 *
 * Since: 1.0
 */
GtkWidget *
bis_carousel_indicator_lines_new (void)
{
  return g_object_new (BIS_TYPE_CAROUSEL_INDICATOR_LINES, NULL);
}

/**
 * bis_carousel_indicator_lines_get_carousel: (attributes org.gtk.Method.get_property=carousel)
 * @self: an indicator
 *
 * Gets the displayed carousel.
 *
 * Returns: (nullable) (transfer none): the displayed carousel
 *
 * Since: 1.0
 */
BisCarousel *
bis_carousel_indicator_lines_get_carousel (BisCarouselIndicatorLines *self)
{
  g_return_val_if_fail (BIS_IS_CAROUSEL_INDICATOR_LINES (self), NULL);

  return self->carousel;
}

/**
 * bis_carousel_indicator_lines_set_carousel: (attributes org.gtk.Method.set_property=carousel)
 * @self: an indicator
 * @carousel: (nullable): a carousel
 *
 * Sets the displayed carousel.
 *
 * Since: 1.0
 */
void
bis_carousel_indicator_lines_set_carousel (BisCarouselIndicatorLines *self,
                                           BisCarousel               *carousel)
{
  g_return_if_fail (BIS_IS_CAROUSEL_INDICATOR_LINES (self));
  g_return_if_fail (carousel == NULL || BIS_IS_CAROUSEL (carousel));

  if (self->carousel == carousel)
    return;

  bis_animation_reset (self->animation);

  if (self->carousel) {
    g_signal_handlers_disconnect_by_func (self->carousel,
                                          gtk_widget_queue_draw, self);
    g_signal_handlers_disconnect_by_func (self->carousel,
                                          bis_animation_play, self->animation);
    g_clear_object (&self->duration_binding);
  }

  g_set_object (&self->carousel, carousel);

  if (self->carousel) {
    g_signal_connect_object (self->carousel, "notify::position",
                             G_CALLBACK (gtk_widget_queue_draw), self,
                             G_CONNECT_SWAPPED);
    g_signal_connect_object (self->carousel, "notify::n-pages",
                             G_CALLBACK (bis_animation_play), self->animation,
                             G_CONNECT_SWAPPED);
    self->duration_binding =
      g_object_bind_property (self->carousel, "reveal-duration",
                              self->animation, "duration",
                              G_BINDING_SYNC_CREATE);
  }

  gtk_widget_queue_resize (GTK_WIDGET (self));

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_CAROUSEL]);
}
