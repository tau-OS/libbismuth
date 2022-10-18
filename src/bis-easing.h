/*
 * Copyright (C) 2021 Manuel Genovés <manuel.genoves@gmail.com>
 * Copyright (C) 2021 Purism SPC
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once

#if !defined(_BISMUTH_INSIDE) && !defined(BISMUTH_COMPILATION)
#error "Only <bismuth.h> can be included directly."
#endif

#include "bis-version.h"

#include <glib.h>

#include "bis-enums.h"

G_BEGIN_DECLS

typedef enum {
  BIS_LINEAR,
  BIS_EASE_IN_QUAD,
  BIS_EASE_OUT_QUAD,
  BIS_EASE_IN_OUT_QUAD,
  BIS_EASE_IN_CUBIC,
  BIS_EASE_OUT_CUBIC,
  BIS_EASE_IN_OUT_CUBIC,
  BIS_EASE_IN_QUART,
  BIS_EASE_OUT_QUART,
  BIS_EASE_IN_OUT_QUART,
  BIS_EASE_IN_QUINT,
  BIS_EASE_OUT_QUINT,
  BIS_EASE_IN_OUT_QUINT,
  BIS_EASE_IN_SINE,
  BIS_EASE_OUT_SINE,
  BIS_EASE_IN_OUT_SINE,
  BIS_EASE_IN_EXPO,
  BIS_EASE_OUT_EXPO,
  BIS_EASE_IN_OUT_EXPO,
  BIS_EASE_IN_CIRC,
  BIS_EASE_OUT_CIRC,
  BIS_EASE_IN_OUT_CIRC,
  BIS_EASE_IN_ELASTIC,
  BIS_EASE_OUT_ELASTIC,
  BIS_EASE_IN_OUT_ELASTIC,
  BIS_EASE_IN_BACK,
  BIS_EASE_OUT_BACK,
  BIS_EASE_IN_OUT_BACK,
  BIS_EASE_IN_BOUNCE,
  BIS_EASE_OUT_BOUNCE,
  BIS_EASE_IN_OUT_BOUNCE,
} BisEasing;

BIS_AVAILABLE_IN_ALL
double bis_easing_ease (BisEasing self,
                        double    value);

G_END_DECLS
