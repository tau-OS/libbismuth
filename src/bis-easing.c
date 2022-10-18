/*
 * Copyright (C) 2021 Manuel Genovés <manuel.genoves@gmail.com>
 * Copyright (C) 2021 Purism SPC
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "config.h"

#include "bis-easing.h"

#include <math.h>

/*
 * Copied from:
 *   https://gitlab.gnome.org/GNOME/clutter/-/blob/a236494ea7f31848b4a459dad41330f225137832/clutter/clutter-easing.c
 *   https://gitlab.gnome.org/GNOME/clutter/-/blob/a236494ea7f31848b4a459dad41330f225137832/clutter/clutter-enums.h
 *
 * Copyright (C) 2011  Intel Corporation
 */

/**
 * BisEasing:
 * @BIS_LINEAR: Linear tweening.
 * @BIS_EASE_IN_QUAD: Quadratic tweening.
 * @BIS_EASE_OUT_QUAD: Quadratic tweening, inverse of `BIS_EASE_IN_QUAD`.
 * @BIS_EASE_IN_OUT_QUAD: Quadratic tweening, combining `BIS_EASE_IN_QUAD` and
 *   `BIS_EASE_OUT_QUAD`.
 * @BIS_EASE_IN_CUBIC: Cubic tweening.
 * @BIS_EASE_OUT_CUBIC: Cubic tweening, inverse of `BIS_EASE_IN_CUBIC`.
 * @BIS_EASE_IN_OUT_CUBIC: Cubic tweening, combining `BIS_EASE_IN_CUBIC` and
 *   `BIS_EASE_OUT_CUBIC`.
 * @BIS_EASE_IN_QUART: Quartic tweening.
 * @BIS_EASE_OUT_QUART: Quartic tweening, inverse of `BIS_EASE_IN_QUART`.
 * @BIS_EASE_IN_OUT_QUART: Quartic tweening, combining `BIS_EASE_IN_QUART` and
 *   `BIS_EASE_OUT_QUART`.
 * @BIS_EASE_IN_QUINT: Quintic tweening.
 * @BIS_EASE_OUT_QUINT: Quintic tweening, inverse of `BIS_EASE_IN_QUINT`.
 * @BIS_EASE_IN_OUT_QUINT: Quintic tweening, combining `BIS_EASE_IN_QUINT` and
 *   `BIS_EASE_OUT_QUINT`.
 * @BIS_EASE_IN_SINE: Sine wave tweening.
 * @BIS_EASE_OUT_SINE: Sine wave tweening, inverse of `BIS_EASE_IN_SINE`.
 * @BIS_EASE_IN_OUT_SINE: Sine wave tweening, combining `BIS_EASE_IN_SINE` and
 *   `BIS_EASE_OUT_SINE`.
 * @BIS_EASE_IN_EXPO: Exponential tweening.
 * @BIS_EASE_OUT_EXPO: Exponential tweening, inverse of `BIS_EASE_IN_EXPO`.
 * @BIS_EASE_IN_OUT_EXPO: Exponential tweening, combining `BIS_EASE_IN_EXPO` and
 *   `BIS_EASE_OUT_EXPO`.
 * @BIS_EASE_IN_CIRC: Circular tweening.
 * @BIS_EASE_OUT_CIRC: Circular tweening, inverse of `BIS_EASE_IN_CIRC`.
 * @BIS_EASE_IN_OUT_CIRC: Circular tweening, combining `BIS_EASE_IN_CIRC` and
 *   `BIS_EASE_OUT_CIRC`.
 * @BIS_EASE_IN_ELASTIC: Elastic tweening, with offshoot on start.
 * @BIS_EASE_OUT_ELASTIC: Elastic tweening, with offshoot on end, inverse of
 *   `BIS_EASE_IN_ELASTIC`.
 * @BIS_EASE_IN_OUT_ELASTIC: Elastic tweening, with offshoot on both ends,
 *   combining `BIS_EASE_IN_ELASTIC` and `BIS_EASE_OUT_ELASTIC`.
 * @BIS_EASE_IN_BACK: Overshooting cubic tweening, with backtracking on start.
 * @BIS_EASE_OUT_BACK: Overshooting cubic tweening, with backtracking on end,
 *   inverse of `BIS_EASE_IN_BACK`.
 * @BIS_EASE_IN_OUT_BACK: Overshooting cubic tweening, with backtracking on both
 *   ends, combining `BIS_EASE_IN_BACK` and `BIS_EASE_OUT_BACK`.
 * @BIS_EASE_IN_BOUNCE: Exponentially decaying parabolic (bounce) tweening,
 *   on start.
 * @BIS_EASE_OUT_BOUNCE: Exponentially decaying parabolic (bounce) tweening,
 *   with bounce on end, inverse of `BIS_EASE_IN_BOUNCE`.
 * @BIS_EASE_IN_OUT_BOUNCE: Exponentially decaying parabolic (bounce) tweening,
 *   with bounce on both ends, combining `BIS_EASE_IN_BOUNCE` and
 *   `BIS_EASE_OUT_BOUNCE`.
 *
 * Describes the available easing functions for use with
 * [class@TimedAnimation].
 *
 * New values may be added to this enumeration over time.
 *
 * Since: 1.0
 */

static inline double
linear (double t,
        double d)
{
  return t / d;
}

static inline double
ease_in_quad (double t,
              double d)
{
  double p = t / d;

  return p * p;
}

static inline double
ease_out_quad (double t,
               double d)
{
  double p = t / d;

  return -1.0 * p * (p - 2);
}

static inline double
ease_in_out_quad (double t,
                  double d)
{
  double p = t / (d / 2);

  if (p < 1)
    return 0.5 * p * p;

  p -= 1;

  return -0.5 * (p * (p - 2) - 1);
}

static inline double
ease_in_cubic (double t,
               double d)
{
  double p = t / d;

  return p * p * p;
}

static inline double
ease_out_cubic (double t,
                double d)
{
  double p = t / d - 1;

  return p * p * p + 1;
}

static inline double
ease_in_out_cubic (double t,
                   double d)
{
  double p = t / (d / 2);

  if (p < 1)
    return 0.5 * p * p * p;

  p -= 2;

  return 0.5 * (p * p * p + 2);
}

static inline double
ease_in_quart (double t,
               double d)
{
  double p = t / d;

  return p * p * p * p;
}

static inline double
ease_out_quart (double t,
                double d)
{
  double p = t / d - 1;

  return -1.0 * (p * p * p * p - 1);
}

static inline double
ease_in_out_quart (double t,
                   double d)
{
  double p = t / (d / 2);

  if (p < 1)
    return 0.5 * p * p * p * p;

  p -= 2;

  return -0.5 * (p * p * p * p - 2);
}

static inline double
ease_in_quint (double t,
               double d)
{
  double p = t / d;

  return p * p * p * p * p;
}

static inline double
ease_out_quint (double t,
                double d)
{
  double p = t / d - 1;

  return p * p * p * p * p + 1;
}

static inline double
ease_in_out_quint (double t,
                   double d)
{
  double p = t / (d / 2);

  if (p < 1)
    return 0.5 * p * p * p * p * p;

  p -= 2;

  return 0.5 * (p * p * p * p * p + 2);
}

static inline double
ease_in_sine (double t,
              double d)
{
  return -1.0 * cos (t / d * G_PI_2) + 1.0;
}

static inline double
ease_out_sine (double t,
               double d)
{
  return sin (t / d * G_PI_2);
}

static inline double
ease_in_out_sine (double t,
                  double d)
{
  return -0.5 * (cos (G_PI * t / d) - 1);
}

static inline double
ease_in_expo (double t,
              double d)
{
  return (t == 0) ? 0.0 : pow (2, 10 * (t / d - 1));
}

static double
ease_out_expo (double t,
               double d)
{
  return (t == d) ? 1.0 : -pow (2, -10 * t / d) + 1;
}

static inline double
ease_in_out_expo (double t,
                  double d)
{
  double p;

  if (t == 0)
    return 0.0;

  if (t == d)
    return 1.0;

  p = t / (d / 2);

  if (p < 1)
    return 0.5 * pow (2, 10 * (p - 1));

  p -= 1;

  return 0.5 * (-pow (2, -10 * p) + 2);
}

static inline double
ease_in_circ (double t,
              double d)
{
  double p = t / d;

  return -1.0 * (sqrt (1 - p * p) - 1);
}

static inline double
ease_out_circ (double t,
               double d)
{
  double p = t / d - 1;

  return sqrt (1 - p * p);
}

static inline double
ease_in_out_circ (double t,
                  double d)
{
  double p = t / (d / 2);

  if (p < 1)
    return -0.5 * (sqrt (1 - p * p) - 1);

  p -= 2;

  return 0.5 * (sqrt (1 - p * p) + 1);
}

static inline double
ease_in_elastic (double t,
                 double d)
{
  double p = d * .3;
  double s = p / 4;
  double q = t / d;

  if (q == 1)
    return 1.0;

  q -= 1;

  return -(pow (2, 10 * q) * sin ((q * d - s) * (2 * G_PI) / p));
}

static inline double
ease_out_elastic (double t,
                  double d)
{
  double p = d * .3;
  double s = p / 4;
  double q = t / d;

  if (q == 1)
    return 1.0;

  return pow (2, -10 * q) * sin ((q * d - s) * (2 * G_PI) / p) + 1.0;
}

static inline double
ease_in_out_elastic (double t,
                     double d)
{
  double p = d * (.3 * 1.5);
  double s = p / 4;
  double q = t / (d / 2);

  if (q == 2)
    return 1.0;

  if (q < 1) {
    q -= 1;

    return -.5 * (pow (2, 10 * q) * sin ((q * d - s) * (2 * G_PI) / p));
  } else {
    q -= 1;

    return pow (2, -10 * q)
         * sin ((q * d - s) * (2 * G_PI) / p)
         * .5 + 1.0;
  }
}

static inline double
ease_in_back (double t,
              double d)
{
  double p = t / d;

  return p * p * ((1.70158 + 1) * p - 1.70158);
}

static inline double
ease_out_back (double t,
               double d)
{
  double p = t / d - 1;

  return p * p * ((1.70158 + 1) * p + 1.70158) + 1;
}

static inline double
ease_in_out_back (double t,
                  double d)
{
  double p = t / (d / 2);
  double s = 1.70158 * 1.525;

  if (p < 1)
    return 0.5 * (p * p * ((s + 1) * p - s));

  p -= 2;

  return 0.5 * (p * p * ((s + 1) * p + s) + 2);
}

static inline double
ease_out_bounce (double t,
                 double d)
{
  double p = t / d;

  if (p < (1 / 2.75)) {
    return 7.5625 * p * p;
  } else if (p < (2 / 2.75)) {
    p -= (1.5 / 2.75);

    return 7.5625 * p * p + .75;
  } else if (p < (2.5 / 2.75)) {
    p -= (2.25 / 2.75);

    return 7.5625 * p * p + .9375;
  } else {
    p -= (2.625 / 2.75);

    return 7.5625 * p * p + .984375;
  }
}

static inline double
ease_in_bounce (double t,
                double d)
{
  return 1.0 - ease_out_bounce (d - t, d);
}

static inline double
ease_in_out_bounce (double t,
                    double d)
{
  if (t < d / 2)
    return ease_in_bounce (t * 2, d) * 0.5;
  else
    return ease_out_bounce (t * 2 - d, d) * 0.5 + 1.0 * 0.5;
}

/**
 * bis_easing_ease:
 * @self: an easing value
 * @value: a value to ease
 *
 * Computes easing with @easing for @value.
 *
 * @value should generally be in the [0, 1] range.
 *
 * Returns: the easing for @value
 *
 * Since: 1.0
 */
double
bis_easing_ease (BisEasing self,
                 double    value)
{
  switch (self) {
    case BIS_LINEAR:
      return linear (value, 1);
    case BIS_EASE_IN_QUAD:
      return ease_in_quad (value, 1);
    case BIS_EASE_OUT_QUAD:
      return ease_out_quad (value, 1);
    case BIS_EASE_IN_OUT_QUAD:
      return ease_in_out_quad (value, 1);
    case BIS_EASE_IN_CUBIC:
      return ease_in_cubic (value, 1);
    case BIS_EASE_OUT_CUBIC:
      return ease_out_cubic (value, 1);
    case BIS_EASE_IN_OUT_CUBIC:
      return ease_in_out_cubic (value, 1);
    case BIS_EASE_IN_QUART:
      return ease_in_quart (value, 1);
    case BIS_EASE_OUT_QUART:
      return ease_out_quart (value, 1);
    case BIS_EASE_IN_OUT_QUART:
      return ease_in_out_quart (value, 1);
    case BIS_EASE_IN_QUINT:
      return ease_in_quint (value, 1);
    case BIS_EASE_OUT_QUINT:
      return ease_out_quint (value, 1);
    case BIS_EASE_IN_OUT_QUINT:
      return ease_in_out_quint (value, 1);
    case BIS_EASE_IN_SINE:
      return ease_in_sine (value, 1);
    case BIS_EASE_OUT_SINE:
      return ease_out_sine (value, 1);
    case BIS_EASE_IN_OUT_SINE:
      return ease_in_out_sine (value, 1);
    case BIS_EASE_IN_EXPO:
      return ease_in_expo (value, 1);
    case BIS_EASE_OUT_EXPO:
      return ease_out_expo (value, 1);
    case BIS_EASE_IN_OUT_EXPO:
      return ease_in_out_expo (value, 1);
    case BIS_EASE_IN_CIRC:
      return ease_in_circ (value, 1);
    case BIS_EASE_OUT_CIRC:
      return ease_out_circ (value, 1);
    case BIS_EASE_IN_OUT_CIRC:
      return ease_in_out_circ (value, 1);
    case BIS_EASE_IN_ELASTIC:
      return ease_in_elastic (value, 1);
    case BIS_EASE_OUT_ELASTIC:
      return ease_out_elastic (value, 1);
    case BIS_EASE_IN_OUT_ELASTIC:
      return ease_in_out_elastic (value, 1);
    case BIS_EASE_IN_BACK:
      return ease_in_back (value, 1);
    case BIS_EASE_OUT_BACK:
      return ease_out_back (value, 1);
    case BIS_EASE_IN_OUT_BACK:
      return ease_in_out_back (value, 1);
    case BIS_EASE_IN_BOUNCE:
      return ease_in_bounce (value, 1);
    case BIS_EASE_OUT_BOUNCE:
      return ease_out_bounce (value, 1);
    case BIS_EASE_IN_OUT_BOUNCE:
      return ease_in_out_bounce (value, 1);
    default:
      g_assert_not_reached ();
  }
}
