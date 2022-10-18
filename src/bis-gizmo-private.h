/*
 * Copyright (C) 2020 Purism SPC
 *
 * Based on gtkgizmoprivate.h
 * https://gitlab.gnome.org/GNOME/gtk/-/blob/5d5625dec839c00fdb572af82fbbe872ea684859/gtk/gtkgizmoprivate.h
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once

#if !defined(_BISMUTH_INSIDE) && !defined(BISMUTH_COMPILATION)
#error "Only <bismuth.h> can be included directly."
#endif

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define BIS_TYPE_GIZMO (bis_gizmo_get_type())

G_DECLARE_FINAL_TYPE (BisGizmo, bis_gizmo, BIS, GIZMO, GtkWidget)

typedef void     (* BisGizmoMeasureFunc)  (BisGizmo       *self,
                                           GtkOrientation  orientation,
                                           int             for_size,
                                           int            *minimum,
                                           int            *natural,
                                           int            *minimum_baseline,
                                           int            *natural_baseline);
typedef void     (* BisGizmoAllocateFunc) (BisGizmo *self,
                                           int       width,
                                           int       height,
                                           int       baseline);
typedef void     (* BisGizmoSnapshotFunc) (BisGizmo    *self,
                                           GtkSnapshot *snapshot);
typedef gboolean (* BisGizmoContainsFunc) (BisGizmo *self,
                                           double    x,
                                           double    y);
typedef gboolean (* BisGizmoFocusFunc)    (BisGizmo         *self,
                                           GtkDirectionType  direction);
typedef gboolean (* BisGizmoGrabFocusFunc)(BisGizmo         *self);

GtkWidget *bis_gizmo_new (const char            *css_name,
                          BisGizmoMeasureFunc    measure_func,
                          BisGizmoAllocateFunc   allocate_func,
                          BisGizmoSnapshotFunc   snapshot_func,
                          BisGizmoContainsFunc   contains_func,
                          BisGizmoFocusFunc      focus_func,
                          BisGizmoGrabFocusFunc  grab_focus_func) G_GNUC_WARN_UNUSED_RESULT;

GtkWidget *bis_gizmo_new_with_role (const char            *css_name,
                                    GtkAccessibleRole      role,
                                    BisGizmoMeasureFunc    measure_func,
                                    BisGizmoAllocateFunc   allocate_func,
                                    BisGizmoSnapshotFunc   snapshot_func,
                                    BisGizmoContainsFunc   contains_func,
                                    BisGizmoFocusFunc      focus_func,
                                    BisGizmoGrabFocusFunc  grab_focus_func) G_GNUC_WARN_UNUSED_RESULT;

void bis_gizmo_set_measure_func    (BisGizmo              *self,
                                    BisGizmoMeasureFunc    measure_func);
void bis_gizmo_set_allocate_func   (BisGizmo              *self,
                                    BisGizmoAllocateFunc   allocate_func);
void bis_gizmo_set_snapshot_func   (BisGizmo              *self,
                                    BisGizmoSnapshotFunc   snapshot_func);
void bis_gizmo_set_contains_func   (BisGizmo              *self,
                                    BisGizmoContainsFunc   contains_func);
void bis_gizmo_set_focus_func      (BisGizmo              *self,
                                    BisGizmoFocusFunc      focus_func);
void bis_gizmo_set_grab_focus_func (BisGizmo              *self,
                                    BisGizmoGrabFocusFunc  grab_focus_func);

G_END_DECLS
