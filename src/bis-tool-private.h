/*
 * Copyright (C) 2020 Purism SPC
 *
 * Based on gtktoolprivate.h
 * https://gitlab.gnome.org/GNOME/gtk/-/blob/5d5625dec839c00fdb572af82fbbe872ea684859/gtk/gtktoolprivate.h
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once

#if !defined(_BISMUTH_INSIDE) && !defined(BISMUTH_COMPILATION)
#error "Only <bismuth.h> can be included directly."
#endif

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define BIS_TYPE_TOOL (bis_tool_get_type())

G_DECLARE_FINAL_TYPE (BisTool, bis_tool, BIS, TOOL, GtkWidget)

typedef void     (* BisToolMeasureFunc)  (BisTool       *self,
                                           GtkOrientation  orientation,
                                           int             for_size,
                                           int            *minimum,
                                           int            *natural,
                                           int            *minimum_baseline,
                                           int            *natural_baseline);
typedef void     (* BisToolAllocateFunc) (BisTool *self,
                                           int       width,
                                           int       height,
                                           int       baseline);
typedef void     (* BisToolSnapshotFunc) (BisTool    *self,
                                           GtkSnapshot *snapshot);
typedef gboolean (* BisToolContainsFunc) (BisTool *self,
                                           double    x,
                                           double    y);
typedef gboolean (* BisToolFocusFunc)    (BisTool         *self,
                                           GtkDirectionType  direction);
typedef gboolean (* BisToolGrabFocusFunc)(BisTool         *self);

GtkWidget *bis_tool_new (const char            *css_name,
                          BisToolMeasureFunc    measure_func,
                          BisToolAllocateFunc   allocate_func,
                          BisToolSnapshotFunc   snapshot_func,
                          BisToolContainsFunc   contains_func,
                          BisToolFocusFunc      focus_func,
                          BisToolGrabFocusFunc  grab_focus_func) G_GNUC_WARN_UNUSED_RESULT;

GtkWidget *bis_tool_new_with_role (const char            *css_name,
                                    GtkAccessibleRole      role,
                                    BisToolMeasureFunc    measure_func,
                                    BisToolAllocateFunc   allocate_func,
                                    BisToolSnapshotFunc   snapshot_func,
                                    BisToolContainsFunc   contains_func,
                                    BisToolFocusFunc      focus_func,
                                    BisToolGrabFocusFunc  grab_focus_func) G_GNUC_WARN_UNUSED_RESULT;

void bis_tool_set_measure_func    (BisTool              *self,
                                    BisToolMeasureFunc    measure_func);
void bis_tool_set_allocate_func   (BisTool              *self,
                                    BisToolAllocateFunc   allocate_func);
void bis_tool_set_snapshot_func   (BisTool              *self,
                                    BisToolSnapshotFunc   snapshot_func);
void bis_tool_set_contains_func   (BisTool              *self,
                                    BisToolContainsFunc   contains_func);
void bis_tool_set_focus_func      (BisTool              *self,
                                    BisToolFocusFunc      focus_func);
void bis_tool_set_grab_focus_func (BisTool              *self,
                                    BisToolGrabFocusFunc  grab_focus_func);

G_END_DECLS
