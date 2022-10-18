/*
 * Copyright (C) 2021 Purism SPC
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * Author: Alexander Mikhaylenko <alexander.mikhaylenko@puri.sm>
 */

#pragma once

#if !defined(_BISMUTH_INSIDE) && !defined(BISMUTH_COMPILATION)
#error "Only <bismuth.h> can be included directly."
#endif

#include <glib-object.h>
#include "bis-enums-private.h"

G_BEGIN_DECLS

typedef enum {
  BIS_SYSTEM_COLOR_SCHEME_DEFAULT,
  BIS_SYSTEM_COLOR_SCHEME_PREFER_DARK,
  BIS_SYSTEM_COLOR_SCHEME_PREFER_LIGHT,
} BisSystemColorScheme;

#define BIS_TYPE_SETTINGS (bis_settings_get_type())

G_DECLARE_FINAL_TYPE (BisSettings, bis_settings, BIS, SETTINGS, GObject)

BIS_AVAILABLE_IN_ALL
BisSettings *bis_settings_get_default (void);

BIS_AVAILABLE_IN_ALL
gboolean bis_settings_get_system_supports_color_schemes (BisSettings *self);

BIS_AVAILABLE_IN_ALL
BisSystemColorScheme bis_settings_get_color_scheme (BisSettings *self);

BIS_AVAILABLE_IN_ALL
gboolean bis_settings_get_high_contrast (BisSettings *self);

BIS_AVAILABLE_IN_ALL
void bis_settings_start_override (BisSettings *self);
BIS_AVAILABLE_IN_ALL
void bis_settings_end_override   (BisSettings *self);

BIS_AVAILABLE_IN_ALL
void bis_settings_override_system_supports_color_schemes (BisSettings *self,
                                                          gboolean     system_supports_color_schemes);

BIS_AVAILABLE_IN_ALL
void bis_settings_override_color_scheme (BisSettings          *self,
                                         BisSystemColorScheme  color_scheme);

BIS_AVAILABLE_IN_ALL
void bis_settings_override_high_contrast (BisSettings *self,
                                          gboolean     high_contrast);

G_END_DECLS
