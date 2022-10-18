/*
 * Copyright (C) 2019 Alexander Mikhaylenko <exalm7659@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once

#if !defined(_BISMUTH_INSIDE) && !defined(BISMUTH_COMPILATION)
#error "Only <bismuth.h> can be included directly."
#endif

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define BIS_TYPE_SHADOW_HELPER (bis_shadow_helper_get_type())

G_DECLARE_FINAL_TYPE (BisShadowHelper, bis_shadow_helper, BIS, SHADOW_HELPER, GObject)

BisShadowHelper *bis_shadow_helper_new (GtkWidget *widget) G_GNUC_WARN_UNUSED_RESULT;

void bis_shadow_helper_size_allocate (BisShadowHelper *self,
                                      int              width,
                                      int              height,
                                      int              baseline,
                                      int              x,
                                      int              y,
                                      double           progress,
                                      GtkPanDirection  direction);

void bis_shadow_helper_snapshot (BisShadowHelper *self,
                                 GtkSnapshot     *snapshot);

G_END_DECLS
