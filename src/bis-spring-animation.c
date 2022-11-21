/*
 * Copyright (C) 2021 Manuel Genovés <manuel.genoves@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "config.h"

#include "bis-spring-animation.h"
#include "bis-spring-params.h"

#include "bis-animation-private.h"
#include "bis-animation-util.h"

#define DELTA 0.001
#define MAX_ITERATIONS 20000

/**
 * BisSpringAnimation:
 *
 * A spring-based [class@Animation].
 *
 * `BisSpringAnimation` implements an animation driven by a physical model of a 
 * spring described by [struct@SpringParams], with a resting position in
 * [property@SpringAnimation:value-to], stretched to
 * [property@SpringAnimation:value-from].
 * 
 * Since the animation is physically simulated, spring animations don't have a
 * fixed duration. The animation will stop when the simulated spring comes to a
 * rest - when the amplitude of the oscillations becomes smaller than
 * [property@SpringAnimation:epsilon], or immediately when it reaches
 * [property@SpringAnimation:value-to] if
 * [property@SpringAnimation:latch] is set to `TRUE`. The estimated duration can
 * be obtained with [property@SpringAnimation:estimated-duration].
 *
 * Due to the nature of spring-driven motion the animation can overshoot
 * [property@SpringAnimation:value-to] before coming to a rest. Whether the
 * animation will overshoot or not depends on the damping ratio of the spring.
 * See [struct@SpringParams] for more information about specific damping ratio
 * values.
 *
 * If [property@SpringAnimation:latch] is `TRUE`, the animation will abruptly
 * end as soon as it reaches the final value, preventing overshooting.
 *
 * Animations can have an initial velocity value, set via
 * [property@SpringAnimation:initial-velocity], which adjusts the curve without
 * changing the duration. This makes spring animations useful for deceleration
 * at the end of gestures.
 *
 * If the initial and final values are equal, and the initial velocity is not 0,
 * the animation value will bounce and return to its resting position.
 *
 * Since: 1.0
 */

struct _BisSpringAnimation
{
  BisAnimation parent_instance;

  double value_from;
  double value_to;

  BisSpringParams *spring_params;

  double initial_velocity;
  double velocity;
  double epsilon;
  gboolean latch;

  guint estimated_duration; /*ms*/
};

struct _BisSpringAnimationClass
{
  BisAnimationClass parent_class;
};

G_DEFINE_TYPE (BisSpringAnimation, bis_spring_animation, BIS_TYPE_ANIMATION)

enum {
  PROP_0,
  PROP_VALUE_FROM,
  PROP_VALUE_TO,
  PROP_SPRING_PARAMS,
  PROP_INITIAL_VELOCITY,
  PROP_EPSILON,
  PROP_CLAMP,
  PROP_ESTIMATED_DURATION,
  PROP_VELOCITY,
  LAST_PROP,
};

static GParamSpec *props[LAST_PROP];

/* Based on RBBSpringAnimation from RBBAnimation, MIT license.
 * https://github.com/robb/RBBAnimation/blob/master/RBBAnimation/RBBSpringAnimation.m
 *
 * @offset: Starting value of the spring simulation. Use -1 for regular animations,
 * as the formulas are tailored to rest at 0 and the resulting evolution between 
 * -1 and 0 will be lerped to the desired range afterwards. Otherwise use 0 for in-place
 * animations which already start at equilibrium
 */
static double
oscillate (BisSpringAnimation *self,
           guint               time,
           double             *velocity)
{
  double b = bis_spring_params_get_damping (self->spring_params);
  double m = bis_spring_params_get_mass (self->spring_params);
  double k = bis_spring_params_get_stiffness (self->spring_params);
  double v0 = self->initial_velocity;

  double t = time / 1000.0;

  double beta = b / (2 * m);
  double omega0 = sqrt (k / m);

  double x0 = self->value_from - self->value_to;

  double envelope = exp (-beta * t);

  /*
   * Solutions of the form C1*e^(lambda1*x) + C2*e^(lambda2*x)
   * for the differential equation m*ẍ+b*ẋ+kx = 0
   */

  /* Underdamped */
  if (beta < omega0) {
    double omega1 = sqrt ((omega0 * omega0) - (beta * beta));

    if (velocity)
      *velocity = envelope * (v0 * cos (omega1 * t) - (x0 * omega1 + (beta * beta * x0 + beta * v0) / (omega1)) * sin (omega1 * t));
    return self->value_to + envelope * (x0 * cos (omega1 * t) + ((beta * x0 + v0) / omega1) * sin (omega1 * t));
  }

  /* Overdamped */
  if (beta > omega0) {
    double omega2 = sqrt ((beta * beta) - (omega0 * omega0));

    if (velocity)
      *velocity = envelope * (v0 * coshl (omega2 * t) + (omega2 * x0 - (beta * beta * x0 + beta * v0) / omega2) * sinhl (omega2 * t));
    return self->value_to + envelope * (x0 * coshl (omega2 * t) + ((beta * x0 + v0) / omega2) * sinhl (omega2 * t));
  }

  /* Critically damped */
  if (velocity)
    *velocity = envelope * (beta * x0 + v0) * (1 - beta);
  return self->value_to + envelope * (x0 + (beta * x0 + v0) * t);
}

static guint
get_first_zero (BisSpringAnimation *self)
{
  /* The first frame is not that important and we avoid finding the trivial 0
   * for in-place animations. */
  guint i = 1;
  double y = oscillate (self, i, NULL);

  while ((self->value_to - self->value_from > FLT_EPSILON && self->value_to - y > self->epsilon) ||
         (self->value_from - self->value_to > FLT_EPSILON && y - self->value_to > self->epsilon)) {
    if (i > MAX_ITERATIONS)
      return 0;

    y = oscillate (self, ++i, NULL);
  }

  return i;
}

static guint
calculate_duration (BisSpringAnimation *self)
{
  double damping = bis_spring_params_get_damping (self->spring_params);
  double mass = bis_spring_params_get_mass (self->spring_params);
  double stiffness = bis_spring_params_get_stiffness (self->spring_params);

  double beta = damping / (2 * mass);
  double omega0;
  double x0, y0;
  double x1, y1;
  double m;

  int i = 0;

  if (beta <= 0)
    return BIS_DURATION_INFINITE;

  if (self->latch) {
    if (G_APPROX_VALUE (self->value_to, self->value_from, FLT_EPSILON))
      return 0;

    return get_first_zero (self);
  }

  omega0 = sqrt (stiffness / mass);

  /*
   * As first ansatz for the overdamped solution,
   * and general estimation for the oscillating ones
   * we take the value of the envelope when it's < epsilon
   */
  x0 = -log (self->epsilon) / beta;

  if (beta <= omega0)
    return x0 * 1000;

  /*
   * Since the overdamped solution decays way slower than the envelope
   * we need to use the value of the oscillation itself.
   * Newton's root finding method is a good candidate in this particular case:
   * https://en.wikipedia.org/wiki/Newton%27s_method
   */
  y0 = oscillate (self, x0*1000, NULL);
  m = (oscillate (self, (x0 + DELTA) * 1000, NULL) - y0) / DELTA;

  x1 = (self->value_to - y0 + m * x0) / m;
  y1 = oscillate (self, x1*1000, NULL);

  while (ABS (self->value_to - y1) > self->epsilon) {
    if (i>1000)
      return 0;
    x0 = x1;
    y0 = y1;

    m = (oscillate (self, (x0 + DELTA) * 1000, NULL) - y0) / DELTA;

    x1 = (self->value_to - y0 + m * x0) / m;
    y1 = oscillate (self, x1*1000, NULL);
    i++;
  }

  return x1 * 1000;
}

static void
estimate_duration (BisSpringAnimation *self)
{
  /* This function can be called during construction */
  if (!self->spring_params)
    return;

  self->estimated_duration = calculate_duration (self);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_ESTIMATED_DURATION]);
}

static guint 
bis_spring_animation_estimate_duration (BisAnimation *animation)
{
  BisSpringAnimation *self = BIS_SPRING_ANIMATION (animation);

  return self->estimated_duration;
}

static double
bis_spring_animation_calculate_value (BisAnimation *animation,
                                      guint         t)
{
  BisSpringAnimation *self = BIS_SPRING_ANIMATION (animation);
  double value;

  if (t >= self->estimated_duration) {
    self->velocity = 0;
    g_object_notify_by_pspec (G_OBJECT (self), props[PROP_VELOCITY]);

    return self->value_to;
  }

  value = oscillate (self, t, &self->velocity);
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_VELOCITY]);

  return value;
}

static void
bis_spring_animation_constructed (GObject *object)
{
  BisSpringAnimation *self = BIS_SPRING_ANIMATION (object);

  G_OBJECT_CLASS (bis_spring_animation_parent_class)->constructed (object);

  estimate_duration (self);
}

static void
bis_spring_animation_dispose (GObject *object)
{
  BisSpringAnimation *self = BIS_SPRING_ANIMATION (object);

  g_clear_pointer (&self->spring_params, bis_spring_params_unref);

  G_OBJECT_CLASS (bis_spring_animation_parent_class)->dispose (object);
}

static void
bis_spring_animation_get_property (GObject    *object,
                                   guint       prop_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  BisSpringAnimation *self = BIS_SPRING_ANIMATION (object);

  switch (prop_id) {
  case PROP_VALUE_FROM:
    g_value_set_double (value, bis_spring_animation_get_value_from (self));
    break;

  case PROP_VALUE_TO:
    g_value_set_double (value, bis_spring_animation_get_value_to (self));
    break;

  case PROP_SPRING_PARAMS:
    g_value_set_boxed (value, bis_spring_animation_get_spring_params (self));
    break;

  case PROP_INITIAL_VELOCITY:
    g_value_set_double (value, bis_spring_animation_get_initial_velocity (self));
    break;

  case PROP_EPSILON:
    g_value_set_double (value, bis_spring_animation_get_epsilon (self));
    break;

  case PROP_CLAMP:
    g_value_set_boolean (value, bis_spring_animation_get_latch (self));
    break;

  case PROP_ESTIMATED_DURATION:
    g_value_set_uint (value, bis_spring_animation_get_estimated_duration (self));
    break;

  case PROP_VELOCITY:
    g_value_set_double (value, bis_spring_animation_get_velocity (self));
    break;

  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
bis_spring_animation_set_property (GObject      *object,
                                   guint         prop_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  BisSpringAnimation *self = BIS_SPRING_ANIMATION (object);

  switch (prop_id) {
  case PROP_VALUE_FROM:
    bis_spring_animation_set_value_from (self, g_value_get_double (value));
    break;

  case PROP_VALUE_TO:
    bis_spring_animation_set_value_to (self, g_value_get_double (value));
    break;

  case PROP_SPRING_PARAMS:
    bis_spring_animation_set_spring_params (self, g_value_get_boxed (value));
    break;

  case PROP_INITIAL_VELOCITY:
    bis_spring_animation_set_initial_velocity (self, g_value_get_double (value));
    break;

  case PROP_EPSILON:
    bis_spring_animation_set_epsilon (self, g_value_get_double (value));
    break;

  case PROP_CLAMP:
    bis_spring_animation_set_latch (self, g_value_get_boolean (value));
    break;

  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
bis_spring_animation_class_init (BisSpringAnimationClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  BisAnimationClass *animation_class = BIS_ANIMATION_CLASS (klass);

  object_class->constructed = bis_spring_animation_constructed;
  object_class->dispose = bis_spring_animation_dispose;
  object_class->set_property = bis_spring_animation_set_property;
  object_class->get_property = bis_spring_animation_get_property;

  animation_class->estimate_duration = bis_spring_animation_estimate_duration;
  animation_class->calculate_value = bis_spring_animation_calculate_value;

  /**
   * BisSpringAnimation:value-from: (attributes org.gtk.Property.get=bis_spring_animation_get_value_from org.gtk.Property.set=bis_spring_animation_set_value_from)
   *
   * The value to animate from.
   *
   * The animation will start at this value and end at
   * [property@SpringAnimation:value-to].
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
   * BisSpringAnimation:value-to: (attributes org.gtk.Property.get=bis_spring_animation_get_value_to org.gtk.Property.set=bis_spring_animation_set_value_to)
   *
   * The value to animate to.
   *
   * The animation will start at [property@SpringAnimation:value-from] and end
   * at this value.
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
   * BisSpringAnimation:spring-params: (attributes org.gtk.Property.get=bis_spring_animation_get_spring_params org.gtk.Property.set=bis_spring_animation_set_spring_params)
   *
   * Physical parameters describing the spring.
   *
   * Since: 1.0
   */
  props[PROP_SPRING_PARAMS] =
    g_param_spec_boxed ("spring-params", NULL, NULL,
                        BIS_TYPE_SPRING_PARAMS,
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BisSpringAnimation:initial-velocity: (attributes org.gtk.Property.get=bis_spring_animation_get_initial_velocity org.gtk.Property.set=bis_spring_animation_set_initial_velocity)
   *
   * The initial velocity to start the animation with.
   *
   * Initial velocity affects only the animation curve, but not its duration.
   *
   * Since: 1.0
   */
  props[PROP_INITIAL_VELOCITY] =
    g_param_spec_double ("initial-velocity", NULL, NULL,
                         -G_MAXDOUBLE,
                         G_MAXDOUBLE,
                         0,
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BisSpringAnimation:epsilon: (attributes org.gtk.Property.get=bis_spring_animation_get_epsilon org.gtk.Property.set=bis_spring_animation_set_epsilon)
   *
   * Precision of the spring.
   *
   * The level of precision used to determine when the animation has come to a
   * rest, that is, when the amplitude of the oscillations becomes smaller than
   * this value.
   *
   * If the epsilon value is too small, the animation will take a long time to
   * stop after the animated value has stopped visibly changing.
   *
   * If the epsilon value is too large, the animation will end prematurely.
   *
   * The default value is 0.001.
   *
   * Since: 1.0
   */
  props[PROP_EPSILON] =
    g_param_spec_double ("epsilon", NULL, NULL,
                         0,
                         G_MAXDOUBLE,
                         0.001,
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BisSpringAnimation:latch: (attributes org.gtk.Property.get=bis_spring_animation_get_latch org.gtk.Property.set=bis_spring_animation_set_latch)
   *
   * Whether the animation should be latched.
   *
   * If set to `TRUE`, the animation will abruptly end as soon as it reaches the
   * final value, preventing overshooting.
   *
   * It won't prevent overshooting [property@SpringAnimation:value-from] if a
   * relative negative [property@SpringAnimation:initial-velocity] is set.
   *
   * Since: 1.0
   */
  props[PROP_CLAMP] =
    g_param_spec_boolean ("latch", NULL, NULL,
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BisSpringAnimation:estimated-duration: (attributes org.gtk.Property.get=bis_spring_animation_get_estimated_duration)
   *
   * Estimated duration of the animation, in milliseconds.
   *
   * Can be [const@DURATION_INFINITE] if the spring damping is set to 0.
   *
   * Since: 1.0
   */
  props[PROP_ESTIMATED_DURATION] =
    g_param_spec_uint ("estimated-duration", NULL, NULL,
                       0,
                       BIS_DURATION_INFINITE,
                       0,
                       G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  /**
   * BisSpringAnimation:velocity: (attributes org.gtk.Property.get=bis_spring_animation_get_velocity)
   *
   * Current velocity of the animation.
   *
   * Since: 1.0
   */
  props[PROP_VELOCITY] =
    g_param_spec_double ("velocity", NULL, NULL,
                         -G_MAXDOUBLE,
                         G_MAXDOUBLE,
                         0,
                         G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, LAST_PROP, props);
}

static void
bis_spring_animation_init (BisSpringAnimation *self)
{
  self->epsilon = 0.001;
}

/**
 * bis_spring_animation_new:
 * @widget: a widget to create animation on
 * @from: a value to animate from
 * @to: a value to animate to
 * @spring_params: (transfer full): physical parameters of the spring
 * @target: (transfer full): a target value to animate
 *
 * Creates a new `BisSpringAnimation` on @widget.
 *
 * The animation will animate @target from @from to @to with the dynamics of a
 * spring described by @spring_params.
 *
 * Returns: (transfer none): the newly created animation
 *
 * Since: 1.0
 */
BisAnimation *
bis_spring_animation_new (GtkWidget         *widget,
                          double              from,
                          double              to,
                          BisSpringParams    *spring_params,
                          BisAnimationTarget *target)
{
  BisAnimation *animation;

  g_return_val_if_fail (GTK_IS_WIDGET (widget), NULL);
  g_return_val_if_fail (spring_params != NULL, NULL);
  g_return_val_if_fail (BIS_IS_ANIMATION_TARGET (target), NULL);

  animation = g_object_new (BIS_TYPE_SPRING_ANIMATION,
                            "widget", widget,
                            "value-from", from,
                            "value-to", to,
                            "spring-params", spring_params,
                            "target", target,
                            NULL);

  g_object_unref (target);
  bis_spring_params_unref (spring_params);

  return animation;
}

/**
 * bis_spring_animation_get_value_from: (attributes org.gtk.Method.get_property=value-from)
 * @self: a spring animation
 *
 * Gets the value @self will animate from.
 *
 * Returns: the value to animate from
 *
 * Since: 1.0
 */
double
bis_spring_animation_get_value_from (BisSpringAnimation *self)
{
  g_return_val_if_fail (BIS_IS_SPRING_ANIMATION (self), 0.0);

  return self->value_from;
}

/**
 * bis_spring_animation_set_value_from: (attributes org.gtk.Method.set_property=value-from)
 * @self: a spring animation
 * @value: the value to animate from
 *
 * Sets the value @self will animate from.
 *
 * The animation will start at this value and end at
 * [property@SpringAnimation:value-to].
 *
 * Since: 1.0
 */
void
bis_spring_animation_set_value_from (BisSpringAnimation *self,
                                     double             value)
{
  g_return_if_fail (BIS_IS_SPRING_ANIMATION (self));

  if (G_APPROX_VALUE (self->value_from, value, FLT_EPSILON))
    return;

  self->value_from = value;

  estimate_duration (self);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_VALUE_FROM]);
}

/**
 * bis_spring_animation_get_value_to: (attributes org.gtk.Method.get_property=value-to)
 * @self: a spring animation
 *
 * Gets the value @self will animate to.
 *
 * Returns: the value to animate to
 *
 * Since: 1.0
 */
double
bis_spring_animation_get_value_to (BisSpringAnimation *self)
{
  g_return_val_if_fail (BIS_IS_SPRING_ANIMATION (self), 0.0);

  return self->value_to;
}

/**
 * bis_spring_animation_set_value_to: (attributes org.gtk.Method.set_property=value-to)
 * @self: a spring animation
 * @value: the value to animate to
 *
 * Sets the value @self will animate to.
 *
 * The animation will start at [property@SpringAnimation:value-from] and end at
 * this value.
 *
 * Since: 1.0
 */
void
bis_spring_animation_set_value_to (BisSpringAnimation *self,
                                   double             value)
{
  g_return_if_fail (BIS_IS_SPRING_ANIMATION (self));

  if (G_APPROX_VALUE (self->value_to, value, FLT_EPSILON))
    return;

  self->value_to = value;

  estimate_duration (self);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_VALUE_TO]);
}

/**
 * bis_spring_animation_get_spring_params: (attributes org.gtk.Method.get_property=spring-params)
 * @self: a spring animation
 *
 * Gets the physical parameters of the spring of @self.
 *
 * Returns: (transfer none): the spring parameters
 *
 * Since: 1.0
 */
BisSpringParams *
bis_spring_animation_get_spring_params (BisSpringAnimation *self)
{
  g_return_val_if_fail (BIS_IS_SPRING_ANIMATION (self), NULL);

  return self->spring_params;
}

/**
 * bis_spring_animation_set_spring_params: (attributes org.gtk.Method.set_property=spring-params)
 * @self: a spring animation
 * @spring_params: the new spring parameters
 *
 * Sets the physical parameters of the spring of @self.
 *
 * Since: 1.0
 */
void
bis_spring_animation_set_spring_params (BisSpringAnimation *self,
                                        BisSpringParams    *spring_params)
{
  g_return_if_fail (BIS_IS_SPRING_ANIMATION (self));
  g_return_if_fail (spring_params != NULL);

  if (self->spring_params == spring_params)
    return;

  g_clear_pointer (&self->spring_params, bis_spring_params_unref);
  self->spring_params = bis_spring_params_ref (spring_params);

  estimate_duration (self);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_SPRING_PARAMS]);
}

/**
 * bis_spring_animation_get_initial_velocity: (attributes org.gtk.Method.get_property=initial-velocity)
 * @self: a spring animation
 *
 * Gets the initial velocity of @self.
 *
 * Returns: the initial velocity
 *
 * Since: 1.0
 */
double
bis_spring_animation_get_initial_velocity (BisSpringAnimation *self)
{
  g_return_val_if_fail (BIS_IS_SPRING_ANIMATION (self), 0.0);

  return self->initial_velocity;
}

/**
 * bis_spring_animation_set_initial_velocity: (attributes org.gtk.Method.set_property=initial-velocity)
 * @self: a spring animation
 * @velocity: the initial velocity
 *
 * Sets the initial velocity of @self.
 *
 * Initial velocity affects only the animation curve, but not its duration.
 *
 * Since: 1.0
 */
void
bis_spring_animation_set_initial_velocity (BisSpringAnimation *self,
                                           double              velocity)
{
  g_return_if_fail (BIS_IS_SPRING_ANIMATION (self));

  if (G_APPROX_VALUE (self->initial_velocity, velocity, FLT_EPSILON))
    return;

  self->initial_velocity = velocity;

  estimate_duration (self);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_INITIAL_VELOCITY]);
}

/**
 * bis_spring_animation_get_epsilon: (attributes org.gtk.Method.get_property=epsilon)
 * @self: a spring animation
 *
 * Gets the precision of the spring.
 *
 * Returns: the epsilon value
 *
 * Since: 1.0
 */
double
bis_spring_animation_get_epsilon (BisSpringAnimation *self)
{
  g_return_val_if_fail (BIS_IS_SPRING_ANIMATION (self), 0.0);

  return self->epsilon;
}

/**
 * bis_spring_animation_set_epsilon: (attributes org.gtk.Method.set_property=epsilon)
 * @self: a spring animation
 * @epsilon: the new value
 *
 * Sets the precision of the spring.
 *
 * The level of precision used to determine when the animation has come to a
 * rest, that is, when the amplitude of the oscillations becomes smaller than
 * this value.
 *
 * If the epsilon value is too small, the animation will take a long time to
 * stop after the animated value has stopped visibly changing.
 *
 * If the epsilon value is too large, the animation will end prematurely.
 *
 * The default value is 0.001.
 *
 * Since: 1.0
 */
void
bis_spring_animation_set_epsilon (BisSpringAnimation *self,
                                  double              epsilon)
{
  g_return_if_fail (BIS_IS_SPRING_ANIMATION (self));
  g_return_if_fail (epsilon> 0.0);

  if (G_APPROX_VALUE (self->epsilon, epsilon, FLT_EPSILON))
    return;

  self->epsilon = epsilon;

  estimate_duration (self);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_EPSILON]);
}

/**
 * bis_spring_animation_get_latch: (attributes org.gtk.Method.get_property=latch)
 * @self: a spring animation
 *
 * Gets whether @self should be latched.
 *
 * Returns: whether @self is latched
 *
 * Since: 1.0
 */
gboolean
bis_spring_animation_get_latch (BisSpringAnimation *self)
{
  g_return_val_if_fail (BIS_IS_SPRING_ANIMATION (self), FALSE);

  return self->latch;
}

/**
 * bis_spring_animation_set_latch: (attributes org.gtk.Method.set_property=latch)
 * @self: a spring animation
 * @latch: the new value
 *
 * Sets whether @self should be latched.
 *
 * If set to `TRUE`, the animation will abruptly end as soon as it reaches the
 * final value, preventing overshooting.
 *
 * It won't prevent overshooting [property@SpringAnimation:value-from] if a
 * relative negative [property@SpringAnimation:initial-velocity] is set.
 *
 * Since: 1.0
 */
void
bis_spring_animation_set_latch (BisSpringAnimation *self,
                                gboolean            latch)
{
  g_return_if_fail (BIS_IS_SPRING_ANIMATION (self));

  if (self->latch == latch)
    return;

  self->latch = latch;

  estimate_duration (self);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_CLAMP]);
}

/**
 * bis_spring_animation_get_estimated_duration: (attributes org.gtk.Method.get_property=estimated-duration)
 * @self: a spring animation
 *
 * Gets the estimated duration of @self.
 *
 * Can be [const@DURATION_INFINITE] if the spring damping is set to 0.
 *
 * Returns: the estimated duration
 *
 * Since: 1.0
 */
guint
bis_spring_animation_get_estimated_duration (BisSpringAnimation *self)
{
  g_return_val_if_fail (BIS_IS_SPRING_ANIMATION (self), 0);

  return self->estimated_duration;
}

/**
 * bis_spring_animation_get_velocity: (attributes org.gtk.Method.get_property=velocity)
 * @self: a spring animation
 *
 * Gets the current velocity of @self.
 *
 * Returns: the current velocity
 *
 * Since: 1.0
 */
double
bis_spring_animation_get_velocity (BisSpringAnimation *self)
{
  g_return_val_if_fail (BIS_IS_SPRING_ANIMATION (self), 0.0);

  return self->velocity;
}
