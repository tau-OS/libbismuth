/*
 * Copyright (C) 2020 Purism SPC
 *
 * Based on gtktool.c
 * https://gitlab.gnome.org/GNOME/gtk/-/blob/5d5625dec839c00fdb572af82fbbe872ea684859/gtk/gtktool.c
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "bis-tool-private.h"

#include "bis-macros-private.h"
#include "bis-widget-utils-private.h"

struct _BisTool
{
  GtkWidget parent_instance;

  BisToolMeasureFunc   measure_func;
  BisToolAllocateFunc  allocate_func;
  BisToolSnapshotFunc  snapshot_func;
  BisToolContainsFunc  contains_func;
  BisToolFocusFunc     focus_func;
  BisToolGrabFocusFunc grab_focus_func;
};

G_DEFINE_FINAL_TYPE (BisTool, bis_tool, GTK_TYPE_WIDGET)

static void
bis_tool_measure (GtkWidget      *widget,
                   GtkOrientation  orientation,
                   int             for_size,
                   int            *minimum,
                   int            *natural,
                   int            *minimum_baseline,
                   int            *natural_baseline)
{
  BisTool *self = BIS_TOOL (widget);

  if (self->measure_func)
    self->measure_func (self, orientation, for_size,
                        minimum, natural,
                        minimum_baseline, natural_baseline);
}

static void
bis_tool_size_allocate (GtkWidget *widget,
                         int        width,
                         int        height,
                         int        baseline)
{
  BisTool *self = BIS_TOOL (widget);

  if (self->allocate_func)
    self->allocate_func (self, width, height, baseline);
}

static void
bis_tool_snapshot (GtkWidget   *widget,
                    GtkSnapshot *snapshot)
{
  BisTool *self = BIS_TOOL (widget);

  if (self->snapshot_func)
    self->snapshot_func (self, snapshot);
  else
    GTK_WIDGET_CLASS (bis_tool_parent_class)->snapshot (widget, snapshot);
}

static gboolean
bis_tool_contains (GtkWidget *widget,
                    double     x,
                    double     y)
{
  BisTool *self = BIS_TOOL (widget);

  if (self->contains_func)
    return self->contains_func (self, x, y);
  else
    return GTK_WIDGET_CLASS (bis_tool_parent_class)->contains (widget, x, y);
}

static gboolean
bis_tool_focus (GtkWidget        *widget,
                 GtkDirectionType  direction)
{
  BisTool *self = BIS_TOOL (widget);

  if (self->focus_func)
    return self->focus_func (self, direction);

  return FALSE;
}

static gboolean
bis_tool_grab_focus (GtkWidget *widget)
{
  BisTool *self = BIS_TOOL (widget);

  if (self->grab_focus_func)
    return self->grab_focus_func (self);

  return FALSE;
}

static void
bis_tool_dispose (GObject *object)
{
  BisTool *self = BIS_TOOL (object);
  GtkWidget *widget = gtk_widget_get_first_child (GTK_WIDGET (self));

  while (widget) {
    GtkWidget *next = gtk_widget_get_next_sibling (widget);

    gtk_widget_unparent (widget);

    widget = next;
  }

  G_OBJECT_CLASS (bis_tool_parent_class)->dispose (object);
}

static void
bis_tool_class_init (BisToolClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = bis_tool_dispose;

  widget_class->measure = bis_tool_measure;
  widget_class->size_allocate = bis_tool_size_allocate;
  widget_class->snapshot = bis_tool_snapshot;
  widget_class->contains = bis_tool_contains;
  widget_class->grab_focus = bis_tool_grab_focus;
  widget_class->focus = bis_tool_focus;
  widget_class->compute_expand = bis_widget_compute_expand;
}

static void
bis_tool_init (BisTool *self)
{
}

GtkWidget *
bis_tool_new (const char            *css_name,
               BisToolMeasureFunc    measure_func,
               BisToolAllocateFunc   allocate_func,
               BisToolSnapshotFunc   snapshot_func,
               BisToolContainsFunc   contains_func,
               BisToolFocusFunc      focus_func,
               BisToolGrabFocusFunc  grab_focus_func)
{
  BisTool *tool = g_object_new (BIS_TYPE_TOOL,
                                  "css-name", css_name,
                                  NULL);

  tool->measure_func  = measure_func;
  tool->allocate_func = allocate_func;
  tool->snapshot_func = snapshot_func;
  tool->contains_func = contains_func;
  tool->focus_func = focus_func;
  tool->grab_focus_func = grab_focus_func;

  return GTK_WIDGET (tool);
}

GtkWidget *
bis_tool_new_with_role (const char            *css_name,
                         GtkAccessibleRole      role,
                         BisToolMeasureFunc    measure_func,
                         BisToolAllocateFunc   allocate_func,
                         BisToolSnapshotFunc   snapshot_func,
                         BisToolContainsFunc   contains_func,
                         BisToolFocusFunc      focus_func,
                         BisToolGrabFocusFunc  grab_focus_func)
{
  BisTool *tool = BIS_TOOL (g_object_new (BIS_TYPE_TOOL,
                                             "css-name", css_name,
                                             "accessible-role", role,
                                             NULL));

  tool->measure_func  = measure_func;
  tool->allocate_func = allocate_func;
  tool->snapshot_func = snapshot_func;
  tool->contains_func = contains_func;
  tool->focus_func = focus_func;
  tool->grab_focus_func = grab_focus_func;

  return GTK_WIDGET (tool);
}

void
bis_tool_set_measure_func (BisTool            *self,
                            BisToolMeasureFunc  measure_func)
{
  self->measure_func = measure_func;

  gtk_widget_queue_resize (GTK_WIDGET (self));
}

void
bis_tool_set_allocate_func (BisTool             *self,
                             BisToolAllocateFunc  allocate_func)
{
  self->allocate_func = allocate_func;

  gtk_widget_queue_allocate (GTK_WIDGET (self));
}

void
bis_tool_set_snapshot_func (BisTool             *self,
                             BisToolSnapshotFunc  snapshot_func)
{
  self->snapshot_func = snapshot_func;

  gtk_widget_queue_draw (GTK_WIDGET (self));
}

void
bis_tool_set_contains_func (BisTool             *self,
                             BisToolContainsFunc  contains_func)
{
  self->contains_func = contains_func;

  gtk_widget_queue_resize (GTK_WIDGET (self));
}

void
bis_tool_set_focus_func (BisTool          *self,
                          BisToolFocusFunc  focus_func)
{
  self->focus_func = focus_func;
}

void
bis_tool_set_grab_focus_func (BisTool              *self,
                               BisToolGrabFocusFunc  grab_focus_func)
{
  self->grab_focus_func = grab_focus_func;
}
