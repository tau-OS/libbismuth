/*
 * Copyright (C) 2021 Christopher Davis <christopherdavis@gnome.org>
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
  BIS_FOLD_THRESHOLD_POLICY_MINIMUM,
  BIS_FOLD_THRESHOLD_POLICY_NATURAL,
} BisFoldThresholdPolicy;

G_END_DECLS
