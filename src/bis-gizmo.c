/*
 * Copyright (C) 2020 Purism SPC
 *
 * Based on gtkgizmo.c
 * https://gitlab.gnome.org/GNOME/gtk/-/blob/5d5625dec839c00fdb572af82fbbe872ea684859/gtk/gtkgizmo.c
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "bis-gizmo-private.h"

#include "bis-macros-private.h"
#include "bis-widget-utils-private.h"

struct _BisGizmo
{
  GtkWidget parent_instance;

  BisGizmoMeasureFunc   measure_func;
  BisGizmoAllocateFunc  allocate_func;
  BisGizmoSnapshotFunc  snapshot_func;
  BisGizmoContainsFunc  contains_func;
  BisGizmoFocusFunc     focus_func;
  BisGizmoGrabFocusFunc grab_focus_func;
};

G_DEFINE_FINAL_TYPE (BisGizmo, bis_gizmo, GTK_TYPE_WIDGET)

static void
bis_gizmo_measure (GtkWidget      *widget,
                   GtkOrientation  orientation,
                   int             for_size,
                   int            *minimum,
                   int            *natural,
                   int            *minimum_baseline,
                   int            *natural_baseline)
{
  BisGizmo *self = BIS_GIZMO (widget);

  if (self->measure_func)
    self->measure_func (self, orientation, for_size,
                        minimum, natural,
                        minimum_baseline, natural_baseline);
}

static void
bis_gizmo_size_allocate (GtkWidget *widget,
                         int        width,
                         int        height,
                         int        baseline)
{
  BisGizmo *self = BIS_GIZMO (widget);

  if (self->allocate_func)
    self->allocate_func (self, width, height, baseline);
}

static void
bis_gizmo_snapshot (GtkWidget   *widget,
                    GtkSnapshot *snapshot)
{
  BisGizmo *self = BIS_GIZMO (widget);

  if (self->snapshot_func)
    self->snapshot_func (self, snapshot);
  else
    GTK_WIDGET_CLASS (bis_gizmo_parent_class)->snapshot (widget, snapshot);
}

static gboolean
bis_gizmo_contains (GtkWidget *widget,
                    double     x,
                    double     y)
{
  BisGizmo *self = BIS_GIZMO (widget);

  if (self->contains_func)
    return self->contains_func (self, x, y);
  else
    return GTK_WIDGET_CLASS (bis_gizmo_parent_class)->contains (widget, x, y);
}

static gboolean
bis_gizmo_focus (GtkWidget        *widget,
                 GtkDirectionType  direction)
{
  BisGizmo *self = BIS_GIZMO (widget);

  if (self->focus_func)
    return self->focus_func (self, direction);

  return FALSE;
}

static gboolean
bis_gizmo_grab_focus (GtkWidget *widget)
{
  BisGizmo *self = BIS_GIZMO (widget);

  if (self->grab_focus_func)
    return self->grab_focus_func (self);

  return FALSE;
}

static void
bis_gizmo_dispose (GObject *object)
{
  BisGizmo *self = BIS_GIZMO (object);
  GtkWidget *widget = gtk_widget_get_first_child (GTK_WIDGET (self));

  while (widget) {
    GtkWidget *next = gtk_widget_get_next_sibling (widget);

    gtk_widget_unparent (widget);

    widget = next;
  }

  G_OBJECT_CLASS (bis_gizmo_parent_class)->dispose (object);
}

static void
bis_gizmo_class_init (BisGizmoClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = bis_gizmo_dispose;

  widget_class->measure = bis_gizmo_measure;
  widget_class->size_allocate = bis_gizmo_size_allocate;
  widget_class->snapshot = bis_gizmo_snapshot;
  widget_class->contains = bis_gizmo_contains;
  widget_class->grab_focus = bis_gizmo_grab_focus;
  widget_class->focus = bis_gizmo_focus;
  widget_class->compute_expand = bis_widget_compute_expand;
}

static void
bis_gizmo_init (BisGizmo *self)
{
}

GtkWidget *
bis_gizmo_new (const char            *css_name,
               BisGizmoMeasureFunc    measure_func,
               BisGizmoAllocateFunc   allocate_func,
               BisGizmoSnapshotFunc   snapshot_func,
               BisGizmoContainsFunc   contains_func,
               BisGizmoFocusFunc      focus_func,
               BisGizmoGrabFocusFunc  grab_focus_func)
{
  BisGizmo *gizmo = g_object_new (BIS_TYPE_GIZMO,
                                  "css-name", css_name,
                                  NULL);

  gizmo->measure_func  = measure_func;
  gizmo->allocate_func = allocate_func;
  gizmo->snapshot_func = snapshot_func;
  gizmo->contains_func = contains_func;
  gizmo->focus_func = focus_func;
  gizmo->grab_focus_func = grab_focus_func;

  return GTK_WIDGET (gizmo);
}

GtkWidget *
bis_gizmo_new_with_role (const char            *css_name,
                         GtkAccessibleRole      role,
                         BisGizmoMeasureFunc    measure_func,
                         BisGizmoAllocateFunc   allocate_func,
                         BisGizmoSnapshotFunc   snapshot_func,
                         BisGizmoContainsFunc   contains_func,
                         BisGizmoFocusFunc      focus_func,
                         BisGizmoGrabFocusFunc  grab_focus_func)
{
  BisGizmo *gizmo = BIS_GIZMO (g_object_new (BIS_TYPE_GIZMO,
                                             "css-name", css_name,
                                             "accessible-role", role,
                                             NULL));

  gizmo->measure_func  = measure_func;
  gizmo->allocate_func = allocate_func;
  gizmo->snapshot_func = snapshot_func;
  gizmo->contains_func = contains_func;
  gizmo->focus_func = focus_func;
  gizmo->grab_focus_func = grab_focus_func;

  return GTK_WIDGET (gizmo);
}

void
bis_gizmo_set_measure_func (BisGizmo            *self,
                            BisGizmoMeasureFunc  measure_func)
{
  self->measure_func = measure_func;

  gtk_widget_queue_resize (GTK_WIDGET (self));
}

void
bis_gizmo_set_allocate_func (BisGizmo             *self,
                             BisGizmoAllocateFunc  allocate_func)
{
  self->allocate_func = allocate_func;

  gtk_widget_queue_allocate (GTK_WIDGET (self));
}

void
bis_gizmo_set_snapshot_func (BisGizmo             *self,
                             BisGizmoSnapshotFunc  snapshot_func)
{
  self->snapshot_func = snapshot_func;

  gtk_widget_queue_draw (GTK_WIDGET (self));
}

void
bis_gizmo_set_contains_func (BisGizmo             *self,
                             BisGizmoContainsFunc  contains_func)
{
  self->contains_func = contains_func;

  gtk_widget_queue_resize (GTK_WIDGET (self));
}

void
bis_gizmo_set_focus_func (BisGizmo          *self,
                          BisGizmoFocusFunc  focus_func)
{
  self->focus_func = focus_func;
}

void
bis_gizmo_set_grab_focus_func (BisGizmo              *self,
                               BisGizmoGrabFocusFunc  grab_focus_func)
{
  self->grab_focus_func = grab_focus_func;
}
