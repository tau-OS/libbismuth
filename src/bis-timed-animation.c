/*
 * Copyright (C) 2021 Manuel Genovés <manuel.genoves@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "config.h"

#include "bis-timed-animation.h"

#include "bis-animation-private.h"
#include "bis-animation-util.h"
#include "bis-macros-private.h"

/**
 * BisTimedAnimation:
 *
 * A time-based [class@Animation].
 *
 * `BisTimedAnimation` implements a simple animation interpolating the given
 * value from [property@TimedAnimation:value-from] to
 * [property@TimedAnimation:value-to] over
 * [property@TimedAnimation:duration] milliseconds using the curve described by
 * [property@TimedAnimation:easing].
 *
 * If [property@TimedAnimation:reverse] is set to `TRUE`, `BisTimedAnimation`
 * will instead animate from [property@TimedAnimation:value-to] to
 * [property@TimedAnimation:value-from], and the easing curve will be inverted.
 *
 * The animation can repeat a certain amount of times, or endlessly, depending
 * on the [property@TimedAnimation:repeat-count] value. If
 * [property@TimedAnimation:alternate] is set to `TRUE`, it will also change the
 * direction every other iteration.
 *
 * Since: 1.0
 */

struct _BisTimedAnimation
{
  BisAnimation parent_instance;

  double value_from;
  double value_to;
  guint duration; /* ms */
  BisEasing easing;
  guint repeat_count;
  gboolean reverse;
  gboolean alternate;
};

struct _BisTimedAnimationClass
{
  BisAnimationClass parent_class;
};

G_DEFINE_FINAL_TYPE (BisTimedAnimation, bis_timed_animation, BIS_TYPE_ANIMATION)

enum {
  PROP_0,
  PROP_VALUE_FROM,
  PROP_VALUE_TO,
  PROP_DURATION,
  PROP_EASING,
  PROP_REPEAT_COUNT,
  PROP_REVERSE,
  PROP_ALTERNATE,
  LAST_PROP,
};

static GParamSpec *props[LAST_PROP];

static guint
bis_timed_animation_estimate_duration (BisAnimation *animation)
{
  BisTimedAnimation *self = BIS_TIMED_ANIMATION (animation);

  if (self->repeat_count == 0)
    return BIS_DURATION_INFINITE;

  return self->duration * self->repeat_count;
}

static double
bis_timed_animation_calculate_value (BisAnimation *animation,
                                     guint         t)
{
  BisTimedAnimation *self = BIS_TIMED_ANIMATION (animation);

  double value;
  double iteration, progress;
  gboolean reverse = false;

  if (self->duration == 0)
    return self->value_to;

  progress = modf (((double) t / self->duration), &iteration);

  if (self->alternate)
    reverse = ((int) iteration % 2);

  if (self->reverse)
    reverse = !reverse;

  /* When the animation ends, return the exact final value, which depends on the
     direction the animation is going at that moment, having into account that at the
     time of this check we're already on the next iteration. */
  if (t >= bis_timed_animation_estimate_duration (animation))
    return self->alternate == reverse ? self->value_to : self->value_from;

  progress = reverse ? (1 - progress) : progress;

  value = bis_easing_ease (self->easing, progress);

  return bis_lerp (self->value_from, self->value_to, value);
}

static void
bis_timed_animation_get_property (GObject    *object,
                                  guint       prop_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  BisTimedAnimation *self = BIS_TIMED_ANIMATION (object);

  switch (prop_id) {
  case PROP_VALUE_FROM:
    g_value_set_double (value, bis_timed_animation_get_value_from (self));
    break;

  case PROP_VALUE_TO:
    g_value_set_double (value, bis_timed_animation_get_value_to (self));
    break;

  case PROP_DURATION:
    g_value_set_uint (value, bis_timed_animation_get_duration (self));
    break;

  case PROP_EASING:
    g_value_set_enum (value, bis_timed_animation_get_easing (self));
    break;

  case PROP_REPEAT_COUNT:
    g_value_set_uint (value, bis_timed_animation_get_repeat_count (self));
    break;

  case PROP_REVERSE:
    g_value_set_boolean (value, bis_timed_animation_get_reverse (self));
    break;

  case PROP_ALTERNATE:
    g_value_set_boolean (value, bis_timed_animation_get_alternate (self));
    break;

  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
bis_timed_animation_set_property (GObject      *object,
                                  guint         prop_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  BisTimedAnimation *self = BIS_TIMED_ANIMATION (object);

  switch (prop_id) {
  case PROP_VALUE_FROM:
    bis_timed_animation_set_value_from (self, g_value_get_double (value));
    break;

  case PROP_VALUE_TO:
    bis_timed_animation_set_value_to (self, g_value_get_double (value));
    break;

  case PROP_DURATION:
    bis_timed_animation_set_duration (self, g_value_get_uint (value));
    break;

  case PROP_EASING:
    bis_timed_animation_set_easing (self, g_value_get_enum (value));
    break;

  case PROP_REPEAT_COUNT:
    bis_timed_animation_set_repeat_count (self, g_value_get_uint (value));
    break;

  case PROP_REVERSE:
    bis_timed_animation_set_reverse (self, g_value_get_boolean (value));
    break;

  case PROP_ALTERNATE:
    bis_timed_animation_set_alternate (self, g_value_get_boolean (value));
    break;

  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
bis_timed_animation_class_init (BisTimedAnimationClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  BisAnimationClass *animation_class = BIS_ANIMATION_CLASS (klass);

  object_class->set_property = bis_timed_animation_set_property;
  object_class->get_property = bis_timed_animation_get_property;

  animation_class->estimate_duration = bis_timed_animation_estimate_duration;
  animation_class->calculate_value = bis_timed_animation_calculate_value;

  /**
   * BisTimedAnimation:value-from: (attributes org.gtk.Property.get=bis_timed_animation_get_value_from org.gtk.Property.set=bis_timed_animation_set_value_from)
   *
   * The value to animate from.
   *
   * The animation will start at this value and end at
   * [property@TimedAnimation:value-to].
   *
   * If [property@TimedAnimation:reverse] is `TRUE`, the animation will end at
   * this value instead.
   *
   * Since: 1.0
   */
  props[PROP_VALUE_FROM] =
    g_param_spec_double ("value-from", NULL, NULL,
                         -G_MAXDOUBLE,
                         G_MAXDOUBLE,
                         0,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BisTimedAnimation:value-to: (attributes org.gtk.Property.get=bis_timed_animation_get_value_to org.gtk.Property.set=bis_timed_animation_set_value_to)
   *
   * The value to animate to.
   *
   * The animation will start at [property@TimedAnimation:value-from] and end at
   * this value.
   *
   * If [property@TimedAnimation:reverse] is `TRUE`, the animation will start
   * at this value instead.
   *
   * Since: 1.0
   */
  props[PROP_VALUE_TO] =
    g_param_spec_double ("value-to", NULL, NULL,
                         -G_MAXDOUBLE,
                         G_MAXDOUBLE,
                         0,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BisTimedAnimation:duration: (attributes org.gtk.Property.get=bis_timed_animation_get_duration org.gtk.Property.set=bis_timed_animation_set_duration)
   *
   * Duration of the animation, in milliseconds.
   *
   * Describes how much time the animation will take.
   *
   * If the animation repeats more than once, describes the duration of one
   * iteration.
   *
   * Since: 1.0
   */
  props[PROP_DURATION] =
    g_param_spec_uint ("duration", NULL, NULL,
                       0,
                       BIS_DURATION_INFINITE,
                       0,
                       G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BisTimedAnimation:easing: (attributes org.gtk.Property.get=bis_timed_animation_get_easing org.gtk.Property.set=bis_timed_animation_set_easing)
   *
   * Easing function used in the animation.
   *
   * Describes the curve the value is interpolated on.
   *
   * See [enum@Easing] for the description of specific easing functions.
   *
   * Since: 1.0
   */
  props[PROP_EASING] =
    g_param_spec_enum ("easing", NULL, NULL,
                       BIS_TYPE_EASING,
                       BIS_EASE_OUT_CUBIC,
                       G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BisTimedAnimation:repeat-count: (attributes org.gtk.Property.get=bis_timed_animation_get_repeat_count org.gtk.Property.set=bis_timed_animation_set_repeat_count)
   *
   * Number of times the animation will play.
   *
   * If set to 0, the animation will repeat endlessly.
   *
   * Since: 1.0
   */
  props[PROP_REPEAT_COUNT] =
    g_param_spec_uint ("repeat-count", NULL, NULL,
                       0,
                       G_MAXUINT,
                       1,
                       G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BisTimedAnimation:reverse: (attributes org.gtk.Property.get=bis_timed_animation_get_reverse org.gtk.Property.set=bis_timed_animation_set_reverse)
   *
   * Whether the animation plays backwards.
   *
   * Since: 1.0
   */
  props[PROP_REVERSE] =
    g_param_spec_boolean ("reverse", NULL, NULL,
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BisTimedAnimation:alternate: (attributes org.gtk.Property.get=bis_timed_animation_get_alternate org.gtk.Property.set=bis_timed_animation_set_alternate)
   *
   * Whether the animation changes direction on every iteration.
   *
   * Since: 1.0
   */
  props[PROP_ALTERNATE] =
    g_param_spec_boolean ("alternate", NULL, NULL,
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, LAST_PROP, props);
}

static void
bis_timed_animation_init (BisTimedAnimation *self)
{
}

/**
 * bis_timed_animation_new:
 * @widget: a widget to create animation on
 * @from: a value to animate from
 * @to: a value to animate to
 * @duration: a duration for the animation
 * @target: (transfer full): a target value to animate
 *
 * Creates a new `BisTimedAnimation` on @widget to animate @target from @from
 * to @to.
 *
 * Returns: (transfer none): the newly created animation
 *
 * Since: 1.0
 */
BisAnimation *
bis_timed_animation_new (GtkWidget          *widget,
                         double              from,
                         double              to,
                         guint               duration,
                         BisAnimationTarget *target)
{
  BisAnimation *animation;

  g_return_val_if_fail (GTK_IS_WIDGET (widget), NULL);
  g_return_val_if_fail (BIS_IS_ANIMATION_TARGET (target), NULL);

  animation = g_object_new (BIS_TYPE_TIMED_ANIMATION,
                            "widget", widget,
                            "value-from", from,
                            "value-to", to,
                            "duration", duration,
                            "target", target,
                            NULL);

  g_object_unref (target);

  return animation;
}

/**
 * bis_timed_animation_get_value_from: (attributes org.gtk.Method.get_property=value-from)
 * @self: a timed animation
 *
 * Gets the value @self will animate from.
 *
 * Returns: the value to animate from
 *
 * Since: 1.0
 */
double
bis_timed_animation_get_value_from (BisTimedAnimation *self)
{
  g_return_val_if_fail (BIS_IS_TIMED_ANIMATION (self), 0.0);

  return self->value_from;
}

/**
 * bis_timed_animation_set_value_from: (attributes org.gtk.Method.set_property=value-from)
 * @self: a timed animation
 * @value: the value to animate from
 *
 * Sets the value @self will animate from.
 *
 * The animation will start at this value and end at
 * [property@TimedAnimation:value-to].
 *
 * If [property@TimedAnimation:reverse] is `TRUE`, the animation will end at
 * this value instead.
 *
 * Since: 1.0
 */
void
bis_timed_animation_set_value_from (BisTimedAnimation *self,
                                    double             value)
{
  g_return_if_fail (BIS_IS_TIMED_ANIMATION (self));

  if (self->value_from == value)
    return;

  self->value_from = value;

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_VALUE_FROM]);
}

/**
 * bis_timed_animation_get_value_to: (attributes org.gtk.Method.get_property=value-to)
 * @self: a timed animation
 *
 * Gets the value @self will animate to.
 *
 * Returns: the value to animate to
 *
 * Since: 1.0
 */
double
bis_timed_animation_get_value_to (BisTimedAnimation *self)
{
  g_return_val_if_fail (BIS_IS_TIMED_ANIMATION (self), 0.0);

  return self->value_to;
}

/**
 * bis_timed_animation_set_value_to: (attributes org.gtk.Method.set_property=value-to)
 * @self: a timed animation
 * @value: the value to animate to
 *
 * Sets the value @self will animate to.
 *
 * The animation will start at [property@TimedAnimation:value-from] and end at
 * this value.
 *
 * If [property@TimedAnimation:reverse] is `TRUE`, the animation will start
 * at this value instead.
 *
 * Since: 1.0
 */
void
bis_timed_animation_set_value_to (BisTimedAnimation *self,
                                  double             value)
{
  g_return_if_fail (BIS_IS_TIMED_ANIMATION (self));

  if (self->value_to == value)
    return;

  self->value_to = value;

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_VALUE_TO]);
}

/**
 * bis_timed_animation_get_duration: (attributes org.gtk.Method.get_property=duration)
 * @self: a timed animation
 *
 * Gets the duration of @self.
 *
 * Returns: the duration of @self, in milliseconds
 *
 * Since: 1.0
 */
guint
bis_timed_animation_get_duration (BisTimedAnimation *self)
{
  g_return_val_if_fail (BIS_IS_TIMED_ANIMATION (self), 0);

  return self->duration;
}

/**
 * bis_timed_animation_set_duration: (attributes org.gtk.Method.set_property=duration)
 * @self: a timed animation
 * @duration: the duration to use, in milliseconds
 *
 * Sets the duration of @self.
 *
 * If the animation repeats more than once, sets the duration of one iteration.
 *
 * Since: 1.0
 */
void
bis_timed_animation_set_duration (BisTimedAnimation *self,
                                  guint              duration)
{
  g_return_if_fail (BIS_IS_TIMED_ANIMATION (self));

  if (self->duration == duration)
    return;

  self->duration = duration;

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_DURATION]);
}

/**
 * bis_timed_animation_get_easing: (attributes org.gtk.Method.get_property=easing)
 * @self: a timed animation
 *
 * Gets the easing function @self uses.
 *
 * Returns: the easing function @self uses
 *
 * Since: 1.0
 */
BisEasing
bis_timed_animation_get_easing (BisTimedAnimation *self)
{
  g_return_val_if_fail (BIS_IS_TIMED_ANIMATION (self),
                        BIS_LINEAR);

  return self->easing;
}

/**
 * bis_timed_animation_set_easing: (attributes org.gtk.Method.set_property=easing)
 * @self: a timed animation
 * @easing: the easing function to use
 *
 * Sets the easing function @self will use.
 *
 * See [enum@Easing] for the description of specific easing functions.
 *
 * Since: 1.0
 */
void
bis_timed_animation_set_easing (BisTimedAnimation *self,
                                BisEasing          easing)
{
  g_return_if_fail (BIS_IS_TIMED_ANIMATION (self));
  g_return_if_fail (easing <= BIS_EASE_IN_OUT_BOUNCE);

  if (self->easing == easing)
    return;

  self->easing = easing;

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_EASING]);
}

/**
 * bis_timed_animation_get_repeat_count: (attributes org.gtk.Method.get_property=repeat-count)
 * @self: a timed animation
 *
 * Gets the number of times @self will play.
 *
 * Returns: the number of times @self will play
 *
 * Since: 1.0
 */
guint
bis_timed_animation_get_repeat_count (BisTimedAnimation *self)
{
  g_return_val_if_fail (BIS_IS_TIMED_ANIMATION (self), 0);

  return self->repeat_count;
}

/**
 * bis_timed_animation_set_repeat_count: (attributes org.gtk.Method.set_property=repeat-count)
 * @self: a timed animation
 * @repeat_count: the number of times @self will play
 *
 * Sets the number of times @self will play.
 *
 * If set to 0, @self will repeat endlessly.
 *
 * Since: 1.0
 */
void
bis_timed_animation_set_repeat_count (BisTimedAnimation *self,
                                      guint              repeat_count)
{
  g_return_if_fail (BIS_IS_TIMED_ANIMATION (self));

  if (self->repeat_count == repeat_count)
    return;

  self->repeat_count = repeat_count;

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_REPEAT_COUNT]);
}

/**
 * bis_timed_animation_get_reverse: (attributes org.gtk.Method.get_property=reverse)
 * @self: a timed animation
 *
 * Gets whether @self plays backwards.
 *
 * Returns: whether @self plays backwards
 *
 * Since: 1.0
 */
gboolean
bis_timed_animation_get_reverse (BisTimedAnimation *self)
{
  g_return_val_if_fail (BIS_IS_TIMED_ANIMATION (self), FALSE);

  return self->reverse;
}

/**
 * bis_timed_animation_set_reverse: (attributes org.gtk.Method.set_property=reverse)
 * @self: a timed animation
 * @reverse: whether @self plays backwards
 *
 * Sets whether @self plays backwards.
 *
 * Since: 1.0
 */
void
bis_timed_animation_set_reverse (BisTimedAnimation *self,
                                 gboolean           reverse)
{
  g_return_if_fail (BIS_IS_TIMED_ANIMATION (self));

  if (self->reverse == reverse)
    return;

  self->reverse = reverse;

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_REVERSE]);
}

/**
 * bis_timed_animation_get_alternate: (attributes org.gtk.Method.get_property=alternate)
 * @self: a timed animation
 *
 * Gets whether @self changes direction on every iteration.
 *
 * Returns: whether @self alternates
 *
 * Since: 1.0
 */
gboolean
bis_timed_animation_get_alternate (BisTimedAnimation *self)
{
  g_return_val_if_fail (BIS_IS_TIMED_ANIMATION (self), FALSE);

  return self->alternate;
}

/**
 * bis_timed_animation_set_alternate: (attributes org.gtk.Method.set_property=alternate)
 * @self: a timed animation
 * @alternate: whether @self alternates
 *
 * Sets whether @self changes direction on every iteration.
 *
 * Since: 1.0
 */
void
bis_timed_animation_set_alternate (BisTimedAnimation *self,
                                   gboolean           alternate)
{
  g_return_if_fail (BIS_IS_TIMED_ANIMATION (self));

  if (self->alternate == alternate)
    return;

  self->alternate = alternate;

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_ALTERNATE]);
}
