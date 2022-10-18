/*
 * Copyright (C) 2013 Red Hat, Inc.
 * Copyright (C) 2019 Purism SPC
 *
 * Author: Alexander Larsson <alexl@redhat.com>
 * Author: Adrien Plazas <adrien.plazas@puri.sm>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

/*
 * Forked from the GTK+ 3.24.2 GtkStack widget initially written by Alexander
 * Larsson, and heavily modified for libbismuth by Adrien Plazas on behalf of
 * Purism SPC 2019.
 */

#include "config.h"

#include "bis-squeezer.h"

#include "bis-animation-util.h"
#include "bis-easing.h"
#include "bis-macros-private.h"
#include "bis-timed-animation.h"
#include "bis-widget-utils-private.h"

/**
 * BisSqueezer:
 *
 * A best fit container.
 *
 * <picture>
 *   <source srcset="squeezer-wide-dark.png" media="(prefers-color-scheme: dark)">
 *   <img src="squeezer-wide.png" alt="squeezer-wide">
 * </picture>
 * <picture>
 *   <source srcset="squeezer-narrow-dark.png" media="(prefers-color-scheme: dark)">
 *   <img src="squeezer-narrow.png" alt="squeezer-narrow">
 * </picture>
 *
 * The `BisSqueezer` widget is a container which only shows the first of its
 * children that fits in the available size. It is convenient to offer different
 * widgets to represent the same data with different levels of detail, making
 * the widget seem to squeeze itself to fit in the available space.
 *
 * Transitions between children can be animated as fades. This can be controlled
 * with [property@Squeezer:transition-type].
 *
 * ## CSS nodes
 *
 * `BisSqueezer` has a single CSS node with name `squeezer`.
 *
 * Since: 1.0
 */

/**
 * BisSqueezerPage:
 *
 * An auxiliary class used by [class@Squeezer].
 */

/**
 * BisSqueezerTransitionType:
 * @BIS_SQUEEZER_TRANSITION_TYPE_NONE: No transition
 * @BIS_SQUEEZER_TRANSITION_TYPE_CROSSFADE: A cross-fade
 *
 * Describes the possible transitions in a [class@Squeezer] widget.
 *
 * Since: 1.0
 */

struct _BisSqueezerPage {
  GObject parent_instance;

  GtkWidget *widget;
  GtkWidget *last_focus;
  gboolean enabled;
};

G_DEFINE_FINAL_TYPE (BisSqueezerPage, bis_squeezer_page, G_TYPE_OBJECT)

enum {
  PAGE_PROP_0,
  PAGE_PROP_CHILD,
  PAGE_PROP_ENABLED,
  LAST_PAGE_PROP
};

static GParamSpec *page_props[LAST_PAGE_PROP];

struct _BisSqueezer
{
  GtkWidget parent_instance;

  GList *children;

  BisSqueezerPage *visible_child;
  BisFoldThresholdPolicy switch_threshold_policy;

  gboolean homogeneous;

  gboolean allow_none;

  BisSqueezerTransitionType transition_type;
  guint transition_duration;

  BisSqueezerPage *last_visible_child;
  gboolean transition_running;
  BisAnimation *animation;

  int last_visible_widget_width;
  int last_visible_widget_height;

  gboolean interpolate_size;

  float xalign;
  float yalign;

  GtkOrientation orientation;

  GtkSelectionModel *pages;
};

enum  {
  PROP_0,
  PROP_VISIBLE_CHILD,
  PROP_HOMOGENEOUS,
  PROP_SWITCH_THRESHOLD_POLICY,
  PROP_ALLOW_NONE,
  PROP_TRANSITION_DURATION,
  PROP_TRANSITION_TYPE,
  PROP_TRANSITION_RUNNING,
  PROP_INTERPOLATE_SIZE,
  PROP_XALIGN,
  PROP_YALIGN,
  PROP_PAGES,

  /* Overridden properties */
  PROP_ORIENTATION,

  LAST_PROP = PROP_PAGES + 1,
};

static GParamSpec *props[LAST_PROP];

static void bis_squeezer_buildable_init (GtkBuildableIface *iface);

G_DEFINE_FINAL_TYPE_WITH_CODE (BisSqueezer, bis_squeezer, GTK_TYPE_WIDGET,
                               G_IMPLEMENT_INTERFACE (GTK_TYPE_ORIENTABLE, NULL)
                               G_IMPLEMENT_INTERFACE (GTK_TYPE_BUILDABLE, bis_squeezer_buildable_init))

static GtkBuildableIface *parent_buildable_iface;

static void
bis_squeezer_page_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  BisSqueezerPage *self = BIS_SQUEEZER_PAGE (object);

  switch (property_id) {
  case PAGE_PROP_CHILD:
    g_value_set_object (value, bis_squeezer_page_get_child (self));
    break;
  case PAGE_PROP_ENABLED:
    g_value_set_boolean (value, bis_squeezer_page_get_enabled (self));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

static void
bis_squeezer_page_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  BisSqueezerPage *self = BIS_SQUEEZER_PAGE (object);

  switch (property_id) {
  case PAGE_PROP_CHILD:
    g_set_object (&self->widget, g_value_get_object (value));
    break;
  case PAGE_PROP_ENABLED:
    bis_squeezer_page_set_enabled (self, g_value_get_boolean (value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

static void
bis_squeezer_page_finalize (GObject *object)
{
  BisSqueezerPage *self = BIS_SQUEEZER_PAGE (object);

  g_clear_object (&self->widget);

  if (self->last_focus)
    g_object_remove_weak_pointer (G_OBJECT (self->last_focus),
                                  (gpointer *) &self->last_focus);

  G_OBJECT_CLASS (bis_squeezer_page_parent_class)->finalize (object);
}

static void
bis_squeezer_page_class_init (BisSqueezerPageClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = bis_squeezer_page_get_property;
  object_class->set_property = bis_squeezer_page_set_property;
  object_class->finalize = bis_squeezer_page_finalize;

  /**
   * BisSqueezerPage:child: (attributes org.gtk.Property.get=bis_squeezer_page_get_child)
   *
   * The the squeezer child to which the page belongs.
   *
   * Since: 1.0
   */
  page_props[PAGE_PROP_CHILD] =
    g_param_spec_object ("child", NULL, NULL,
                         GTK_TYPE_WIDGET,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  /**
   * BisSqueezerPage:enabled: (attributes org.gtk.Property.get=bis_squeezer_page_get_enabled org.gtk.Property.set=bis_squeezer_page_set_enabled)
   *
   * Whether the child is enabled.
   *
   * If a child is disabled, it will be ignored when looking for the child
   * fitting the available size best.
   *
   * This allows to programmatically and prematurely hide a child even if it
   * fits in the available space.
   *
   * This can be used e.g. to ensure a certain child is hidden below a certain
   * window width, or any other constraint you find suitable.
   *
   * Since: 1.0
   */
  page_props[PAGE_PROP_ENABLED] =
    g_param_spec_boolean ("enabled", NULL, NULL,
                          TRUE,
                          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, LAST_PAGE_PROP, page_props);
}

static void
bis_squeezer_page_init (BisSqueezerPage *self)
{
  self->enabled = TRUE;
}

#define BIS_TYPE_SQUEEZER_PAGES (bis_squeezer_pages_get_type ())

G_DECLARE_FINAL_TYPE (BisSqueezerPages, bis_squeezer_pages, BIS, SQUEEZER_PAGES, GObject)

struct _BisSqueezerPages
{
  GObject parent_instance;
  BisSqueezer *squeezer;
};

static GType
bis_squeezer_pages_get_item_type (GListModel *model)
{
  return BIS_TYPE_SQUEEZER_PAGE;
}

static guint
bis_squeezer_pages_get_n_items (GListModel *model)
{
  BisSqueezerPages *self = BIS_SQUEEZER_PAGES (model);

  return g_list_length (self->squeezer->children);
}

static gpointer
bis_squeezer_pages_get_item (GListModel *model,
                             guint       position)
{
  BisSqueezerPages *self = BIS_SQUEEZER_PAGES (model);
  BisSqueezerPage *page;

  page = g_list_nth_data (self->squeezer->children, position);

  if (!page)
    return NULL;

  return g_object_ref (page);
}

static void
bis_squeezer_pages_list_model_init (GListModelInterface *iface)
{
  iface->get_item_type = bis_squeezer_pages_get_item_type;
  iface->get_n_items = bis_squeezer_pages_get_n_items;
  iface->get_item = bis_squeezer_pages_get_item;
}

static gboolean
bis_squeezer_pages_is_selected (GtkSelectionModel *model,
                                guint              position)
{
  BisSqueezerPages *self = BIS_SQUEEZER_PAGES (model);
  BisSqueezerPage *page;

  page = g_list_nth_data (self->squeezer->children, position);

  return page && page == self->squeezer->visible_child;
}

static void
bis_squeezer_pages_selection_model_init (GtkSelectionModelInterface *iface)
{
  iface->is_selected = bis_squeezer_pages_is_selected;
}

G_DEFINE_FINAL_TYPE_WITH_CODE (BisSqueezerPages, bis_squeezer_pages, G_TYPE_OBJECT,
                               G_IMPLEMENT_INTERFACE (G_TYPE_LIST_MODEL, bis_squeezer_pages_list_model_init)
                               G_IMPLEMENT_INTERFACE (GTK_TYPE_SELECTION_MODEL, bis_squeezer_pages_selection_model_init))

static void
bis_squeezer_pages_init (BisSqueezerPages *pages)
{
}

static void
bis_squeezer_pages_class_init (BisSqueezerPagesClass *klass)
{
}

static BisSqueezerPages *
bis_squeezer_pages_new (BisSqueezer *squeezer)
{
  BisSqueezerPages *pages;

  pages = g_object_new (BIS_TYPE_SQUEEZER_PAGES, NULL);
  pages->squeezer = squeezer;

  return pages;
}

static GtkOrientation
get_orientation (BisSqueezer *self)
{
  return self->orientation;
}

static void
set_orientation (BisSqueezer    *self,
                 GtkOrientation  orientation)
{
  if (self->orientation == orientation)
    return;

  self->orientation = orientation;
  gtk_widget_queue_resize (GTK_WIDGET (self));
  g_object_notify (G_OBJECT (self), "orientation");
}

static BisSqueezerPage *
find_page_for_widget (BisSqueezer *self,
                      GtkWidget   *child)
{
  BisSqueezerPage *page;
  GList *l;

  for (l = self->children; l != NULL; l = l->next) {
    page = l->data;
    if (page->widget == child)
      return page;
  }

  return NULL;
}

static void
transition_cb (double       value,
               BisSqueezer *self)
{
  if (!self->homogeneous)
    gtk_widget_queue_resize (GTK_WIDGET (self));
  else
    gtk_widget_queue_draw (GTK_WIDGET (self));
}

static void
set_transition_running (BisSqueezer *self,
                        gboolean     running)
{
  if (self->transition_running == running)
    return;

  self->transition_running = running;
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_TRANSITION_RUNNING]);
}

static void
transition_done_cb (BisSqueezer *self)
{
  if (self->last_visible_child) {
    gtk_widget_set_child_visible (self->last_visible_child->widget, FALSE);
    self->last_visible_child = NULL;
  }

  bis_animation_reset (self->animation);

  set_transition_running (self, FALSE);
}

static void
set_visible_child (BisSqueezer               *self,
                   BisSqueezerPage           *page,
                   BisSqueezerTransitionType  transition_type,
                   guint                      transition_duration)
{
  GtkWidget *widget = GTK_WIDGET (self);
  GtkRoot *root;
  GtkWidget *focus;
  gboolean contains_focus = FALSE;
  guint old_pos = GTK_INVALID_LIST_POSITION;
  guint new_pos = GTK_INVALID_LIST_POSITION;

  /* If we are being destroyed, do not bother with transitions and
   * notifications.
   */
  if (gtk_widget_in_destruction (widget))
    return;

  /* If none, pick the first visible. */
  if (!page && !self->allow_none) {
    GList *l;

    for (l = self->children; l; l = l->next) {
      BisSqueezerPage *p = l->data;
      if (gtk_widget_get_visible (p->widget)) {
        page = p;
        break;
      }
    }
  }

  if (page == self->visible_child)
    return;

  if (page != NULL && self->pages) {
    guint position;
    GList *l;

    for (l = self->children, position = 0; l; l = l->next, position++) {
      BisSqueezerPage *p = l->data;
      if (p == self->visible_child)
        old_pos = position;
      else if (p == page)
        new_pos = position;
    }
  }

  root = gtk_widget_get_root (widget);
  if (root)
    focus = gtk_root_get_focus (root);
  else
    focus = NULL;

  if (focus &&
      self->visible_child &&
      self->visible_child->widget &&
      gtk_widget_is_ancestor (focus, self->visible_child->widget)) {
    contains_focus = TRUE;

    if (self->visible_child->last_focus)
      g_object_remove_weak_pointer (G_OBJECT (self->visible_child->last_focus),
                                    (gpointer *)&self->visible_child->last_focus);
    self->visible_child->last_focus = focus;
    g_object_add_weak_pointer (G_OBJECT (self->visible_child->last_focus),
                               (gpointer *)&self->visible_child->last_focus);
  }

  if (self->transition_running)
    bis_animation_skip (self->animation);

  if (self->visible_child && self->visible_child->widget) {
    if (gtk_widget_is_visible (widget)) {
      self->last_visible_child = self->visible_child;
      self->last_visible_widget_width = gtk_widget_get_width (self->last_visible_child->widget);
      self->last_visible_widget_height = gtk_widget_get_height (self->last_visible_child->widget);
    } else {
      gtk_widget_set_child_visible (self->visible_child->widget, FALSE);
    }
  }

  self->visible_child = page;

  if (page) {
    gtk_widget_set_child_visible (page->widget, TRUE);

    if (contains_focus) {
      if (page->last_focus)
        gtk_widget_grab_focus (page->last_focus);
      else
        gtk_widget_child_focus (page->widget, GTK_DIR_TAB_FORWARD);
    }
  }

  if (self->homogeneous)
    gtk_widget_queue_allocate (widget);
  else
    gtk_widget_queue_resize (widget);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_VISIBLE_CHILD]);

  if (self->pages) {
    if (old_pos == GTK_INVALID_LIST_POSITION && new_pos == GTK_INVALID_LIST_POSITION)
      ; /* nothing to do */
    else if (old_pos == GTK_INVALID_LIST_POSITION)
      gtk_selection_model_selection_changed (self->pages, new_pos, 1);
    else if (new_pos == GTK_INVALID_LIST_POSITION)
      gtk_selection_model_selection_changed (self->pages, old_pos, 1);
    else
      gtk_selection_model_selection_changed (self->pages,
                                             MIN (old_pos, new_pos),
                                             MAX (old_pos, new_pos) - MIN (old_pos, new_pos) + 1);
  }

  if (self->transition_type == BIS_SQUEEZER_TRANSITION_TYPE_NONE ||
      (self->last_visible_child == NULL && !self->allow_none))
    bis_timed_animation_set_duration (BIS_TIMED_ANIMATION (self->animation), 0);
  else
    bis_timed_animation_set_duration (BIS_TIMED_ANIMATION (self->animation),
                                      self->transition_duration);

  set_transition_running (self, TRUE);
  bis_animation_play (self->animation);
}

static void
update_child_visible (BisSqueezer     *self,
                      BisSqueezerPage *page)
{
  gboolean enabled;

  enabled = page->enabled && gtk_widget_get_visible (page->widget);

  if (self->visible_child == NULL && enabled)
    set_visible_child (self, page, self->transition_type, self->transition_duration);
  else if (self->visible_child == page && !enabled)
    set_visible_child (self, NULL, self->transition_type, self->transition_duration);

  if (page == self->last_visible_child) {
    gtk_widget_set_child_visible (self->last_visible_child->widget, FALSE);
    self->last_visible_child = NULL;
  }
}

static void
squeezer_child_visibility_notify_cb (GObject    *obj,
                                     GParamSpec *pspec,
                                     gpointer    user_data)
{
  BisSqueezer *self = BIS_SQUEEZER (user_data);
  GtkWidget *child = GTK_WIDGET (obj);
  BisSqueezerPage *page;

  page = find_page_for_widget (self, child);
  g_return_if_fail (page != NULL);

  update_child_visible (self, page);
}

static void
add_page (BisSqueezer     *self,
          BisSqueezerPage *page)
{
  g_return_if_fail (page->widget != NULL);

  self->children = g_list_append (self->children, g_object_ref (page));

  gtk_widget_set_child_visible (page->widget, FALSE);
  gtk_widget_set_parent (page->widget, GTK_WIDGET (self));

  if (self->pages)
    g_list_model_items_changed (G_LIST_MODEL (self->pages), g_list_length (self->children) - 1, 0, 1);

  g_signal_connect (page->widget, "notify::visible",
                    G_CALLBACK (squeezer_child_visibility_notify_cb), self);

  if (self->visible_child == NULL &&
      gtk_widget_get_visible (page->widget))
    set_visible_child (self, page, self->transition_type, self->transition_duration);

  if (self->homogeneous || self->visible_child == page)
    gtk_widget_queue_resize (GTK_WIDGET (self));
}

static void
squeezer_remove (BisSqueezer *self,
                 GtkWidget   *child,
                 gboolean     in_dispose)
{
  BisSqueezerPage *page;
  gboolean was_visible;

  page = find_page_for_widget (self, child);
  if (!page)
    return;

  self->children = g_list_remove (self->children, page);

  g_signal_handlers_disconnect_by_func (child,
                                        squeezer_child_visibility_notify_cb,
                                        self);

  was_visible = gtk_widget_get_visible (child);

  g_clear_object (&page->widget);

  if (self->visible_child == page)
    {
      if (in_dispose)
        self->visible_child = NULL;
      else
        set_visible_child (self, NULL, self->transition_type, self->transition_duration);
    }

  if (self->last_visible_child == page)
    self->last_visible_child = NULL;

  gtk_widget_unparent (child);

  g_object_unref (page);

  if (self->homogeneous && was_visible)
    gtk_widget_queue_resize (GTK_WIDGET (self));
}

static void
bis_squeezer_get_property (GObject    *object,
                           guint       property_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  BisSqueezer *self = BIS_SQUEEZER (object);

  switch (property_id) {
  case PROP_VISIBLE_CHILD:
    g_value_set_object (value, bis_squeezer_get_visible_child (self));
    break;
  case PROP_HOMOGENEOUS:
    g_value_set_boolean (value, bis_squeezer_get_homogeneous (self));
    break;
  case PROP_SWITCH_THRESHOLD_POLICY:
    g_value_set_enum (value, bis_squeezer_get_switch_threshold_policy (self));
    break;
  case PROP_ALLOW_NONE:
    g_value_set_boolean (value, bis_squeezer_get_allow_none (self));
    break;
  case PROP_TRANSITION_DURATION:
    g_value_set_uint (value, bis_squeezer_get_transition_duration (self));
    break;
  case PROP_TRANSITION_TYPE:
    g_value_set_enum (value, bis_squeezer_get_transition_type (self));
    break;
  case PROP_TRANSITION_RUNNING:
    g_value_set_boolean (value, bis_squeezer_get_transition_running (self));
    break;
  case PROP_INTERPOLATE_SIZE:
    g_value_set_boolean (value, bis_squeezer_get_interpolate_size (self));
    break;
  case PROP_XALIGN:
    g_value_set_float (value, bis_squeezer_get_xalign (self));
    break;
  case PROP_YALIGN:
    g_value_set_float (value, bis_squeezer_get_yalign (self));
    break;
  case PROP_ORIENTATION:
    g_value_set_enum (value, get_orientation (self));
    break;
  case PROP_PAGES:
    g_value_take_object (value, bis_squeezer_get_pages (self));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

static void
bis_squeezer_set_property (GObject      *object,
                           guint         property_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  BisSqueezer *self = BIS_SQUEEZER (object);

  switch (property_id) {
  case PROP_HOMOGENEOUS:
    bis_squeezer_set_homogeneous (self, g_value_get_boolean (value));
    break;
  case PROP_SWITCH_THRESHOLD_POLICY:
    bis_squeezer_set_switch_threshold_policy (self, g_value_get_enum (value));
    break;
  case PROP_ALLOW_NONE:
    bis_squeezer_set_allow_none (self, g_value_get_boolean (value));
    break;
  case PROP_TRANSITION_DURATION:
    bis_squeezer_set_transition_duration (self, g_value_get_uint (value));
    break;
  case PROP_TRANSITION_TYPE:
    bis_squeezer_set_transition_type (self, g_value_get_enum (value));
    break;
  case PROP_INTERPOLATE_SIZE:
    bis_squeezer_set_interpolate_size (self, g_value_get_boolean (value));
    break;
  case PROP_XALIGN:
    bis_squeezer_set_xalign (self, g_value_get_float (value));
    break;
  case PROP_YALIGN:
    bis_squeezer_set_yalign (self, g_value_get_float (value));
    break;
  case PROP_ORIENTATION:
    set_orientation (self, g_value_get_enum (value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

static void
bis_squeezer_snapshot_crossfade (GtkWidget   *widget,
                                 GtkSnapshot *snapshot)
{
  BisSqueezer *self = BIS_SQUEEZER (widget);
  double progress = bis_animation_get_value (self->animation);

  gtk_snapshot_push_cross_fade (snapshot, progress);

  if (self->last_visible_child)
    gtk_widget_snapshot_child (widget,
                               self->last_visible_child->widget,
                               snapshot);

  gtk_snapshot_pop (snapshot);

  if (self->visible_child)
    gtk_widget_snapshot_child (widget,
                               self->visible_child->widget,
                               snapshot);
  gtk_snapshot_pop (snapshot);
}


static void
bis_squeezer_snapshot (GtkWidget   *widget,
                       GtkSnapshot *snapshot)
{
  BisSqueezer *self = BIS_SQUEEZER (widget);

  if (self->visible_child || self->allow_none) {
    if (self->transition_running &&
        self->transition_type != BIS_SQUEEZER_TRANSITION_TYPE_NONE) {
      gtk_snapshot_push_clip (snapshot,
                              &GRAPHENE_RECT_INIT(
                                  0, 0,
                                  gtk_widget_get_width (widget),
                                  gtk_widget_get_height (widget)
                              ));

      switch (self->transition_type)
        {
        case BIS_SQUEEZER_TRANSITION_TYPE_CROSSFADE:
          bis_squeezer_snapshot_crossfade (widget, snapshot);
          break;
        case BIS_SQUEEZER_TRANSITION_TYPE_NONE:
        default:
          g_assert_not_reached ();
        }

      gtk_snapshot_pop (snapshot);
    } else if (self->visible_child) {
      gtk_widget_snapshot_child (widget,
                                 self->visible_child->widget,
                                 snapshot);
    }
  }
}

static void
bis_squeezer_size_allocate (GtkWidget *widget,
                            int        width,
                            int        height,
                            int        baseline)
{
  BisSqueezer *self = BIS_SQUEEZER (widget);
  BisSqueezerPage *page = NULL;
  GList *l;
  GtkAllocation child_allocation;

  for (l = self->children; l; l = l->next) {
    GtkWidget *child = NULL;
    int child_min, child_nat;
    int compare_size;

    page = l->data;
    child = page->widget;

    if (!gtk_widget_get_visible (child))
      continue;

    if (!page->enabled)
      continue;

    if (self->orientation == GTK_ORIENTATION_VERTICAL)
      compare_size = height;
    else
      compare_size = width;

    gtk_widget_measure (child, self->orientation, -1,
                        &child_min, &child_nat, NULL, NULL);

    if (child_min <= compare_size && self->switch_threshold_policy == BIS_FOLD_THRESHOLD_POLICY_MINIMUM)
      break;

    if (child_nat <= compare_size && self->switch_threshold_policy == BIS_FOLD_THRESHOLD_POLICY_NATURAL)
      break;
  }

  if (l == NULL && self->allow_none)
    page = NULL;

  set_visible_child (self, page,
                     self->transition_type,
                     self->transition_duration);

  child_allocation.x = 0;
  child_allocation.y = 0;

  if (self->last_visible_child) {
    int min;

    if (self->orientation == GTK_ORIENTATION_HORIZONTAL) {
      gtk_widget_measure (self->last_visible_child->widget, GTK_ORIENTATION_HORIZONTAL,
                          -1, &min, NULL, NULL, NULL);
      child_allocation.width = MAX (min, width);
      gtk_widget_measure (self->last_visible_child->widget, GTK_ORIENTATION_VERTICAL,
                          child_allocation.width, &min, NULL, NULL, NULL);
      child_allocation.height = MAX (min, height);
    } else {
      gtk_widget_measure (self->last_visible_child->widget, GTK_ORIENTATION_VERTICAL,
                          -1, &min, NULL, NULL, NULL);
      child_allocation.height = MAX (min, height);
      gtk_widget_measure (self->last_visible_child->widget, GTK_ORIENTATION_HORIZONTAL,
                          child_allocation.height, &min, NULL, NULL, NULL);
      child_allocation.width = MAX (min, width);
    }

    if (child_allocation.width > width) {
      if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL)
        child_allocation.x = (width - child_allocation.width) * (1 - self->xalign);
      else
        child_allocation.x = (width - child_allocation.width) * self->xalign;
    }

    if (child_allocation.height > height)
      child_allocation.y = (height - child_allocation.height) * self->yalign;

    gtk_widget_size_allocate (self->last_visible_child->widget, &child_allocation, -1);
  }

  child_allocation.width = width;
  child_allocation.height = height;
  child_allocation.x = 0;
  child_allocation.y = 0;

  if (self->visible_child) {
    int min;

    if (self->orientation == GTK_ORIENTATION_HORIZONTAL) {
      gtk_widget_measure (self->visible_child->widget, GTK_ORIENTATION_HORIZONTAL,
                          -1, &min, NULL, NULL, NULL);
      child_allocation.width = MAX (min, width);
      gtk_widget_measure (self->visible_child->widget, GTK_ORIENTATION_VERTICAL,
                          child_allocation.width, &min, NULL, NULL, NULL);
      child_allocation.height = MAX (min, height);
    } else {
      gtk_widget_measure (self->visible_child->widget, GTK_ORIENTATION_VERTICAL,
                          -1, &min, NULL, NULL, NULL);
      child_allocation.height = MAX (min, height);
      gtk_widget_measure (self->visible_child->widget, GTK_ORIENTATION_HORIZONTAL,
                          child_allocation.height, &min, NULL, NULL, NULL);
      child_allocation.width = MAX (min, width);
    }

    if (child_allocation.width > width) {
      if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL)
        child_allocation.x = (width - child_allocation.width) * (1 - self->xalign);
      else
        child_allocation.x = (width - child_allocation.width) * self->xalign;
    }

    if (child_allocation.height > height)
      child_allocation.y = (height - child_allocation.height) * self->yalign;

    gtk_widget_size_allocate (self->visible_child->widget, &child_allocation, -1);
  }
}

static void
bis_squeezer_measure (GtkWidget      *widget,
                      GtkOrientation  orientation,
                      int             for_size,
                      int            *minimum,
                      int            *natural,
                      int            *minimum_baseline,
                      int            *natural_baseline)
{
  BisSqueezer *self = BIS_SQUEEZER (widget);
  int child_min, child_nat;
  GList *l;
  int min = 0, nat = 0;

  for (l = self->children; l != NULL; l = l->next) {
    BisSqueezerPage *page = l->data;
    GtkWidget *child = page->widget;

    if (self->orientation != orientation && !self->homogeneous &&
        self->visible_child != page)
      continue;

    if (!gtk_widget_get_visible (child))
      continue;

    /* Disabled children are taken into account when measuring the widget, to
     * keep its size request and allocation consistent. This avoids the
     * appearant size and position of a child to changes suddenly when a larger
     * child gets enabled/disabled.
     */
    if (self->orientation == orientation)
      gtk_widget_measure (child, orientation, -1,
                          &child_min, &child_nat, NULL, NULL);
    else
      gtk_widget_measure (child, orientation, for_size,
                          &child_min, &child_nat, NULL, NULL);

    if (self->orientation == orientation) {
      if (self->allow_none)
        min = 0;
      else
        min = min == 0 ? child_min : MIN (min, child_min);
    } else {
      min = MAX (min, child_min);
    }

    nat = MAX (nat, child_nat);
  }

  if (self->orientation != orientation && !self->homogeneous &&
      self->interpolate_size &&
      (self->last_visible_child != NULL || self->allow_none)) {
    double t = bis_animation_get_value (self->animation);
    t = bis_easing_ease (BIS_EASE_OUT_CUBIC, t);

    if (orientation == GTK_ORIENTATION_VERTICAL) {
      min = bis_lerp (self->last_visible_widget_height, min, t);
      nat = bis_lerp (self->last_visible_widget_height, nat, t);
    } else {
      min = bis_lerp (self->last_visible_widget_width, min, t);
      nat = bis_lerp (self->last_visible_widget_width, nat, t);
    }
  }

  if (minimum)
    *minimum = min;
  if (natural)
    *natural = nat;
  if (minimum_baseline)
    *minimum_baseline = -1;
  if (natural_baseline)
    *natural_baseline = -1;
}

static void
bis_squeezer_dispose (GObject *object)
{
  BisSqueezer *self = BIS_SQUEEZER (object);
  GtkWidget *child;

  if (self->pages)
    g_list_model_items_changed (G_LIST_MODEL (self->pages), 0,
                                g_list_length (self->children), 0);

  while ((child = gtk_widget_get_first_child (GTK_WIDGET (self))))
    squeezer_remove (self, child, TRUE);

  g_clear_object (&self->animation);

  G_OBJECT_CLASS (bis_squeezer_parent_class)->dispose (object);
}

static void
bis_squeezer_finalize (GObject *object)
{
  BisSqueezer *self = BIS_SQUEEZER (object);

  if (self->pages)
    g_object_remove_weak_pointer (G_OBJECT (self->pages),
                                  (gpointer *) &self->pages);

  G_OBJECT_CLASS (bis_squeezer_parent_class)->finalize (object);
}

static void
bis_squeezer_class_init (BisSqueezerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->get_property = bis_squeezer_get_property;
  object_class->set_property = bis_squeezer_set_property;
  object_class->dispose = bis_squeezer_dispose;
  object_class->finalize = bis_squeezer_finalize;

  widget_class->size_allocate = bis_squeezer_size_allocate;
  widget_class->snapshot = bis_squeezer_snapshot;
  widget_class->measure = bis_squeezer_measure;
  widget_class->get_request_mode = bis_widget_get_request_mode;
  widget_class->compute_expand = bis_widget_compute_expand;

  g_object_class_override_property (object_class,
                                    PROP_ORIENTATION,
                                    "orientation");

  /**
   * BisSqueezer:visible-child: (attributes org.gtk.Property.get=bis_squeezer_get_visible_child)
   *
   * The currently visible child.
   *
   * Since: 1.0
   */
  props[PROP_VISIBLE_CHILD] =
    g_param_spec_object ("visible-child", NULL, NULL,
                         GTK_TYPE_WIDGET,
                         G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  /**
   * BisSqueezer:homogeneous: (attributes org.gtk.Property.get=bis_squeezer_get_homogeneous org.gtk.Property.set=bis_squeezer_set_homogeneous)
   *
   * Whether all children have the same size for the opposite orientation.
   *
   * For example, if a squeezer is horizontal and is homogeneous, it will
   * request the same height for all its children. If it isn't, the squeezer may
   * change size when a different child becomes visible.
   *
   * Since: 1.0
   */
  props[PROP_HOMOGENEOUS] =
    g_param_spec_boolean ("homogeneous", NULL, NULL,
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BisSqueezer:switch-threshold-policy: (attributes org.gtk.Property.get=bis_squeezer_get_switch_threshold_policy org.gtk.Property.set=bis_squeezer_set_switch_threshold_policy)
   *
   * The switch threshold policy.
   *
   * Determines when the squeezer will switch children.
   *
   * If set to `BIS_FOLD_THRESHOLD_POLICY_MINIMUM`, it will only switch when the
   * visible child cannot fit anymore. With `BIS_FOLD_THRESHOLD_POLICY_NATURAL`,
   * it will switch as soon as the visible child doesn't get their natural size.
   *
   * This can be useful if you have a long ellipsizing label and want to let it
   * ellipsize instead of immediately switching.
   *
   * Since: 1.0
   */
  props[PROP_SWITCH_THRESHOLD_POLICY] =
    g_param_spec_enum ("switch-threshold-policy", NULL, NULL,
                       BIS_TYPE_FOLD_THRESHOLD_POLICY,
                       BIS_FOLD_THRESHOLD_POLICY_NATURAL,
                       G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BisSqueezer:allow-none: (attributes org.gtk.Property.get=bis_squeezer_get_allow_none org.gtk.Property.set=bis_squeezer_set_allow_none)
   *
   * Whether to allow squeezing beyond the last child's minimum size.
   *
   * If set to `TRUE`, the squeezer can shrink to the point where no child can
   * be shown. This is functionally equivalent to appending a widget with 0×0
   * minimum size.
   *
   * Since: 1.0
   */
  props[PROP_ALLOW_NONE] =
    g_param_spec_boolean ("allow-none", NULL, NULL,
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BisSqueezer:transition-duration: (attributes org.gtk.Property.get=bis_squeezer_get_transition_duration org.gtk.Property.set=bis_squeezer_set_transition_duration)
   *
   * The transition animation duration, in milliseconds.
   *
   * Since: 1.0
   */
  props[PROP_TRANSITION_DURATION] =
    g_param_spec_uint ("transition-duration", NULL, NULL,
                       0, G_MAXUINT, 200,
                       G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BisSqueezer:transition-type: (attributes org.gtk.Property.get=bis_squeezer_get_transition_type org.gtk.Property.set=bis_squeezer_set_transition_type)
   *
   * The type of animation used for transitions between children.
   *
   * Since: 1.0
   */
  props[PROP_TRANSITION_TYPE] =
    g_param_spec_enum ("transition-type", NULL, NULL,
                       BIS_TYPE_SQUEEZER_TRANSITION_TYPE,
                       BIS_SQUEEZER_TRANSITION_TYPE_NONE,
                       G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BisSqueezer:transition-running: (attributes org.gtk.Property.get=bis_squeezer_get_transition_running)
   *
   * Whether a transition is currently running.
   *
   * If a transition is impossible, the property value will be set to `TRUE` and
   * then immediately to `FALSE`, so it's possible to rely on its notifications
   * to know that a transition has happened.
   *
   * Since: 1.0
   */
  props[PROP_TRANSITION_RUNNING] =
    g_param_spec_boolean ("transition-running", NULL, NULL,
                          FALSE,
                          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  /**
   * BisSqueezer:interpolate-size: (attributes org.gtk.Property.get=bis_squeezer_get_interpolate_size org.gtk.Property.set=bis_squeezer_set_interpolate_size)
   *
   * Whether the squeezer interpolates its size when changing the visible child.
   *
   * If `TRUE`, the squeezer will interpolate its size between the one of the
   * previous visible child and the one of the new visible child, according to
   * the set transition duration and the orientation, e.g. if the squeezer is
   * horizontal, it will interpolate the its height.
   *
   * Since: 1.0
   */
  props[PROP_INTERPOLATE_SIZE] =
    g_param_spec_boolean ("interpolate-size", NULL, NULL,
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BisSqueezer:xalign: (attributes org.gtk.Property.get=bis_squeezer_get_xalign org.gtk.Property.set=bis_squeezer_set_xalign)
   *
   * The horizontal alignment, from 0 (start) to 1 (end).
   *
   * This affects the children allocation during transitions, when they exceed
   * the size of the squeezer.
   *
   * For example, 0.5 means the child will be centered, 0 means it will keep the
   * start side aligned and overflow the end side, and 1 means the opposite.
   *
   * Since: 1.0
   */
  props[PROP_XALIGN] =
    g_param_spec_float ("xalign", NULL, NULL,
                        0.0, 1.0,
                        0.5,
                        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BisSqueezer:yalign: (attributes org.gtk.Property.get=bis_squeezer_get_yalign org.gtk.Property.set=bis_squeezer_set_yalign)
   *
   * The vertical alignment, from 0 (top) to 1 (bottom).
   *
   * This affects the children allocation during transitions, when they exceed
   * the size of the squeezer.
   *
   * For example, 0.5 means the child will be centered, 0 means it will keep the
   * top side aligned and overflow the bottom side, and 1 means the opposite.
   *
   * Since: 1.0
   */
  props[PROP_YALIGN] =
    g_param_spec_float ("yalign", NULL, NULL,
                        0.0, 1.0,
                        0.5,
                        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BisSqueezer:pages: (attributes org.gtk.Property.get=bis_squeezer_get_pages)
   *
   * A selection model with the squeezer's pages.
   *
   * This can be used to keep an up-to-date view. The model also implements
   * [iface@Gtk.SelectionModel] and can be used to track the visible page.
   *
   * Since: 1.0
   */
  props[PROP_PAGES] =
    g_param_spec_object ("pages", NULL, NULL,
                         GTK_TYPE_SELECTION_MODEL,
                         G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, LAST_PROP, props);

  gtk_widget_class_set_css_name (widget_class, "squeezer");
}

static void
bis_squeezer_init (BisSqueezer *self)
{
  BisAnimationTarget *target;

  self->homogeneous = TRUE;
  self->transition_duration = 200;
  self->transition_type = BIS_SQUEEZER_TRANSITION_TYPE_NONE;
  self->xalign = 0.5;
  self->yalign = 0.5;

  target = bis_callback_animation_target_new ((BisAnimationTargetFunc) transition_cb,
                                              self, NULL);
  self->animation = bis_timed_animation_new (GTK_WIDGET (self), 0, 1,
                                             self->transition_duration,
                                             target);
  bis_timed_animation_set_easing (BIS_TIMED_ANIMATION (self->animation),
                                  BIS_LINEAR);
  g_signal_connect_swapped (self->animation, "done",
                            G_CALLBACK (transition_done_cb), self);
}

static void
bis_squeezer_buildable_add_child (GtkBuildable *buildable,
                                  GtkBuilder   *builder,
                                  GObject      *child,
                                  const char   *type)
{
  if (BIS_IS_SQUEEZER_PAGE (child))
    add_page (BIS_SQUEEZER (buildable), BIS_SQUEEZER_PAGE (child));
  else if (GTK_IS_WIDGET (child))
    bis_squeezer_add (BIS_SQUEEZER (buildable), GTK_WIDGET (child));
  else
    parent_buildable_iface->add_child (buildable, builder, child, type);
}

static void
bis_squeezer_buildable_init (GtkBuildableIface *iface)
{
  parent_buildable_iface = g_type_interface_peek_parent (iface);

  iface->add_child = bis_squeezer_buildable_add_child;
}

/**
 * bis_squeezer_page_get_child: (attributes org.gtk.Method.get_property=child)
 * @self: a squeezer page
 *
 * Returns the squeezer child to which @self belongs.
 *
 * Returns: (transfer none): the child to which @self belongs
 *
 * Since: 1.0
 */
GtkWidget *
bis_squeezer_page_get_child (BisSqueezerPage *self)
{
  g_return_val_if_fail (BIS_IS_SQUEEZER_PAGE (self), NULL);

  return self->widget;
}

/**
 * bis_squeezer_page_get_enabled: (attributes org.gtk.Method.get_property=enabled)
 * @self: a squeezer page
 *
 * Gets whether @self is enabled.
 *
 * Returns: whether @self is enabled
 *
 * Since: 1.0
 */
gboolean
bis_squeezer_page_get_enabled (BisSqueezerPage *self)
{
  g_return_val_if_fail (BIS_IS_SQUEEZER_PAGE (self), FALSE);

  return self->enabled;
}

/**
 * bis_squeezer_page_set_enabled: (attributes org.gtk.Method.set_property=enabled)
 * @self: a squeezer page
 * @enabled: whether @self is enabled
 *
 * Sets whether @self is enabled.
 *
 * If a child is disabled, it will be ignored when looking for the child
 * fitting the available size best.
 *
 * This allows to programmatically and prematurely hide a child even if it fits
 * in the available space.
 *
 * This can be used e.g. to ensure a certain child is hidden below a certain
 * window width, or any other constraint you find suitable.
 *
 * Since: 1.0
 */
void
bis_squeezer_page_set_enabled (BisSqueezerPage *self,
                               gboolean         enabled)
{
  g_return_if_fail (BIS_IS_SQUEEZER_PAGE (self));

  enabled = !!enabled;

  if (enabled == self->enabled)
    return;

  self->enabled = enabled;

  if (self->widget && gtk_widget_get_parent (self->widget)) {
    BisSqueezer *squeezer = BIS_SQUEEZER (gtk_widget_get_parent (self->widget));

    gtk_widget_queue_resize (GTK_WIDGET (squeezer));
    update_child_visible (squeezer, self);
  }

  g_object_notify_by_pspec (G_OBJECT (self), page_props[PAGE_PROP_ENABLED]);
}

/**
 * bis_squeezer_new:
 *
 * Creates a new `BisSqueezer`.
 *
 * Returns: the newly created `BisSqueezer`
 *
 * Since: 1.0
 */
GtkWidget *
bis_squeezer_new (void)
{
  return g_object_new (BIS_TYPE_SQUEEZER, NULL);
}

/**
 * bis_squeezer_add:
 * @self: a squeezer
 * @child: the widget to add
 *
 * Adds a child to @self.
 *
 * Returns: (transfer none): the [class@SqueezerPage] for @child
 *
 * Since: 1.0
 */
BisSqueezerPage *
bis_squeezer_add (BisSqueezer *self,
                  GtkWidget   *child)
{
  BisSqueezerPage *page;

  g_return_val_if_fail (BIS_IS_SQUEEZER (self), NULL);
  g_return_val_if_fail (GTK_IS_WIDGET (child), NULL);

  page = g_object_new (BIS_TYPE_SQUEEZER_PAGE, NULL);
  page->widget = g_object_ref (child);

  add_page (self, page);

  g_object_unref (page);

  return page;
}

/**
 * bis_squeezer_remove:
 * @self: a squeezer
 * @child: the child to remove
 *
 * Removes a child widget from @self.
 *
 * Since: 1.0
 */
void
bis_squeezer_remove (BisSqueezer *self,
                     GtkWidget   *child)
{
  GList *l;
  guint position;

  g_return_if_fail (BIS_IS_SQUEEZER (self));
  g_return_if_fail (GTK_IS_WIDGET (child));
  g_return_if_fail (gtk_widget_get_parent (child) == GTK_WIDGET (self));

  for (l = self->children, position = 0; l; l = l->next, position++) {
    BisSqueezerPage *page = l->data;

    if (page->widget == child)
      break;
  }

  squeezer_remove (self, child, FALSE);

  if (self->pages)
    g_list_model_items_changed (G_LIST_MODEL (self->pages), position, 1, 0);
}

/**
 * bis_squeezer_get_page:
 * @self: a squeezer
 * @child: a child of @self
 *
 * Returns the [class@SqueezerPage] object for @child.
 *
 * Returns: (transfer none): the page object for @child
 *
 * Since: 1.0
 */
BisSqueezerPage *
bis_squeezer_get_page (BisSqueezer *self,
                       GtkWidget   *child)
{
  g_return_val_if_fail (BIS_IS_SQUEEZER (self), NULL);
  g_return_val_if_fail (GTK_IS_WIDGET (child), NULL);

  return find_page_for_widget (self, child);
}

/**
 * bis_squeezer_get_visible_child: (attributes org.gtk.Method.get_property=visible-child)
 * @self: a squeezer
 *
 * Gets the currently visible child of @self.
 *
 * Returns: (transfer none) (nullable): the visible child
 *
 * Since: 1.0
 */
GtkWidget *
bis_squeezer_get_visible_child (BisSqueezer *self)
{
  g_return_val_if_fail (BIS_IS_SQUEEZER (self), NULL);

  return self->visible_child ? self->visible_child->widget : NULL;
}

/**
 * bis_squeezer_get_homogeneous: (attributes org.gtk.Method.get_property=homogeneous)
 * @self: a squeezer
 *
 * Gets whether all children have the same size for the opposite orientation.
 *
 * Returns: whether @self is homogeneous
 *
 * Since: 1.0
 */
gboolean
bis_squeezer_get_homogeneous (BisSqueezer *self)
{
  g_return_val_if_fail (BIS_IS_SQUEEZER (self), FALSE);

  return self->homogeneous;
}

/**
 * bis_squeezer_set_homogeneous: (attributes org.gtk.Method.set_property=homogeneous)
 * @self: a squeezer
 * @homogeneous: whether @self is homogeneous
 *
 * Sets whether all children have the same size for the opposite orientation.
 *
 * For example, if a squeezer is horizontal and is homogeneous, it will request
 * the same height for all its children. If it isn't, the squeezer may change
 * size when a different child becomes visible.
 *
 * Since: 1.0
 */
void
bis_squeezer_set_homogeneous (BisSqueezer *self,
                              gboolean     homogeneous)
{
  g_return_if_fail (BIS_IS_SQUEEZER (self));

  homogeneous = !!homogeneous;

  if (self->homogeneous == homogeneous)
    return;

  self->homogeneous = homogeneous;

  if (gtk_widget_get_visible (GTK_WIDGET(self)))
    gtk_widget_queue_resize (GTK_WIDGET (self));

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_HOMOGENEOUS]);
}

/**
 * bis_squeezer_get_switch_threshold_policy: (attributes org.gtk.Method.get_property=switch-threshold-policy)
 * @self: a squeezer
 *
 * Gets the switch threshold policy for @self.
 *
 * Since: 1.0
 */
BisFoldThresholdPolicy
bis_squeezer_get_switch_threshold_policy (BisSqueezer *self)
{
  g_return_val_if_fail (BIS_IS_SQUEEZER (self), BIS_FOLD_THRESHOLD_POLICY_NATURAL);

  return self->switch_threshold_policy;
}


/**
 * bis_squeezer_set_switch_threshold_policy: (attributes org.gtk.Method.set_property=switch-threshold-policy)
 * @self: a squeezer
 * @policy: the policy to use
 *
 * Sets the switch threshold policy for @self.
 *
 * Determines when the squeezer will switch children.
 *
 * If set to `BIS_FOLD_THRESHOLD_POLICY_MINIMUM`, it will only switch when the
 * visible child cannot fit anymore. With `BIS_FOLD_THRESHOLD_POLICY_NATURAL`,
 * it will switch as soon as the visible child doesn't get their natural size.
 *
 * This can be useful if you have a long ellipsizing label and want to let it
 * ellipsize instead of immediately switching.
 *
 * Since: 1.0
 */
void
bis_squeezer_set_switch_threshold_policy (BisSqueezer            *self,
                                          BisFoldThresholdPolicy  policy)
{
  g_return_if_fail (BIS_IS_SQUEEZER (self));
  g_return_if_fail (policy <= BIS_FOLD_THRESHOLD_POLICY_NATURAL);

  if (self->switch_threshold_policy == policy)
    return;

  self->switch_threshold_policy = policy;

  gtk_widget_queue_allocate (GTK_WIDGET (self));

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_SWITCH_THRESHOLD_POLICY]);
}

/**
 * bis_squeezer_get_allow_none: (attributes org.gtk.Method.get_property=allow-none)
 * @self: a squeezer
 *
 * Gets whether to allow squeezing beyond the last child's minimum size.
 *
 * Returns: whether @self allows squeezing beyond the last child
 *
 * Since: 1.0
 */
gboolean
bis_squeezer_get_allow_none (BisSqueezer *self)
{
  g_return_val_if_fail (BIS_IS_SQUEEZER (self), FALSE);

  return self->allow_none;
}

/**
 * bis_squeezer_set_allow_none: (attributes org.gtk.Method.set_property=allow-none)
 * @self: a squeezer
 * @allow_none: whether @self allows squeezing beyond the last child
 *
 * Sets whether to allow squeezing beyond the last child's minimum size.
 *
 * If set to `TRUE`, the squeezer can shrink to the point where no child can be
 * shown. This is functionally equivalent to appending a widget with 0×0 minimum
 * size.
 *
 * Since: 1.0
 */
void
bis_squeezer_set_allow_none (BisSqueezer *self,
                             gboolean     allow_none)
{
  g_return_if_fail (BIS_IS_SQUEEZER (self));

  allow_none = !!allow_none;

  if (self->allow_none == allow_none)
    return;

  self->allow_none = allow_none;

  gtk_widget_queue_resize (GTK_WIDGET (self));

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_ALLOW_NONE]);
}

/**
 * bis_squeezer_get_transition_duration: (attributes org.gtk.Method.get_property=transition-duration)
 * @self: a squeezer
 *
 * Gets the transition animation duration for @self.
 *
 * Returns: the transition duration, in milliseconds
 *
 * Since: 1.0
 */
guint
bis_squeezer_get_transition_duration (BisSqueezer *self)
{
  g_return_val_if_fail (BIS_IS_SQUEEZER (self), 0);

  return self->transition_duration;
}

/**
 * bis_squeezer_set_transition_duration: (attributes org.gtk.Method.set_property=transition-duration)
 * @self: a squeezer
 * @duration: the new duration, in milliseconds
 *
 * Sets the transition animation duration for @self.
 *
 * Since: 1.0
 */
void
bis_squeezer_set_transition_duration (BisSqueezer *self,
                                      guint        duration)
{
  g_return_if_fail (BIS_IS_SQUEEZER (self));

  if (self->transition_duration == duration)
    return;

  self->transition_duration = duration;

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_TRANSITION_DURATION]);
}

/**
 * bis_squeezer_get_transition_type: (attributes org.gtk.Method.get_property=transition-type)
 * @self: a squeezer
 *
 * Gets the type of animation used for transitions between children in @self.
 *
 * Returns: the current transition type of @self
 *
 * Since: 1.0
 */
BisSqueezerTransitionType
bis_squeezer_get_transition_type (BisSqueezer *self)
{
  g_return_val_if_fail (BIS_IS_SQUEEZER (self), BIS_SQUEEZER_TRANSITION_TYPE_NONE);

  return self->transition_type;
}

/**
 * bis_squeezer_set_transition_type: (attributes org.gtk.Method.set_property=transition-type)
 * @self: a squeezer
 * @transition: the new transition type
 *
 * Sets the type of animation used for transitions between children in @self.
 *
 * Since: 1.0
 */
void
bis_squeezer_set_transition_type (BisSqueezer               *self,
                                  BisSqueezerTransitionType  transition)
{
  g_return_if_fail (BIS_IS_SQUEEZER (self));

  if (self->transition_type == transition)
    return;

  self->transition_type = transition;
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_TRANSITION_TYPE]);
}

/**
 * bis_squeezer_get_transition_running: (attributes org.gtk.Method.get_property=transition-running)
 * @self: a squeezer
 *
 * Gets whether a transition is currently running for @self.
 *
 * If a transition is impossible, the property value will be set to `TRUE` and
 * then immediately to `FALSE`, so it's possible to rely on its notifications
 * to know that a transition has happened.
 *
 * Returns: whether a transition is currently running
 *
 * Since: 1.0
 */
gboolean
bis_squeezer_get_transition_running (BisSqueezer *self)
{
  g_return_val_if_fail (BIS_IS_SQUEEZER (self), FALSE);

  return self->transition_running;
}

/**
 * bis_squeezer_get_interpolate_size: (attributes org.gtk.Method.get_property=interpolate-size)
 * @self: A squeezer
 *
 * Gets whether @self interpolates its size when changing the visible child.
 *
 * Returns: whether the size is interpolated
 *
 * Since: 1.0
 */
gboolean
bis_squeezer_get_interpolate_size (BisSqueezer *self)
{
  g_return_val_if_fail (BIS_IS_SQUEEZER (self), FALSE);

  return self->interpolate_size;
}

/**
 * bis_squeezer_set_interpolate_size: (attributes org.gtk.Method.set_property=interpolate-size)
 * @self: A squeezer
 * @interpolate_size: whether to interpolate the size
 *
 * Sets whether @self interpolates its size when changing the visible child.
 *
 * If `TRUE`, the squeezer will interpolate its size between the one of the
 * previous visible child and the one of the new visible child, according to the
 * set transition duration and the orientation, e.g. if the squeezer is
 * horizontal, it will interpolate the its height.
 *
 * Since: 1.0
 */
void
bis_squeezer_set_interpolate_size (BisSqueezer *self,
                                   gboolean     interpolate_size)
{
  g_return_if_fail (BIS_IS_SQUEEZER (self));

  interpolate_size = !!interpolate_size;

  if (self->interpolate_size == interpolate_size)
    return;

  self->interpolate_size = interpolate_size;
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_INTERPOLATE_SIZE]);
}

/**
 * bis_squeezer_get_xalign: (attributes org.gtk.Method.get_property=xalign)
 * @self: a squeezer
 *
 * Gets the horizontal alignment, from 0 (start) to 1 (end).
 *
 * Returns: the alignment value
 *
 * Since: 1.0
 */
float
bis_squeezer_get_xalign (BisSqueezer *self)
{
  g_return_val_if_fail (BIS_IS_SQUEEZER (self), 0.5);

  return self->xalign;
}

/**
 * bis_squeezer_set_xalign: (attributes org.gtk.Method.set_property=xalign)
 * @self: a squeezer
 * @xalign: the new alignment value
 *
 * Sets the horizontal alignment, from 0 (start) to 1 (end).
 *
 * This affects the children allocation during transitions, when they exceed the
 * size of the squeezer.
 *
 * For example, 0.5 means the child will be centered, 0 means it will keep the
 * start side aligned and overflow the end side, and 1 means the opposite.
 *
 * Since: 1.0
 */
void
bis_squeezer_set_xalign (BisSqueezer *self,
                         float        xalign)
{
  g_return_if_fail (BIS_IS_SQUEEZER (self));

  xalign = CLAMP (xalign, 0.0, 1.0);

  if (self->xalign == xalign)
    return;

  self->xalign = xalign;
  gtk_widget_queue_draw (GTK_WIDGET (self));
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_XALIGN]);
}

/**
 * bis_squeezer_get_yalign: (attributes org.gtk.Method.get_property=yalign)
 * @self: a squeezer
 *
 * Gets the vertical alignment, from 0 (top) to 1 (bottom).
 *
 * Returns: the alignment value
 *
 * Since: 1.0
 */
float
bis_squeezer_get_yalign (BisSqueezer *self)
{
  g_return_val_if_fail (BIS_IS_SQUEEZER (self), 0.5);

  return self->yalign;
}

/**
 * bis_squeezer_set_yalign: (attributes org.gtk.Method.set_property=yalign)
 * @self: a squeezer
 * @yalign: the new alignment value
 *
 * Sets the vertical alignment, from 0 (top) to 1 (bottom).
 *
 * This affects the children allocation during transitions, when they exceed the
 * size of the squeezer.
 *
 * For example, 0.5 means the child will be centered, 0 means it will keep the
 * top side aligned and overflow the bottom side, and 1 means the opposite.
 *
 * Since: 1.0
 */
void
bis_squeezer_set_yalign (BisSqueezer *self,
                         float        yalign)
{
  g_return_if_fail (BIS_IS_SQUEEZER (self));

  yalign = CLAMP (yalign, 0.0, 1.0);

  if (self->yalign == yalign)
    return;

  self->yalign = yalign;
  gtk_widget_queue_draw (GTK_WIDGET (self));
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_YALIGN]);
}

/**
 * bis_squeezer_get_pages: (attributes org.gtk.Method.get_property=pages)
 * @self: a squeezer
 *
 * Returns a [iface@Gio.ListModel] that contains the pages of @self.
 *
 * This can be used to keep an up-to-date view. The model also implements
 * [iface@Gtk.SelectionModel] and can be used to track the visible page.
 *
 * Returns: (transfer full): a `GtkSelectionModel` for the squeezer's children
 *
 * Since: 1.0
 */
GtkSelectionModel *
bis_squeezer_get_pages (BisSqueezer *self)
{
  g_return_val_if_fail (BIS_IS_SQUEEZER (self), NULL);

  if (self->pages)
    return g_object_ref (self->pages);

  self->pages = GTK_SELECTION_MODEL (bis_squeezer_pages_new (self));
  g_object_add_weak_pointer (G_OBJECT (self->pages), (gpointer *) &self->pages);

  return self->pages;
}
