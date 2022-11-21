/*
 * Copyright (C) 2019 Alexander Mikhaylenko <exalm7659@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "config.h"

#include "bis-carousel.h"

#include "bis-animation-util.h"
#include "bis-macros-private.h"
#include "bis-navigation-direction.h"
#include "bis-spring-animation.h"
#include "bis-swipe-tracker.h"
#include "bis-swipeable.h"
#include "bis-timed-animation.h"
#include "bis-widget-utils-private.h"

#include <math.h>

#define SCROLL_TIMEOUT_DURATION 150

/**
 * BisCarousel:
 *
 * A paginated scrolling widget.
 *
 * <picture>
 *   <source srcset="carousel-dark.png" media="(prefers-color-scheme: dark)">
 *   <img src="carousel.png" alt="carousel">
 * </picture>
 *
 * The `BisCarousel` widget can be used to display a set of pages with
 * swipe-based navigation between them.
 *
 * [class@CarouselIndicatorDots] and [class@CarouselIndicatorLines] can be used
 * to provide page indicators for `BisCarousel`.
 *
 * ## CSS nodes
 *
 * `BisCarousel` has a single CSS node with name `carousel`.
 *
 * Since: 1.0
 */

typedef struct {
  GtkWidget *widget;
  int position;
  gboolean visible;
  double size;
  double snap_point;
  gboolean adding;
  gboolean removing;

  gboolean shift_position;
  BisAnimation *resize_animation;
} ChildInfo;

struct _BisCarousel
{
  GtkWidget parent_instance;

  GList *children;
  double distance;
  double position;
  guint spacing;
  GtkOrientation orientation;
  guint reveal_duration;

  double animation_source_position;
  BisAnimation *animation;
  ChildInfo *animation_target_child;

  BisSwipeTracker *tracker;

  gboolean allow_scroll_wheel;

  double position_shift;

  guint scroll_timeout_id;
  gboolean can_scroll;
};

static void bis_carousel_buildable_init (GtkBuildableIface *iface);
static void bis_carousel_swipeable_init (BisSwipeableInterface *iface);

G_DEFINE_FINAL_TYPE_WITH_CODE (BisCarousel, bis_carousel, GTK_TYPE_WIDGET,
                               G_IMPLEMENT_INTERFACE (GTK_TYPE_ORIENTABLE, NULL)
                               G_IMPLEMENT_INTERFACE (GTK_TYPE_BUILDABLE, bis_carousel_buildable_init)
                               G_IMPLEMENT_INTERFACE (BIS_TYPE_SWIPEABLE, bis_carousel_swipeable_init))

static GtkBuildableIface *parent_buildable_iface;

enum {
  PROP_0,
  PROP_N_PAGES,
  PROP_POSITION,
  PROP_INTERACTIVE,
  PROP_SPACING,
  PROP_SCROLL_PARAMS,
  PROP_ALLOW_MOUSE_DRAG,
  PROP_ALLOW_SCROLL_WHEEL,
  PROP_ALLOW_LONG_SWIPES,
  PROP_REVEAL_DURATION,

  /* GtkOrientable */
  PROP_ORIENTATION,
  LAST_PROP = PROP_REVEAL_DURATION + 1,
};

static GParamSpec *props[LAST_PROP];

enum {
  SIGNAL_PAGE_CHANGED,
  SIGNAL_LAST_SIGNAL,
};
static guint signals[SIGNAL_LAST_SIGNAL];

static ChildInfo *
find_child_info (BisCarousel *self,
                 GtkWidget   *widget)
{
  GList *l;

  for (l = self->children; l; l = l->next) {
    ChildInfo *info = l->data;

    if (widget == info->widget)
      return info;
  }

  return NULL;
}

static int
find_child_index (BisCarousel *self,
                  GtkWidget   *widget,
                  gboolean     count_removing)
{
  GList *l;
  int i;

  i = 0;
  for (l = self->children; l; l = l->next) {
    ChildInfo *info = l->data;

    if (info->removing && !count_removing)
      continue;

    if (widget == info->widget)
      return i;

    i++;
  }

  return -1;
}

static GList *
get_nth_link (BisCarousel *self,
              int          n)
{

  GList *l;
  int i;

  i = n;
  for (l = self->children; l; l = l->next) {
    ChildInfo *info = l->data;

    if (info->removing)
      continue;

    if (i-- == 0)
      return l;
  }

  return NULL;
}

static ChildInfo *
get_closest_child_at (BisCarousel *self,
                      double       position,
                      gboolean     count_adding,
                      gboolean     count_removing)
{
  GList *l;
  ChildInfo *closest_child = NULL;

  for (l = self->children; l; l = l->next) {
    ChildInfo *child = l->data;

    if (child->adding && !count_adding)
      continue;

    if (child->removing && !count_removing)
      continue;

    if (!closest_child ||
        ABS (closest_child->snap_point - position) >
        ABS (child->snap_point - position))
      closest_child = child;
  }

  return closest_child;
}

static inline void
get_range (BisCarousel *self,
           double      *lower,
           double      *upper)
{
  GList *l = g_list_last (self->children);
  ChildInfo *child = l ? l->data : NULL;

  if (lower)
    *lower = 0;

  if (upper)
    *upper = MAX (0, self->position_shift + (child ? child->snap_point : 0));
}

static GtkWidget *
get_page_at_position (BisCarousel *self,
                      double       position)
{
  double lower = 0, upper = 0;
  ChildInfo *child;

  get_range (self, &lower, &upper);

  position = CLAMP (position, lower, upper);

  child = get_closest_child_at (self, position, TRUE, FALSE);

  if (!child)
    return NULL;

  return child->widget;
}

static void
update_shift_position_flag (BisCarousel *self,
                            ChildInfo   *child)
{
  ChildInfo *closest_child;
  int animating_index, closest_index;

  /* We want to still shift position when the active child is being removed */
  closest_child = get_closest_child_at (self, self->position, FALSE, TRUE);

  if (!closest_child)
    return;

  animating_index = g_list_index (self->children, child);
  closest_index = g_list_index (self->children, closest_child);

  child->shift_position = (closest_index >= animating_index);
}

static void
set_position (BisCarousel *self,
              double       position)
{
  GList *l;
  double lower = 0, upper = 0;

  get_range (self, &lower, &upper);

  position = CLAMP (position, lower, upper);

  self->position = position;
  gtk_widget_queue_allocate (GTK_WIDGET (self));

  for (l = self->children; l; l = l->next) {
    ChildInfo *child = l->data;

    if (child->adding || child->removing)
      update_shift_position_flag (self, child);
  }

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_POSITION]);
}

static void
resize_animation_value_cb (double     value,
                           ChildInfo *child)
{
  BisCarousel *self = BIS_CAROUSEL (bis_animation_get_widget (child->resize_animation));
  double delta = value - child->size;

  child->size = value;

  if (child->shift_position)
    self->position_shift += delta;

  gtk_widget_queue_allocate (GTK_WIDGET (self));
}

static void
resize_animation_done_cb (ChildInfo *child)
{
  BisCarousel *self = BIS_CAROUSEL (bis_animation_get_widget (child->resize_animation));

  g_clear_object (&child->resize_animation);

  if (child->adding)
    child->adding = FALSE;

  if (child->removing) {
    self->children = g_list_remove (self->children, child);

    g_free (child);
  }

  gtk_widget_queue_allocate (GTK_WIDGET (self));
}

static void
animate_child_resize (BisCarousel *self,
                      ChildInfo   *child,
                      double       value,
                      guint        duration)
{
  BisAnimationTarget *target;
  double old_size = child->size;

  update_shift_position_flag (self, child);

  if (child->resize_animation) {
    gboolean been_removing = child->removing;
    bis_animation_skip (child->resize_animation);
    /* It's because the skip finishes the animation, which triggers
       the 'done' signal, which calls resize_animation_done_cb(),
       which frees the 'child' immediately. */
    if (been_removing)
      return;
  }

  target = bis_callback_animation_target_new ((BisAnimationTargetFunc)
                                              resize_animation_value_cb,
                                              child, NULL);
  child->resize_animation =
    bis_timed_animation_new (GTK_WIDGET (self), old_size,
                             value, duration, target);

  g_signal_connect_swapped (child->resize_animation, "done",
                            G_CALLBACK (resize_animation_done_cb), child);

  bis_animation_play (child->resize_animation);
}

static void
scroll_animation_value_cb (double       value,
                           BisCarousel *self)
{
  set_position (self, value);

  gtk_widget_queue_allocate (GTK_WIDGET (self));
}

static void
scroll_animation_done_cb (BisCarousel *self)
{
  GtkWidget *child;
  int index;

  self->animation_source_position = 0;
  self->animation_target_child = NULL;

  child = get_page_at_position (self, self->position);
  index = find_child_index (self, child, FALSE);

  g_signal_emit (self, signals[SIGNAL_PAGE_CHANGED], 0, index);
}

static void
scroll_to (BisCarousel *self,
           GtkWidget   *widget,
           double       velocity)
{
  self->animation_target_child = find_child_info (self, widget);

  if (self->animation_target_child == NULL)
    return;

  self->animation_source_position = self->position;

  bis_spring_animation_set_value_from (BIS_SPRING_ANIMATION (self->animation),
                                       self->animation_source_position);
  bis_spring_animation_set_value_to (BIS_SPRING_ANIMATION (self->animation),
                                     self->animation_target_child->snap_point);
  bis_spring_animation_set_initial_velocity (BIS_SPRING_ANIMATION (self->animation),
                                             velocity);
  bis_animation_play (self->animation);
}

static inline double
get_closest_snap_point (BisCarousel *self)
{
  ChildInfo *closest_child =
    get_closest_child_at (self, self->position, TRUE, TRUE);

  if (!closest_child)
    return 0;

  return closest_child->snap_point;
}

static void
begin_swipe_cb (BisSwipeTracker *tracker,
                BisCarousel     *self)
{
  bis_animation_pause (self->animation);
}

static void
update_swipe_cb (BisSwipeTracker *tracker,
                 double           progress,
                 BisCarousel     *self)
{
  set_position (self, progress);
}

static void
end_swipe_cb (BisSwipeTracker *tracker,
              double           velocity,
              double           to,
              BisCarousel     *self)
{
  GtkWidget *child = get_page_at_position (self, to);

  scroll_to (self, child, velocity);
}

/* Copied from GtkOrientable. Orientable widgets are supposed
 * to do this manually via a private GTK function. */
static void
set_orientable_style_classes (GtkOrientable *orientable)
{
  GtkOrientation orientation = gtk_orientable_get_orientation (orientable);
  GtkWidget *widget = GTK_WIDGET (orientable);

  if (orientation == GTK_ORIENTATION_HORIZONTAL) {
    gtk_widget_add_css_class (widget, "horizontal");
    gtk_widget_remove_css_class (widget, "vertical");
  } else {
    gtk_widget_add_css_class (widget, "vertical");
    gtk_widget_remove_css_class (widget, "horizontal");
  }
}

static void
update_orientation (BisCarousel *self)
{
  gboolean reversed =
    self->orientation == GTK_ORIENTATION_HORIZONTAL &&
    gtk_widget_get_direction (GTK_WIDGET (self)) == GTK_TEXT_DIR_RTL;

  gtk_orientable_set_orientation (GTK_ORIENTABLE (self->tracker),
                                  self->orientation);
  bis_swipe_tracker_set_reversed (self->tracker,
                                  reversed);

  set_orientable_style_classes (GTK_ORIENTABLE (self));
}

static gboolean
scroll_timeout_cb (BisCarousel *self)
{
  self->can_scroll = TRUE;
  return G_SOURCE_REMOVE;
}

static gboolean
scroll_cb (BisCarousel              *self,
           double                    dx,
           double                    dy,
           GtkEventControllerScroll *controller)
{
  GdkDevice *source_device;
  GdkInputSource input_source;
  int index;
  gboolean allow_vertical;
  GtkOrientation orientation;
  GtkWidget *child;

  if (!self->allow_scroll_wheel)
    return GDK_EVENT_PROPAGATE;

  if (!self->can_scroll)
    return GDK_EVENT_PROPAGATE;

  if (!bis_carousel_get_interactive (self))
    return GDK_EVENT_PROPAGATE;

  if (bis_carousel_get_n_pages (self) == 0)
    return GDK_EVENT_PROPAGATE;

  source_device = gtk_event_controller_get_current_event_device (GTK_EVENT_CONTROLLER (controller));
  input_source = gdk_device_get_source (source_device);
  if (input_source == GDK_SOURCE_TOUCHPAD)
    return GDK_EVENT_PROPAGATE;

  /* Mice often don't have easily accessible horizontal scrolling,
   * hence allow vertical mouse scrolling regardless of orientation */
  allow_vertical = (input_source == GDK_SOURCE_MOUSE);

  orientation = gtk_orientable_get_orientation (GTK_ORIENTABLE (self));
  index = 0;

  if (orientation == GTK_ORIENTATION_VERTICAL || allow_vertical) {
    if (dy > 0)
      index++;
    else if (dy < 0)
      index--;
  }

  if (orientation == GTK_ORIENTATION_HORIZONTAL && index == 0) {
    if (dx > 0)
      index++;
    else if (dx < 0)
      index--;
  }

  if (index == 0)
    return GDK_EVENT_PROPAGATE;

  child = get_page_at_position (self, self->position);

  index += find_child_index (self, child, FALSE);
  index = CLAMP (index, 0, (int) bis_carousel_get_n_pages (self) - 1);

  scroll_to (self, bis_carousel_get_nth_page (self, index), 0);

  self->can_scroll = FALSE;
  self->scroll_timeout_id =
   g_timeout_add (SCROLL_TIMEOUT_DURATION, (GSourceFunc) scroll_timeout_cb, self);

  return GDK_EVENT_STOP;
}

static void
bis_carousel_measure (GtkWidget      *widget,
                      GtkOrientation  orientation,
                      int             for_size,
                      int            *minimum,
                      int            *natural,
                      int            *minimum_baseline,
                      int            *natural_baseline)
{
  BisCarousel *self = BIS_CAROUSEL (widget);
  GList *children;

  if (minimum)
    *minimum = 0;
  if (natural)
    *natural = 0;

  if (minimum_baseline)
    *minimum_baseline = -1;
  if (natural_baseline)
    *natural_baseline = -1;

  for (children = self->children; children; children = children->next) {
    ChildInfo *child_info = children->data;
    GtkWidget *child = child_info->widget;
    int child_min, child_nat;

    if (child_info->removing)
      continue;

    if (!gtk_widget_get_visible (child))
      continue;

    gtk_widget_measure (child, orientation, for_size,
                        &child_min, &child_nat, NULL, NULL);

    if (minimum)
      *minimum = MAX (*minimum, child_min);
    if (natural)
      *natural = MAX (*natural, child_nat);
  }
}

static void
bis_carousel_size_allocate (GtkWidget *widget,
                            int        width,
                            int        height,
                            int        baseline)
{
  BisCarousel *self = BIS_CAROUSEL (widget);
  int size, child_width, child_height;
  GList *children;
  double x, y, offset;
  gboolean is_rtl;
  double snap_point;

  if (self->position_shift != 0) {
    set_position (self, self->position + self->position_shift);
    bis_swipe_tracker_shift_position (self->tracker, self->position_shift);
    self->position_shift = 0;
  }

  size = 0;
  for (children = self->children; children; children = children->next) {
    ChildInfo *child_info = children->data;
    GtkWidget *child = child_info->widget;
    int min, nat;
    int child_size;

    if (child_info->removing)
      continue;

    if (self->orientation == GTK_ORIENTATION_HORIZONTAL) {
      gtk_widget_measure (child, self->orientation,
                          height, &min, &nat, NULL, NULL);
      if (gtk_widget_get_hexpand (child))
        child_size = width;
      else
        child_size = CLAMP (nat, min, width);
    } else {
      gtk_widget_measure (child, self->orientation,
                          width, &min, &nat, NULL, NULL);
      if (gtk_widget_get_vexpand (child))
        child_size = height;
      else
        child_size = CLAMP (nat, min, height);
    }

    size = MAX (size, child_size);
  }

  self->distance = size + self->spacing;

  if (self->orientation == GTK_ORIENTATION_HORIZONTAL) {
    child_width = size;
    child_height = height;
  } else {
    child_width = width;
    child_height = size;
  }

  snap_point = 0;

  for (children = self->children; children; children = children->next) {
    ChildInfo *child_info = children->data;

    child_info->snap_point = snap_point + child_info->size - 1;

    snap_point += child_info->size;

    if (child_info == self->animation_target_child)
      bis_spring_animation_set_value_to (BIS_SPRING_ANIMATION (self->animation),
                                         child_info->snap_point);
  }

  if (!gtk_widget_get_realized (GTK_WIDGET (self)))
    return;

  x = 0;
  y = 0;

  is_rtl = (gtk_widget_get_direction (GTK_WIDGET (self)) == GTK_TEXT_DIR_RTL);

  if (self->orientation == GTK_ORIENTATION_VERTICAL)
    offset = (self->distance * self->position) - (height - child_height) / 2.0;
  else if (is_rtl)
    offset = -(self->distance * self->position) - (width - child_width) / 2.0;
  else
    offset = (self->distance * self->position) - (width - child_width) / 2.0;

  if (self->orientation == GTK_ORIENTATION_VERTICAL)
    y -= offset;
  else
    x -= offset;

  for (children = self->children; children; children = children->next) {
    ChildInfo *child_info = children->data;
    GskTransform *transform = gsk_transform_new ();

    if (!child_info->removing) {
      if (!gtk_widget_get_visible (child_info->widget))
        continue;

      if (self->orientation == GTK_ORIENTATION_VERTICAL) {
        child_info->position = y;
        child_info->visible = child_info->position < height &&
                              child_info->position + child_height > 0;

        transform = gsk_transform_translate (transform, &GRAPHENE_POINT_INIT (0, child_info->position));
      } else {
        child_info->position = x;
        child_info->visible = child_info->position < width &&
                              child_info->position + child_width > 0;

        transform = gsk_transform_translate (transform, &GRAPHENE_POINT_INIT (child_info->position, 0));
      }

      gtk_widget_allocate (child_info->widget, child_width, child_height, baseline, transform);
    }

    if (self->orientation == GTK_ORIENTATION_VERTICAL)
      y += self->distance * child_info->size;
    else if (is_rtl)
      x -= self->distance * child_info->size;
    else
      x += self->distance * child_info->size;
  }
}

static void
bis_carousel_direction_changed (GtkWidget        *widget,
                                GtkTextDirection  previous_direction)
{
  BisCarousel *self = BIS_CAROUSEL (widget);

  update_orientation (self);
}

static void
bis_carousel_constructed (GObject *object)
{
  BisCarousel *self = (BisCarousel *)object;

  update_orientation (self);

  G_OBJECT_CLASS (bis_carousel_parent_class)->constructed (object);
}

static void
bis_carousel_dispose (GObject *object)
{
  BisCarousel *self = BIS_CAROUSEL (object);

  while (self->children) {
    ChildInfo *info = self->children->data;

    bis_carousel_remove (self, info->widget);
  }

  g_clear_object (&self->tracker);
  g_clear_object (&self->animation);
  g_clear_handle_id (&self->scroll_timeout_id, g_source_remove);

  G_OBJECT_CLASS (bis_carousel_parent_class)->dispose (object);
}

static void
bis_carousel_finalize (GObject *object)
{
  BisCarousel *self = BIS_CAROUSEL (object);

  g_list_free_full (self->children, (GDestroyNotify) g_free);

  G_OBJECT_CLASS (bis_carousel_parent_class)->finalize (object);
}

static void
bis_carousel_get_property (GObject    *object,
                           guint       prop_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  BisCarousel *self = BIS_CAROUSEL (object);

  switch (prop_id) {
  case PROP_N_PAGES:
    g_value_set_uint (value, bis_carousel_get_n_pages (self));
    break;

  case PROP_POSITION:
    g_value_set_double (value, bis_carousel_get_position (self));
    break;

  case PROP_INTERACTIVE:
    g_value_set_boolean (value, bis_carousel_get_interactive (self));
    break;

  case PROP_SPACING:
    g_value_set_uint (value, bis_carousel_get_spacing (self));
    break;

  case PROP_ALLOW_MOUSE_DRAG:
    g_value_set_boolean (value, bis_carousel_get_allow_mouse_drag (self));
    break;

  case PROP_ALLOW_SCROLL_WHEEL:
    g_value_set_boolean (value, bis_carousel_get_allow_scroll_wheel (self));
    break;

  case PROP_ALLOW_LONG_SWIPES:
    g_value_set_boolean (value, bis_carousel_get_allow_long_swipes (self));
    break;

  case PROP_REVEAL_DURATION:
    g_value_set_uint (value, bis_carousel_get_reveal_duration (self));
    break;

  case PROP_ORIENTATION:
    g_value_set_enum (value, self->orientation);
    break;

  case PROP_SCROLL_PARAMS:
    g_value_set_boxed (value, bis_carousel_get_scroll_params (self));
    break;

  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
bis_carousel_set_property (GObject      *object,
                           guint         prop_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  BisCarousel *self = BIS_CAROUSEL (object);

  switch (prop_id) {
  case PROP_INTERACTIVE:
    bis_carousel_set_interactive (self, g_value_get_boolean (value));
    break;

  case PROP_SPACING:
    bis_carousel_set_spacing (self, g_value_get_uint (value));
    break;

  case PROP_SCROLL_PARAMS:
    bis_carousel_set_scroll_params (self, g_value_get_boxed (value));
    break;

  case PROP_REVEAL_DURATION:
    bis_carousel_set_reveal_duration (self, g_value_get_uint (value));
    break;

  case PROP_ALLOW_MOUSE_DRAG:
    bis_carousel_set_allow_mouse_drag (self, g_value_get_boolean (value));
    break;

  case PROP_ALLOW_SCROLL_WHEEL:
    bis_carousel_set_allow_scroll_wheel (self, g_value_get_boolean (value));
    break;

  case PROP_ALLOW_LONG_SWIPES:
    bis_carousel_set_allow_long_swipes (self, g_value_get_boolean (value));
    break;

  case PROP_ORIENTATION:
    {
      GtkOrientation orientation = g_value_get_enum (value);
      if (orientation != self->orientation) {
        self->orientation = orientation;
        update_orientation (self);
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
bis_carousel_class_init (BisCarouselClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->constructed = bis_carousel_constructed;
  object_class->dispose = bis_carousel_dispose;
  object_class->finalize = bis_carousel_finalize;
  object_class->get_property = bis_carousel_get_property;
  object_class->set_property = bis_carousel_set_property;

  widget_class->measure = bis_carousel_measure;
  widget_class->size_allocate = bis_carousel_size_allocate;
  widget_class->direction_changed = bis_carousel_direction_changed;
  widget_class->get_request_mode = bis_widget_get_request_mode;
  widget_class->compute_expand = bis_widget_compute_expand;

  /**
   * BisCarousel:n-pages: (attributes org.gtk.Property.get=bis_carousel_get_n_pages)
   *
   * The number of pages in a `BisCarousel`.
   *
   * Since: 1.0
   */
  props[PROP_N_PAGES] =
    g_param_spec_uint ("n-pages", NULL, NULL,
                       0,
                       G_MAXUINT,
                       0,
                       G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  /**
   * BisCarousel:position: (attributes org.gtk.Property.get=bis_carousel_get_position)
   *
   * Current scrolling position, unitless.
   *
   * 1 matches 1 page. Use [method@Carousel.scroll_to] for changing it.
   *
   * Since: 1.0
   */
  props[PROP_POSITION] =
    g_param_spec_double ("position", NULL, NULL,
                         0,
                         G_MAXDOUBLE,
                         0,
                         G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  /**
   * BisCarousel:interactive: (attributes org.gtk.Property.get=bis_carousel_get_interactive org.gtk.Property.set=bis_carousel_set_interactive)
   *
   * Whether the carousel can be navigated.
   *
   * This can be used to temporarily disable the carousel to only allow
   * navigating it in a certain state.
   *
   * Since: 1.0
   */
  props[PROP_INTERACTIVE] =
    g_param_spec_boolean ("interactive", NULL, NULL,
                          TRUE,
                          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BisCarousel:spacing: (attributes org.gtk.Property.get=bis_carousel_get_spacing org.gtk.Property.set=bis_carousel_set_spacing)
   *
   * Spacing between pages in pixels.
   *
   * Since: 1.0
   */
  props[PROP_SPACING] =
    g_param_spec_uint ("spacing", NULL, NULL,
                       0,
                       G_MAXUINT,
                       0,
                       G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BisCarousel:scroll-params: (attributes org.gtk.Property.get=bis_carousel_get_scroll_params org.gtk.Property.set=bis_carousel_set_scroll_params)
   *
   * Scroll animation spring parameters.
   *
   * The default value is equivalent to:
   *
   * ```c
   * bis_spring_params_new (1, 0.5, 500)
   * ```
   *
   * Since: 1.0
   */
  props[PROP_SCROLL_PARAMS] =
    g_param_spec_boxed ("scroll-params", NULL, NULL,
                        BIS_TYPE_SPRING_PARAMS,
                        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BisCarousel:allow-mouse-drag: (attributes org.gtk.Property.get=bis_carousel_get_allow_mouse_drag org.gtk.Property.set=bis_carousel_set_allow_mouse_drag)
   *
   * Sets whether the `BisCarousel` can be dragged with mouse pointer.
   *
   * If the value is `FALSE`, dragging is only available on touch.
   *
   * Since: 1.0
   */
  props[PROP_ALLOW_MOUSE_DRAG] =
    g_param_spec_boolean ("allow-mouse-drag", NULL, NULL,
                          TRUE,
                          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BisCarousel:allow-scroll-wheel: (attributes org.gtk.Property.get=bis_carousel_get_allow_scroll_wheel org.gtk.Property.set=bis_carousel_set_allow_scroll_wheel)
   *
   * Whether the widget will respond to scroll wheel events.
   *
   * If the value is `FALSE`, wheel events will be ignored.
   *
   * Since: 1.0
   */
  props[PROP_ALLOW_SCROLL_WHEEL] =
    g_param_spec_boolean ("allow-scroll-wheel", NULL, NULL,
                          TRUE,
                          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BisCarousel:allow-long-swipes: (attributes org.gtk.Property.get=bis_carousel_get_allow_long_swipes org.gtk.Property.set=bis_carousel_set_allow_long_swipes)
   *
   * Whether to allow swiping for more than one page at a time.
   *
   * If the value is `FALSE`, each swipe can only move to the adjacent pages.
   *
   * Since: 1.0
   */
  props[PROP_ALLOW_LONG_SWIPES] =
    g_param_spec_boolean ("allow-long-swipes", NULL, NULL,
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BisCarousel:reveal-duration:
   *
   * Page reveal duration, in milliseconds.
   *
   * Reveal duration is used when animating adding or removing pages.
   *
   * Since: 1.0
   */
  props[PROP_REVEAL_DURATION] =
    g_param_spec_uint ("reveal-duration", NULL, NULL,
                       0,
                       G_MAXUINT,
                       0,
                       G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_override_property (object_class,
                                    PROP_ORIENTATION,
                                    "orientation");

  g_object_class_install_properties (object_class, LAST_PROP, props);

  /**
   * BisCarousel::page-changed:
   * @self: a carousel
   * @index: current page
   *
   * This signal is emitted after a page has been changed.
   *
   * It can be used to implement "infinite scrolling" by amending the pages
   * after every scroll.
   *
   * Since: 1.0
   */
  signals[SIGNAL_PAGE_CHANGED] =
    g_signal_new ("page-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  0,
                  NULL, NULL, NULL,
                  G_TYPE_NONE,
                  1,
                  G_TYPE_UINT);

  gtk_widget_class_set_css_name (widget_class, "carousel");
}

static void
bis_carousel_init (BisCarousel *self)
{
  GtkEventController *controller;
  BisAnimationTarget *target;

  self->allow_scroll_wheel = TRUE;

  gtk_widget_set_overflow (GTK_WIDGET (self), GTK_OVERFLOW_HIDDEN);

  self->orientation = GTK_ORIENTATION_HORIZONTAL;
  self->reveal_duration = 0;
  self->can_scroll = TRUE;

  self->tracker = bis_swipe_tracker_new (BIS_SWIPEABLE (self));
  bis_swipe_tracker_set_allow_mouse_drag (self->tracker, TRUE);

  g_signal_connect_object (self->tracker, "begin-swipe", G_CALLBACK (begin_swipe_cb), self, 0);
  g_signal_connect_object (self->tracker, "update-swipe", G_CALLBACK (update_swipe_cb), self, 0);
  g_signal_connect_object (self->tracker, "end-swipe", G_CALLBACK (end_swipe_cb), self, 0);

  controller = gtk_event_controller_scroll_new (GTK_EVENT_CONTROLLER_SCROLL_BOTH_AXES);
  g_signal_connect_swapped (controller, "scroll", G_CALLBACK (scroll_cb), self);
  gtk_widget_add_controller (GTK_WIDGET (self), controller);

  target = bis_callback_animation_target_new ((BisAnimationTargetFunc)
                                              scroll_animation_value_cb,
                                              self, NULL);
  self->animation =
    bis_spring_animation_new (GTK_WIDGET (self), 0, 0,
                              bis_spring_params_new (1, 0.5, 500),
                              target);
  bis_spring_animation_set_latch (BIS_SPRING_ANIMATION (self->animation), TRUE);

  g_signal_connect_swapped (self->animation, "done",
                            G_CALLBACK (scroll_animation_done_cb), self);
}

static void
bis_carousel_buildable_add_child (GtkBuildable *buildable,
                                  GtkBuilder   *builder,
                                  GObject      *child,
                                  const char   *type)
{
  if (GTK_IS_WIDGET (child))
    bis_carousel_append (BIS_CAROUSEL (buildable), GTK_WIDGET (child));
  else
    parent_buildable_iface->add_child (buildable, builder, child, type);
}

static void
bis_carousel_buildable_init (GtkBuildableIface *iface)
{
  parent_buildable_iface = g_type_interface_peek_parent (iface);

  iface->add_child = bis_carousel_buildable_add_child;
}

static double
bis_carousel_get_distance (BisSwipeable *swipeable)
{
  BisCarousel *self = BIS_CAROUSEL (swipeable);

  return self->distance;
}

static double *
bis_carousel_get_snap_points (BisSwipeable *swipeable,
                              int          *n_snap_points)
{
  BisCarousel *self = BIS_CAROUSEL (swipeable);
  guint i, n_pages;
  double *points;
  GList *l;

  n_pages = MAX (g_list_length (self->children), 1);
  points = g_new0 (double, n_pages);

  i = 0;
  for (l = self->children; l; l = l->next) {
    ChildInfo *info = l->data;

    points[i++] = info->snap_point;
  }

  if (n_snap_points)
    *n_snap_points = n_pages;

  return points;
}

static double
bis_carousel_get_progress (BisSwipeable *swipeable)
{
  BisCarousel *self = BIS_CAROUSEL (swipeable);

  return bis_carousel_get_position (self);
}

static double
bis_carousel_get_cancel_progress (BisSwipeable *swipeable)
{
  BisCarousel *self = BIS_CAROUSEL (swipeable);

  return get_closest_snap_point (self);
}

static void
bis_carousel_swipeable_init (BisSwipeableInterface *iface)
{
  iface->get_distance = bis_carousel_get_distance;
  iface->get_snap_points = bis_carousel_get_snap_points;
  iface->get_progress = bis_carousel_get_progress;
  iface->get_cancel_progress = bis_carousel_get_cancel_progress;
}

/**
 * bis_carousel_new:
 *
 * Creates a new `BisCarousel`.
 *
 * Returns: the newly created `BisCarousel`
 *
 * Since: 1.0
 */
GtkWidget *
bis_carousel_new (void)
{
  return g_object_new (BIS_TYPE_CAROUSEL, NULL);
}

/**
 * bis_carousel_prepend:
 * @self: a carousel
 * @child: a widget to add
 *
 * Prepends @child to @self.
 *
 * Since: 1.0
 */
void
bis_carousel_prepend (BisCarousel *self,
                      GtkWidget   *widget)
{
  g_return_if_fail (BIS_IS_CAROUSEL (self));
  g_return_if_fail (GTK_IS_WIDGET (widget));

  bis_carousel_insert (self, widget, 0);
}

/**
 * bis_carousel_append:
 * @self: a carousel
 * @child: a widget to add
 *
 * Appends @child to @self.
 *
 * Since: 1.0
 */
void
bis_carousel_append (BisCarousel *self,
                     GtkWidget   *widget)
{
  g_return_if_fail (BIS_IS_CAROUSEL (self));
  g_return_if_fail (GTK_IS_WIDGET (widget));

  bis_carousel_insert (self, widget, -1);
}

/**
 * bis_carousel_insert:
 * @self: a carousel
 * @child: a widget to add
 * @position: the position to insert @child at
 *
 * Inserts @child into @self at position @position.
 *
 * If position is -1, or larger than the number of pages,
 * @child will be appended to the end.
 *
 * Since: 1.0
 */
void
bis_carousel_insert (BisCarousel *self,
                     GtkWidget   *widget,
                     int          position)
{
  ChildInfo *info;
  GList *next_link = NULL;

  g_return_if_fail (BIS_IS_CAROUSEL (self));
  g_return_if_fail (GTK_IS_WIDGET (widget));
  g_return_if_fail (position >= -1);

  info = g_new0 (ChildInfo, 1);
  info->widget = widget;
  info->size = 0;
  info->adding = TRUE;

  if (position >= 0)
    next_link = get_nth_link (self, position);

  self->children = g_list_insert_before (self->children, next_link, info);

  if (next_link) {
    ChildInfo *next_sibling = next_link->data;

    gtk_widget_insert_before (widget, GTK_WIDGET (self), next_sibling->widget);
  } else {
    gtk_widget_set_parent (widget, GTK_WIDGET (self));
  }

  gtk_widget_queue_allocate (GTK_WIDGET (self));

  animate_child_resize (self, info, 1, self->reveal_duration);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_N_PAGES]);
}
/**
 * bis_carousel_reorder:
 * @self: a carousel
 * @child: a widget to add
 * @position: the position to move @child to
 *
 * Moves @child into position @position.
 *
 * If position is -1, or larger than the number of pages, @child will be moved
 * at the end.
 *
 * Since: 1.0
 */
void
bis_carousel_reorder (BisCarousel *self,
                      GtkWidget   *child,
                      int          position)
{
  ChildInfo *info, *next_info;
  GList *link, *next_link;
  int old_position, n_pages;
  double closest_point, old_point, new_point;

  g_return_if_fail (BIS_IS_CAROUSEL (self));
  g_return_if_fail (GTK_IS_WIDGET (child));
  g_return_if_fail (position >= -1);

  closest_point = get_closest_snap_point (self);

  info = find_child_info (self, child);
  link = g_list_find (self->children, info);
  old_position = g_list_position (self->children, link);

  if (position == old_position)
    return;

  old_point = info->snap_point;
  n_pages = bis_carousel_get_n_pages (self);

  if (position < 0 || position > n_pages)
    position = n_pages;

  if (old_position == n_pages - 1 && position == n_pages)
    return;

  if (position == n_pages)
    next_link = NULL;
  else if (position > old_position)
    next_link = get_nth_link (self, position + 1);
  else
    next_link = get_nth_link (self, position);

  if (next_link) {
    next_info = next_link->data;
    new_point = next_info->snap_point;

    /* Since we know position > old_position, it's not 0 so prev_info exists */
    if (position > old_position) {
      ChildInfo *prev_info = next_link->prev->data;

      new_point = prev_info->snap_point;
    }
  } else {
    GList *last_link = g_list_last (self->children);
    ChildInfo *last_info = last_link->data;

    new_point = last_info->snap_point;
  }

  self->children = g_list_remove_link (self->children, link);

  if (next_link) {
    self->children = g_list_insert_before_link (self->children, next_link, link);

    gtk_widget_insert_before (child, GTK_WIDGET (self), next_info->widget);
  } else {
    self->children = g_list_append (self->children, info);
    g_list_free (link);

    gtk_widget_insert_before (child, GTK_WIDGET (self), NULL);
  }

  if (closest_point == old_point)
    self->position_shift += new_point - old_point;
  else if (old_point >= closest_point && closest_point >= new_point)
    self->position_shift += info->size;
  else if (new_point >= closest_point && closest_point >= old_point)
    self->position_shift -= info->size;

  gtk_widget_queue_allocate (GTK_WIDGET (self));
}

/**
 * bis_carousel_remove:
 * @self: a carousel
 * @child: a widget to remove
 *
 * Removes @child from @self.
 *
 * Since: 1.0
 */
void
bis_carousel_remove (BisCarousel *self,
                     GtkWidget   *child)
{
  ChildInfo *info;

  g_return_if_fail (BIS_IS_CAROUSEL (self));
  g_return_if_fail (GTK_IS_WIDGET (child));
  g_return_if_fail (gtk_widget_get_parent (child) == GTK_WIDGET (self));

  info = find_child_info (self, child);

  g_assert_nonnull (info);

  info->removing = TRUE;

  gtk_widget_unparent (child);

  info->widget = NULL;

  if (!gtk_widget_in_destruction (GTK_WIDGET (self)))
    animate_child_resize (self, info, 0, self->reveal_duration);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_N_PAGES]);
}

/**
 * bis_carousel_scroll_to:
 * @self: a carousel
 * @widget: a child of @self
 * @animate: whether to animate the transition
 *
 * Scrolls to @widget.
 *
 * If @animate is `TRUE`, the transition will be animated.
 *
 * Since: 1.0
 */
void
bis_carousel_scroll_to (BisCarousel *self,
                        GtkWidget   *widget,
                        gboolean     animate)
{
  g_return_if_fail (BIS_IS_CAROUSEL (self));
  g_return_if_fail (GTK_IS_WIDGET (widget));
  g_return_if_fail (gtk_widget_get_parent (widget) == GTK_WIDGET (self));

  scroll_to (self, widget, 0);

  if (!animate)
    bis_animation_skip (self->animation);
}

/**
 * bis_carousel_get_nth_page:
 * @self: a carousel
 * @n: index of the page
 *
 * Gets the page at position @n.
 *
 * Returns: (transfer none): the page
 *
 * Since: 1.0
 */
GtkWidget *
bis_carousel_get_nth_page (BisCarousel *self,
                           guint        n)
{
  ChildInfo *info;

  g_return_val_if_fail (BIS_IS_CAROUSEL (self), NULL);
  g_return_val_if_fail (n < bis_carousel_get_n_pages (self), NULL);

  info = get_nth_link (self, n)->data;

  return info->widget;
}

/**
 * bis_carousel_get_n_pages: (attributes org.gtk.Method.get_property=n-pages)
 * @self: a carousel
 *
 * Gets the number of pages in @self.
 *
 * Returns: the number of pages in @self
 *
 * Since: 1.0
 */
guint
bis_carousel_get_n_pages (BisCarousel *self)
{
  GList *l;
  guint n_pages;

  g_return_val_if_fail (BIS_IS_CAROUSEL (self), 0);

  n_pages = 0;
  for (l = self->children; l; l = l->next) {
    ChildInfo *child = l->data;

    if (!child->removing)
      n_pages++;
  }

  return n_pages;
}

/**
 * bis_carousel_get_position: (attributes org.gtk.Method.get_property=position)
 * @self: a carousel
 *
 * Gets current scroll position in @self, unitless.
 *
 * 1 matches 1 page. Use [method@Carousel.scroll_to] for changing it.
 *
 * Returns: the scroll position
 *
 * Since: 1.0
 */
double
bis_carousel_get_position (BisCarousel *self)
{
  g_return_val_if_fail (BIS_IS_CAROUSEL (self), 0.0);

  return self->position;
}

/**
 * bis_carousel_get_interactive: (attributes org.gtk.Method.get_property=interactive)
 * @self: a carousel
 *
 * Gets whether @self can be navigated.
 *
 * Returns: whether @self can be navigated
 *
 * Since: 1.0
 */
gboolean
bis_carousel_get_interactive (BisCarousel *self)
{
  g_return_val_if_fail (BIS_IS_CAROUSEL (self), FALSE);

  return bis_swipe_tracker_get_enabled (self->tracker);
}

/**
 * bis_carousel_set_interactive: (attributes org.gtk.Method.set_property=interactive)
 * @self: a carousel
 * @interactive: whether @self can be navigated
 *
 * Sets whether @self can be navigated.
 *
 * This can be used to temporarily disable the carousel to only allow navigating
 * it in a certain state.
 *
 * Since: 1.0
 */
void
bis_carousel_set_interactive (BisCarousel *self,
                              gboolean     interactive)
{
  g_return_if_fail (BIS_IS_CAROUSEL (self));

  interactive = !!interactive;

  if (bis_swipe_tracker_get_enabled (self->tracker) == interactive)
    return;

  bis_swipe_tracker_set_enabled (self->tracker, interactive);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_INTERACTIVE]);
}

/**
 * bis_carousel_get_spacing: (attributes org.gtk.Method.get_property=spacing)
 * @self: a carousel
 *
 * Gets spacing between pages in pixels.
 *
 * Returns: spacing between pages
 *
 * Since: 1.0
 */
guint
bis_carousel_get_spacing (BisCarousel *self)
{
  g_return_val_if_fail (BIS_IS_CAROUSEL (self), 0);

  return self->spacing;
}

/**
 * bis_carousel_set_spacing: (attributes org.gtk.Method.set_property=spacing)
 * @self: a carousel
 * @spacing: the new spacing value
 *
 * Sets spacing between pages in pixels.
 *
 * Since: 1.0
 */
void
bis_carousel_set_spacing (BisCarousel *self,
                          guint        spacing)
{
  g_return_if_fail (BIS_IS_CAROUSEL (self));

  if (self->spacing == spacing)
    return;

  self->spacing = spacing;
  gtk_widget_queue_resize (GTK_WIDGET (self));

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_SPACING]);
}

/**
 * bis_carousel_get_scroll_params: (attributes org.gtk.Method.get_property=scroll-params)
 * @self: a carousel
 *
 * Gets the scroll animation spring parameters for @self.
 *
 * Returns: the animation parameters
 *
 * Since: 1.0
 */
BisSpringParams *
bis_carousel_get_scroll_params (BisCarousel *self)
{
  g_return_val_if_fail (BIS_IS_CAROUSEL (self), NULL);

  return bis_spring_animation_get_spring_params (BIS_SPRING_ANIMATION (self->animation));
}

/**
 * bis_carousel_set_scroll_params: (attributes org.gtk.Method.set_property=scroll-params)
 * @self: a carousel
 * @params: the new parameters
 *
 * Sets the scroll animation spring parameters for @self.
 *
 * The default value is equivalent to:
 *
 * ```c
 * bis_spring_params_new (1, 0.5, 500)
 * ```
 *
 * Since: 1.0
 */
void
bis_carousel_set_scroll_params (BisCarousel     *self,
                                BisSpringParams *params)
{
  g_return_if_fail (BIS_IS_CAROUSEL (self));
  g_return_if_fail (params != NULL);

  if (bis_carousel_get_scroll_params (self) == params)
    return;

  bis_spring_animation_set_spring_params (BIS_SPRING_ANIMATION (self->animation), params);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_SCROLL_PARAMS]);
}

/**
 * bis_carousel_get_allow_mouse_drag: (attributes org.gtk.Method.get_property=allow-mouse-drag)
 * @self: a carousel
 *
 * Sets whether @self can be dragged with mouse pointer.
 *
 * Returns: whether @self can be dragged with mouse pointer
 *
 * Since: 1.0
 */
gboolean
bis_carousel_get_allow_mouse_drag (BisCarousel *self)
{
  g_return_val_if_fail (BIS_IS_CAROUSEL (self), FALSE);

  return bis_swipe_tracker_get_allow_mouse_drag (self->tracker);
}

/**
 * bis_carousel_set_allow_mouse_drag: (attributes org.gtk.Method.set_property=allow-mouse-drag)
 * @self: a carousel
 * @allow_mouse_drag: whether @self can be dragged with mouse pointer
 *
 * Sets whether @self can be dragged with mouse pointer.
 *
 * If @allow_mouse_drag is `FALSE`, dragging is only available on touch.
 *
 * Since: 1.0
 */
void
bis_carousel_set_allow_mouse_drag (BisCarousel *self,
                                   gboolean     allow_mouse_drag)
{
  g_return_if_fail (BIS_IS_CAROUSEL (self));

  allow_mouse_drag = !!allow_mouse_drag;

  if (bis_carousel_get_allow_mouse_drag (self) == allow_mouse_drag)
    return;

  bis_swipe_tracker_set_allow_mouse_drag (self->tracker, allow_mouse_drag);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_ALLOW_MOUSE_DRAG]);
}

/**
 * bis_carousel_get_allow_scroll_wheel: (attributes org.gtk.Method.get_property=allow-scroll-wheel)
 * @self: a carousel
 *
 * Gets whether @self will respond to scroll wheel events.
 *
 * Returns: `TRUE` if @self will respond to scroll wheel events
 *
 * Since: 1.0
 */
gboolean
bis_carousel_get_allow_scroll_wheel (BisCarousel *self)
{
  g_return_val_if_fail (BIS_IS_CAROUSEL (self), FALSE);

  return self->allow_scroll_wheel;
}

/**
 * bis_carousel_set_allow_scroll_wheel: (attributes org.gtk.Method.set_property=allow-scroll-wheel)
 * @self: a carousel
 * @allow_scroll_wheel: whether @self will respond to scroll wheel events
 *
 * Sets whether @self will respond to scroll wheel events.
 *
 * If @allow_scroll_wheel is `FALSE`, wheel events will be ignored.
 *
 * Since: 1.0
 */
void
bis_carousel_set_allow_scroll_wheel (BisCarousel *self,
                                     gboolean     allow_scroll_wheel)
{
  g_return_if_fail (BIS_IS_CAROUSEL (self));

  allow_scroll_wheel = !!allow_scroll_wheel;

  if (self->allow_scroll_wheel == allow_scroll_wheel)
    return;

  self->allow_scroll_wheel = allow_scroll_wheel;

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_ALLOW_SCROLL_WHEEL]);
}

/**
 * bis_carousel_get_allow_long_swipes: (attributes org.gtk.Method.get_property=allow-long-swipes)
 * @self: a carousel
 *
 * Gets whether to allow swiping for more than one page at a time.
 *
 * Returns: `TRUE` if long swipes are allowed
 *
 * Since: 1.0
 */
gboolean
bis_carousel_get_allow_long_swipes (BisCarousel *self)
{
  g_return_val_if_fail (BIS_IS_CAROUSEL (self), FALSE);

  return bis_swipe_tracker_get_allow_long_swipes (self->tracker);
}

/**
 * bis_carousel_set_allow_long_swipes: (attributes org.gtk.Method.set_property=allow-long-swipes)
 * @self: a carousel
 * @allow_long_swipes: whether to allow long swipes
 *
 * Sets whether to allow swiping for more than one page at a time.
 *
 * If @allow_long_swipes is `FALSE`, each swipe can only move to the adjacent
 * pages.
 *
 * Since: 1.0
 */
void
bis_carousel_set_allow_long_swipes (BisCarousel *self,
                                    gboolean     allow_long_swipes)
{
  g_return_if_fail (BIS_IS_CAROUSEL (self));

  allow_long_swipes = !!allow_long_swipes;

  if (bis_swipe_tracker_get_allow_long_swipes (self->tracker) == allow_long_swipes)
    return;

  bis_swipe_tracker_set_allow_long_swipes (self->tracker, allow_long_swipes);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_ALLOW_LONG_SWIPES]);
}

/**
 * bis_carousel_get_reveal_duration: (attributes org.gtk.Method.get_property=reveal-duration)
 * @self: a carousel
 *
 * Gets the page reveal duration, in milliseconds.
 *
 * Returns: the duration
 *
 * Since: 1.0
 */
guint
bis_carousel_get_reveal_duration (BisCarousel *self)
{
  g_return_val_if_fail (BIS_IS_CAROUSEL (self), 0);

  return self->reveal_duration;
}

/**
 * bis_carousel_set_reveal_duration: (attributes org.gtk.Method.set_property=reveal-duration)
 * @self: a carousel
 * @reveal_duration: the new reveal duration value
 *
 * Sets the page reveal duration, in milliseconds.
 *
 * Reveal duration is used when animating adding or removing pages.
 *
 * Since: 1.0
 */
void
bis_carousel_set_reveal_duration (BisCarousel *self,
                                  guint        reveal_duration)
{
  g_return_if_fail (BIS_IS_CAROUSEL (self));

  if (self->reveal_duration == reveal_duration)
    return;

  self->reveal_duration = reveal_duration;

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_REVEAL_DURATION]);
}
