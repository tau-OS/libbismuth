/*
 * Copyright (C) 2019 Purism SPC
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once

#if !defined(_BISMUTH_INSIDE) && !defined(BISMUTH_COMPILATION)
#error "Only <bismuth.h> can be included directly."
#endif

#include "bis-version.h"

#include <gtk/gtk.h>
#include "bis-enums.h"
#include "bis-fold-threshold-policy.h"

G_BEGIN_DECLS

#define BIS_TYPE_HUGGER_PAGE (bis_hugger_page_get_type ())

BIS_AVAILABLE_IN_ALL
G_DECLARE_FINAL_TYPE (BisHuggerPage, bis_hugger_page, BIS, HUGGER_PAGE, GObject)

BIS_AVAILABLE_IN_ALL
GtkWidget *bis_hugger_page_get_child (BisHuggerPage *self);

BIS_AVAILABLE_IN_ALL
gboolean bis_hugger_page_get_enabled (BisHuggerPage *self);
BIS_AVAILABLE_IN_ALL
void     bis_hugger_page_set_enabled (BisHuggerPage *self,
                                        gboolean         enabled);

#define BIS_TYPE_HUGGER (bis_hugger_get_type ())

BIS_AVAILABLE_IN_ALL
G_DECLARE_FINAL_TYPE (BisHugger, bis_hugger, BIS, HUGGER, GtkWidget)

typedef enum {
  BIS_HUGGER_TRANSITION_TYPE_NONE,
  BIS_HUGGER_TRANSITION_TYPE_CROSSFADE,
} BisHuggerTransitionType;

BIS_AVAILABLE_IN_ALL
GtkWidget *bis_hugger_new (void) G_GNUC_WARN_UNUSED_RESULT;

BIS_AVAILABLE_IN_ALL
BisHuggerPage *bis_hugger_add    (BisHugger *self,
                                      GtkWidget   *child);
BIS_AVAILABLE_IN_ALL
void             bis_hugger_remove (BisHugger *self,
                                      GtkWidget   *child);

BIS_AVAILABLE_IN_ALL
BisHuggerPage *bis_hugger_get_page (BisHugger *self,
                                        GtkWidget   *child);

BIS_AVAILABLE_IN_ALL
GtkWidget *bis_hugger_get_visible_child (BisHugger *self);

BIS_AVAILABLE_IN_ALL
gboolean bis_hugger_get_homogeneous (BisHugger *self);
BIS_AVAILABLE_IN_ALL
void     bis_hugger_set_homogeneous (BisHugger *self,
                                       gboolean     homogeneous);

BIS_AVAILABLE_IN_ALL
BisFoldThresholdPolicy bis_hugger_get_switch_threshold_policy (BisHugger             *self);
BIS_AVAILABLE_IN_ALL
void                   bis_hugger_set_switch_threshold_policy (BisHugger            *self,
                                                                 BisFoldThresholdPolicy  policy);

BIS_AVAILABLE_IN_ALL
gboolean bis_hugger_get_allow_none (BisHugger *self);
BIS_AVAILABLE_IN_ALL
void     bis_hugger_set_allow_none (BisHugger *self,
                                      gboolean     allow_none);

BIS_AVAILABLE_IN_ALL
guint bis_hugger_get_transition_duration (BisHugger *self);
BIS_AVAILABLE_IN_ALL
void  bis_hugger_set_transition_duration (BisHugger *self,
                                            guint        duration);

BIS_AVAILABLE_IN_ALL
BisHuggerTransitionType bis_hugger_get_transition_type (BisHugger               *self);
BIS_AVAILABLE_IN_ALL
void                      bis_hugger_set_transition_type (BisHugger               *self,
                                                            BisHuggerTransitionType  transition);

BIS_AVAILABLE_IN_ALL
gboolean bis_hugger_get_transition_running (BisHugger *self);

BIS_AVAILABLE_IN_ALL
gboolean bis_hugger_get_interpolate_size (BisHugger *self);
BIS_AVAILABLE_IN_ALL
void     bis_hugger_set_interpolate_size (BisHugger *self,
                                            gboolean     interpolate_size);

BIS_AVAILABLE_IN_ALL
float bis_hugger_get_xalign (BisHugger *self);
BIS_AVAILABLE_IN_ALL
void  bis_hugger_set_xalign (BisHugger *self,
                               float        xalign);

BIS_AVAILABLE_IN_ALL
float bis_hugger_get_yalign (BisHugger *self);
BIS_AVAILABLE_IN_ALL
void  bis_hugger_set_yalign (BisHugger *self,
                               float        yalign);

BIS_AVAILABLE_IN_ALL
GtkSelectionModel *bis_hugger_get_pages (BisHugger *self) G_GNUC_WARN_UNUSED_RESULT;

G_END_DECLS
