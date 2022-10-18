/*
 * Copyright (C) 2019 Alexander Mikhaylenko <exalm7659@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once

#if !defined(_BISMUTH_INSIDE) && !defined(BISMUTH_COMPILATION)
#error "Only <bismuth.h> can be included directly."
#endif

#include <glib-object.h>
#include "bis-enums.h"

G_BEGIN_DECLS

typedef enum {
  BIS_NAVIGATION_DIRECTION_BACK,
  BIS_NAVIGATION_DIRECTION_FORWARD,
} BisNavigationDirection;

G_END_DECLS
