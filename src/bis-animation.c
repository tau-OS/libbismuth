/*
 * Copyright (C) 2019-2020 Purism SPC
 * Copyright (C) 2021 Manuel Genovés <manuel.genoves@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "config.h"

#include "bis-animation-private.h"

#include "bis-animation-target-private.h"
#include "bis-animation-util.h"

/**
 * BisAnimation:
 *
 * A base class for animations.
 *
 * `BisAnimation` represents an animation on a widget. It has a target that
 * provides a value to animate, and a state indicating whether the
 * animation hasn't been started yet, is playing, paused or finished.
 *
 * Currently there are two concrete animation types:
 * [class@TimedAnimation] and [class@SpringAnimation].
 *
 * `BisAnimation` will automatically skip the animation if
 * [property@Animation:widget] is unmapped, or if
 * [property@Gtk.Settings:gtk-enable-animations] is `FALSE`.
 *
 * The [signal@Animation::done] signal can be used to perform an action after
 * the animation ends, for example hiding a widget after animating its
 * [property@Gtk.Widget:opacity] to 0.
 *
 * `BisAnimation` will be kept alive while the animation is playing. As such,
 * it's safe to create an animation, start it and immediately unref it:
 * A fire-and-forget animation:
 *
 * ```c
 * static void
 * animation_cb (double    value,
 *               MyObject *self)
 * {
 *   // Do something with @value
 * }
 *
 * static void
 * my_object_animate (MyObject *self)
 * {
 *   BisAnimationTarget *target =
 *     bis_callback_animation_target_new ((BisAnimationTargetFunc) animation_cb,
 *                                        self, NULL);
 *   g_autoptr (BisAnimation) animation =
 *     bis_timed_animation_new (widget, 0, 1, 250, target);
 *
 *   bis_animation_play (animation);
 * }
 * ```
 *
 * If there's a chance the previous animation for the same target hasn't yet
 * finished, the previous animation should be stopped first, or the existing
 * `BisAnimation` object can be reused.
 *
 * Since: 1.0
 */

/**
 * BisAnimationState:
 * @BIS_ANIMATION_IDLE: The animation hasn't started yet.
 * @BIS_ANIMATION_PAUSED: The animation has been paused.
 * @BIS_ANIMATION_PLAYING: The animation is currently playing.
 * @BIS_ANIMATION_FINISHED: The animation has finished.
 *
 * Describes the possible states of an [class@Animation].
 *
 * The state can be controlled with [method@Animation.play],
 * [method@Animation.pause], [method@Animation.resume],
 * [method@Animation.reset] and [method@Animation.skip].
 *
 * Since: 1.0
 */

typedef struct
{
  GtkWidget *widget;

  double value;

  gint64 start_time; /* ms */
  gint64 paused_time;
  guint tick_cb_id;
  gulong unmap_cb_id;

  BisAnimationTarget *target;
  gpointer user_data;

  BisAnimationState state;
} BisAnimationPrivate;

G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE (BisAnimation, bis_animation, G_TYPE_OBJECT)

enum {
  PROP_0,
  PROP_WIDGET,
  PROP_TARGET,
  PROP_VALUE,
  PROP_STATE,
  LAST_PROP,
};

static GParamSpec *props[LAST_PROP];

enum {
  SIGNAL_DONE,
  SIGNAL_LAST_SIGNAL,
};

static guint signals[SIGNAL_LAST_SIGNAL];

static void
widget_notify_cb (BisAnimation *self)
{
  BisAnimationPrivate *priv = bis_animation_get_instance_private (self);

  priv->widget = NULL;
}

static void
set_widget (BisAnimation *self,
            GtkWidget    *widget)
{
  BisAnimationPrivate *priv = bis_animation_get_instance_private (self);

  if (priv->widget == widget)
    return;

  if (priv->widget)
    g_object_weak_unref (G_OBJECT (priv->widget),
                         (GWeakNotify) widget_notify_cb,
                         self);

  priv->widget = widget;

  if (priv->widget)
    g_object_weak_ref (G_OBJECT (priv->widget),
                       (GWeakNotify) widget_notify_cb,
                       self);
}

static void
set_value (BisAnimation *self,
           guint         t)
{
  BisAnimationPrivate *priv = bis_animation_get_instance_private (self);

  priv->value = BIS_ANIMATION_GET_CLASS (self)->calculate_value (self, t);

  bis_animation_target_set_value (priv->target, priv->value);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_VALUE]);
}

static void
stop_animation (BisAnimation *self)
{
  BisAnimationPrivate *priv = bis_animation_get_instance_private (self);

  if (priv->tick_cb_id) {
    gtk_widget_remove_tick_callback (priv->widget, priv->tick_cb_id);
    priv->tick_cb_id = 0;
  }

  if (priv->unmap_cb_id) {
    g_signal_handler_disconnect (priv->widget, priv->unmap_cb_id);
    priv->unmap_cb_id = 0;
  }
}

static gboolean
tick_cb (GtkWidget     *widget,
         GdkFrameClock *frame_clock,
         BisAnimation  *self)
{
  BisAnimationPrivate *priv = bis_animation_get_instance_private (self);

  gint64 frame_time = gdk_frame_clock_get_frame_time (frame_clock) / 1000; /* ms */
  guint duration = BIS_ANIMATION_GET_CLASS (self)->estimate_duration (self);
  guint t = (guint) (frame_time - priv->start_time);

  if (t >= duration && duration != BIS_DURATION_INFINITE) {
    bis_animation_skip (self);

    return G_SOURCE_REMOVE;
  }

  set_value (self, t);

  return G_SOURCE_CONTINUE;
}

static guint
bis_animation_estimate_duration (BisAnimation *animation)
{
  g_assert_not_reached ();
}

static double
bis_animation_calculate_value (BisAnimation *animation,
                               guint         t)
{
  g_assert_not_reached ();
}

static void
play (BisAnimation *self)
{

  BisAnimationPrivate *priv = bis_animation_get_instance_private (self);

  if (priv->state == BIS_ANIMATION_PLAYING) {
    g_critical ("Trying to play animation %p, but it's already playing", self);

    return;
  }

  priv->state = BIS_ANIMATION_PLAYING;
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_STATE]);

  if (!bis_get_enable_animations (priv->widget) ||
      !gtk_widget_get_mapped (priv->widget)) {
    bis_animation_skip (g_object_ref (self));

    return;
  }

  priv->start_time += gdk_frame_clock_get_frame_time (gtk_widget_get_frame_clock (priv->widget)) / 1000;
  priv->start_time -= priv->paused_time;

  if (priv->tick_cb_id)
    return;

  priv->unmap_cb_id =
    g_signal_connect_swapped (priv->widget, "unmap",
                              G_CALLBACK (bis_animation_skip), self);
  priv->tick_cb_id = gtk_widget_add_tick_callback (priv->widget, (GtkTickCallback) tick_cb, self, NULL);

  g_object_ref (self);
}

static void
bis_animation_constructed (GObject *object)
{
  BisAnimation *self = BIS_ANIMATION (object);
  BisAnimationPrivate *priv = bis_animation_get_instance_private (self);

  G_OBJECT_CLASS (bis_animation_parent_class)->constructed (object);

  priv->value = BIS_ANIMATION_GET_CLASS (self)->calculate_value (self, 0);
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_VALUE]);
}

static void
bis_animation_dispose (GObject *object)
{
  BisAnimation *self = BIS_ANIMATION (object);
  BisAnimationPrivate *priv = bis_animation_get_instance_private (self);

  if (priv->state == BIS_ANIMATION_PLAYING)
    bis_animation_skip (self);

  g_clear_object (&priv->target);

  set_widget (self, NULL);

  G_OBJECT_CLASS (bis_animation_parent_class)->dispose (object);
}

static void
bis_animation_get_property (GObject    *object,
                            guint       prop_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  BisAnimation *self = BIS_ANIMATION (object);

  switch (prop_id) {
  case PROP_WIDGET:
    g_value_set_object (value, bis_animation_get_widget (self));
    break;

  case PROP_TARGET:
    g_value_set_object (value, bis_animation_get_target (self));
    break;

  case PROP_VALUE:
    g_value_set_double (value, bis_animation_get_value (self));
    break;

  case PROP_STATE:
    g_value_set_enum (value, bis_animation_get_state (self));
    break;

  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
bis_animation_set_property (GObject      *object,
                            guint         prop_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  BisAnimation *self = BIS_ANIMATION (object);

  switch (prop_id) {
  case PROP_WIDGET:
    set_widget (self, g_value_get_object (value));
    break;

  case PROP_TARGET:
    bis_animation_set_target (BIS_ANIMATION (self), g_value_get_object (value));
    break;

  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
bis_animation_class_init (BisAnimationClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed = bis_animation_constructed;
  object_class->dispose = bis_animation_dispose;
  object_class->set_property = bis_animation_set_property;
  object_class->get_property = bis_animation_get_property;

  klass->estimate_duration = bis_animation_estimate_duration;
  klass->calculate_value = bis_animation_calculate_value;

  /**
   * BisAnimation:widget: (attributes org.gtk.Property.get=bis_animation_get_widget)
   *
   * The animation widget.
   *
   * It provides the frame clock for the animation. It's not strictly necessary
   * for this widget to be same as the one being animated.
   *
   * The widget must be mapped in order for the animation to work. If it's not
   * mapped, or if it gets unmapped during an ongoing animation, the animation
   * will be automatically skipped.
   *
   * Since: 1.0
   */
  props[PROP_WIDGET] =
    g_param_spec_object ("widget", NULL, NULL,
                         GTK_TYPE_WIDGET,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  /**
   * BisAnimation:target: (attributes org.gtk.Property.get=bis_animation_get_target org.gtk.Property.set=bis_animation_set_target)
   *
   * The target to animate.
   *
   * Since: 1.0
   */
  props[PROP_TARGET] =
    g_param_spec_object ("target", NULL, NULL,
                         BIS_TYPE_ANIMATION_TARGET,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BisAnimation:value: (attributes org.gtk.Property.get=bis_animation_get_value)
   *
   * The current value of the animation.
   *
   * Since: 1.0
   */
  props[PROP_VALUE] =
    g_param_spec_double ("value", NULL, NULL,
                         -G_MAXDOUBLE,
                         G_MAXDOUBLE,
                         0,
                         G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  /**
   * BisAnimation:state: (attributes org.gtk.Property.get=bis_animation_get_state)
   *
   * The animation state.
   *
   * The state indicates whether the animation is currently playing, paused,
   * finished or hasn't been started yet.
   *
   * Since: 1.0
   */
  props[PROP_STATE] =
    g_param_spec_enum ("state", NULL, NULL,
                       BIS_TYPE_ANIMATION_STATE,
                       BIS_ANIMATION_IDLE,
                       G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, LAST_PROP, props);

  /**
   * BisAnimation::done:
   *
   * This signal is emitted when the animation has been completed, either on its
   * own or via calling [method@Animation.skip].
   *
   * Since: 1.0
   */
  signals[SIGNAL_DONE] =
    g_signal_new ("done",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  0,
                  NULL, NULL, NULL,
                  G_TYPE_NONE,
                  0);
}

static void
bis_animation_init (BisAnimation *self)
{
  BisAnimationPrivate *priv = bis_animation_get_instance_private (self);

  priv->state = BIS_ANIMATION_IDLE;
}

/**
 * bis_animation_get_widget: (attributes org.gtk.Method.get_property=widget)
 * @self: an animation
 *
 * Gets the widget @self was created for.
 *
 * It provides the frame clock for the animation. It's not strictly necessary
 * for this widget to be same as the one being animated.
 *
 * The widget must be mapped in order for the animation to work. If it's not
 * mapped, or if it gets unmapped during an ongoing animation, the animation
 * will be automatically skipped.
 *
 * Returns: (transfer none): the animation widget
 *
 * Since: 1.0
 */
GtkWidget *
bis_animation_get_widget (BisAnimation *self)
{
  BisAnimationPrivate *priv;

  g_return_val_if_fail (BIS_IS_ANIMATION (self), NULL);

  priv = bis_animation_get_instance_private (self);

  return priv->widget;
}

/**
 * bis_animation_get_target: (attributes org.gtk.Method.get_property=target)
 * @self: an animation
 *
 * Gets the target @self animates.
 *
 * Returns: (transfer none): the animation target
 *
 * Since: 1.0
 */
BisAnimationTarget *
bis_animation_get_target (BisAnimation *self)
{
  BisAnimationPrivate *priv;

  g_return_val_if_fail (BIS_IS_ANIMATION (self), NULL);

  priv = bis_animation_get_instance_private (self);

  return priv->target;
}

/**
 * bis_animation_set_target: (attributes org.gtk.Method.set_property=target)
 * @self: an animation
 * @target: an animation target
 *
 * Sets the target @self animates to @target.
 *
 * Since: 1.0
 */
void
bis_animation_set_target (BisAnimation       *self,
                          BisAnimationTarget *target)
{
  BisAnimationPrivate *priv;

  g_return_if_fail (BIS_IS_ANIMATION (self));
  g_return_if_fail (BIS_IS_ANIMATION_TARGET (target));

  priv = bis_animation_get_instance_private (self);

  if (target == priv->target)
    return;

  g_set_object (&priv->target, target);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_TARGET]);
}

/**
 * bis_animation_get_value: (attributes org.gtk.Method.get_property=value)
 * @self: an animation
 *
 * Gets the current value of @self.
 *
 * Returns: the current value
 *
 * Since: 1.0
 */
double
bis_animation_get_value (BisAnimation *self)
{
  BisAnimationPrivate *priv;

  g_return_val_if_fail (BIS_IS_ANIMATION (self), 0.0);

  priv = bis_animation_get_instance_private (self);

  return priv->value;
}

/**
 * bis_animation_get_state: (attributes org.gtk.Method.get_property=state)
 * @self: an animation
 *
 * Gets the current value of @self.
 *
 * The state indicates whether @self is currently playing, paused, finished or
 * hasn't been started yet.
 *
 * Returns: the animation value
 *
 * Since: 1.0
 */
BisAnimationState
bis_animation_get_state (BisAnimation *self)
{
  BisAnimationPrivate *priv;

  g_return_val_if_fail (BIS_IS_ANIMATION (self), BIS_ANIMATION_IDLE);

  priv = bis_animation_get_instance_private (self);

  return priv->state;
}

/**
 * bis_animation_play:
 * @self: an animation
 *
 * Starts the animation for @self.
 *
 * If the animation is playing, paused or has been completed, restarts it from
 * the beginning. This allows to easily play an animation regardless of whether
 * it's already playing or not.
 *
 * Sets [property@Animation:state] to `BIS_ANIMATION_PLAYING`.
 *
 * The animation will be automatically skipped if [property@Animation:widget] is
 * unmapped, or if [property@Gtk.Settings:gtk-enable-animations] is `FALSE`.
 *
 * As such, it's not guaranteed that the animation will actually run. For
 * example, when using [func@GLib.idle_add] and starting an animation
 * immediately afterwards, it's entirely possible that the idle callback will
 * run after the animation has already finished, and not while it's playing.
 *
 * Since: 1.0
 */
void
bis_animation_play (BisAnimation *self)
{
  BisAnimationPrivate *priv;

  g_return_if_fail (BIS_IS_ANIMATION (self));

  priv = bis_animation_get_instance_private (self);

  if (priv->state != BIS_ANIMATION_IDLE) {
    priv->state = BIS_ANIMATION_IDLE;
    priv->start_time = 0;
    priv->paused_time = 0;
  }

  play (self);
}

/**
 * bis_animation_pause:
 * @self: an animation
 *
 * Pauses a playing animation for @self.
 *
 * Does nothing if the current state of @self isn't `BIS_ANIMATION_PLAYING`.
 *
 * Sets [property@Animation:state] to `BIS_ANIMATION_PAUSED`.
 *
 * Since: 1.0
 */
void
bis_animation_pause (BisAnimation *self)
{
  BisAnimationPrivate *priv;

  g_return_if_fail (BIS_IS_ANIMATION (self));

  priv = bis_animation_get_instance_private (self);

  if (priv->state != BIS_ANIMATION_PLAYING)
    return;

  g_object_freeze_notify (G_OBJECT (self));

  priv->state = BIS_ANIMATION_PAUSED;
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_STATE]);

  stop_animation (self);

  priv->paused_time = gdk_frame_clock_get_frame_time (gtk_widget_get_frame_clock (priv->widget)) / 1000;

  g_object_thaw_notify (G_OBJECT (self));

  g_object_unref (self);
}

/**
 * bis_animation_resume:
 * @self: an animation
 *
 * Resumes a paused animation for @self.
 *
 * This function must only be used if the animation has been paused with
 * [method@Animation.pause].
 *
 * Sets [property@Animation:state] to `BIS_ANIMATION_PLAYING`.
 *
 * Since: 1.0
 */
void
bis_animation_resume (BisAnimation *self)
{
  BisAnimationPrivate *priv;

  g_return_if_fail (BIS_IS_ANIMATION (self));

  priv = bis_animation_get_instance_private (self);

  if (priv->state != BIS_ANIMATION_PAUSED) {
    g_critical ("Trying to resume animation %p, but it's not paused", self);

    return;
  }

  play (self);
}

/**
 * bis_animation_skip:
 * @self: an animation
 *
 * Skips the animation for @self.
 *
 * If the animation hasn't been started yet, is playing, or is paused, instantly
 * skips the animation to the end and causes [signal@Animation::done] to be
 * emitted.
 *
 * Sets [property@Animation:state] to `BIS_ANIMATION_FINISHED`.
 *
 * Since: 1.0
 */
void
bis_animation_skip (BisAnimation *self)
{
  BisAnimationPrivate *priv;
  gboolean was_playing;

  g_return_if_fail (BIS_IS_ANIMATION (self));

  priv = bis_animation_get_instance_private (self);

  if (priv->state == BIS_ANIMATION_FINISHED)
    return;

  g_object_freeze_notify (G_OBJECT (self));

  was_playing = priv->state == BIS_ANIMATION_PLAYING;

  priv->state = BIS_ANIMATION_FINISHED;
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_STATE]);

  stop_animation (self);

  set_value (self, BIS_ANIMATION_GET_CLASS (self)->estimate_duration (self));

  priv->start_time = 0;
  priv->paused_time = 0;

  g_object_thaw_notify (G_OBJECT (self));

  g_signal_emit (self, signals[SIGNAL_DONE], 0);

  if (was_playing)
    g_object_unref (self);
}

/**
 * bis_animation_reset:
 * @self: an animation
 *
 * Resets the animation for @self.
 *
 * Sets [property@Animation:state] to `BIS_ANIMATION_IDLE`.
 *
 * Since: 1.0
 */
void
bis_animation_reset (BisAnimation *self)
{
  BisAnimationPrivate *priv;
  gboolean was_playing;

  g_return_if_fail (BIS_IS_ANIMATION (self));

  priv = bis_animation_get_instance_private (self);

  if (priv->state == BIS_ANIMATION_IDLE)
    return;

  g_object_freeze_notify (G_OBJECT (self));

  was_playing = priv->state == BIS_ANIMATION_PLAYING;

  priv->state = BIS_ANIMATION_IDLE;
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_STATE]);

  stop_animation (self);

  set_value (self, 0);
  priv->start_time = 0;
  priv->paused_time = 0;

  g_object_thaw_notify (G_OBJECT (self));

  if (was_playing)
    g_object_unref (self);
}
