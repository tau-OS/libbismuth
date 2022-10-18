/*
 * Copyright (C) 2020 Purism SPC
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#pragma once

#if !defined(_BISMUTH_INSIDE) && !defined(BISMUTH_COMPILATION)
#error "Only <bismuth.h> can be included directly."
#endif

#include "bis-version.h"

#include <glib.h>

G_BEGIN_DECLS

BIS_AVAILABLE_IN_ALL
void bis_init (void);

BIS_AVAILABLE_IN_ALL
gboolean bis_is_initialized (void);

G_END_DECLS
