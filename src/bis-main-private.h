/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#pragma once

#if !defined(_BISMUTH_INSIDE) && !defined(BISMUTH_COMPILATION)
#error "Only <bismuth.h> can be included directly."
#endif

#include "bis-main.h"

G_BEGIN_DECLS

/* Initializes the public GObject types, which is needed to ensure they are
 * discoverable, for example so they can easily be used with GtkBuilder.
 *
 * The function is implemented in bis-public-types.c which is generated at
 * compile time by gen-public-types.sh
 */
void bis_init_public_types (void);

G_END_DECLS
