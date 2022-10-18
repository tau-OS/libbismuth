/*
 * Copyright (C) 2019 Purism SPC
 * Copyright (C) 2021 Manuel Genovés <manuel.genoves@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once

#if !defined(_BISMUTH_INSIDE) && !defined(BISMUTH_COMPILATION)
#error "Only <bismuth.h> can be included directly."
#endif

#include "bis-animation.h"

G_BEGIN_DECLS

struct _BisAnimationClass
{
  GObjectClass parent_class;

  guint (*estimate_duration) (BisAnimation *self);

  double (*calculate_value) (BisAnimation *self,
                             guint         t);
};

G_END_DECLS
