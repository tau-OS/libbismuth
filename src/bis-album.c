/*
 * Copyright (C) 2018 Purism SPC
 * Copyright (C) 2019 Alexander Mikhaylenko <exalm7659@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "config.h"

#include "bis-animation-util.h"
#include "bis-enums-private.h"
#include "bis-fold-threshold-policy.h"
#include "bis-macros-private.h"
#include "bis-album.h"
#include "bis-shadow-helper-private.h"
#include "bis-spring-animation.h"
#include "bis-swipeable.h"
#include "bis-swipe-tracker-private.h"
#include "bis-timed-animation.h"
#include "bis-widget-utils-private.h"

/**
 * BisAlbum:
 *
 * An adaptive container acting like a box or a stack.
 *
 * <picture>
 *   <source srcset="album-wide-dark.png" media="(prefers-color-scheme: dark)">
 *   <img src="album-wide.png" alt="album-wide">
 * </picture>
 * <picture>
 *   <source srcset="album-narrow-dark.png" media="(prefers-color-scheme: dark)">
 *   <img src="album-narrow.png" alt="album-narrow">
 * </picture>
 *
 * The `BisAlbum` widget can display its children like a [class@Gtk.Box] does
 * or like a [class@Gtk.Stack] does, adapting to size changes by switching
 * between the two modes.
 *
 * When there is enough space the children are displayed side by side, otherwise
 * only one is displayed and the album is said to be “folded”.
 * The threshold is dictated by the preferred minimum sizes of the children.
 * When a album is folded, the children can be navigated using swipe gestures.
 *
 * The “over” and “under” transition types stack the children one on top of the
 * other, while the “slide” transition puts the children side by side. While
 * navigating to a child on the side or below can be performed by swiping the
 * current child away, navigating to an upper child requires dragging it from
 * the edge where it resides. This doesn't affect non-dragging swipes.
 *
 * ## CSS nodes
 *
 * `BisAlbum` has a single CSS node with name `album`. The node will get the
 * style classes `.folded` when it is folded, `.unfolded` when it's not, or none
 * if it hasn't computed its fold yet.
 *
 * Since: 1.0
 */

/**
 * BisAlbumPage:
 *
 * An auxiliary class used by [class@Album].
 */

/**
 * BisAlbumTransitionType:
 * @BIS_ALBUM_TRANSITION_TYPE_OVER: Cover the old page or uncover the new page, sliding from or towards the end according to orientation, text direction and children order
 * @BIS_ALBUM_TRANSITION_TYPE_UNDER: Uncover the new page or cover the old page, sliding from or towards the start according to orientation, text direction and children order
 * @BIS_ALBUM_TRANSITION_TYPE_SLIDE: Slide from left, right, up or down according to the orientation, text direction and the children order
 *
 * Describes the possible transitions in a [class@Album] widget.
 *
 * New values may be added to this enumeration over time.
 *
 * Since: 1.0
 */

enum {
  PROP_0,
  PROP_CAN_UNFOLD,
  PROP_FOLDED,
  PROP_FOLD_THRESHOLD_POLICY,
  PROP_HOMOGENEOUS,
  PROP_VISIBLE_CHILD,
  PROP_VISIBLE_CHILD_NAME,
  PROP_TRANSITION_TYPE,
  PROP_MODE_TRANSITION_DURATION,
  PROP_CHILD_TRANSITION_PARAMS,
  PROP_CHILD_TRANSITION_RUNNING,
  PROP_CAN_NAVIGATE_BACK,
  PROP_CAN_NAVIGATE_FORWARD,
  PROP_PAGES,

  /* orientable */
  PROP_ORIENTATION,
  LAST_PROP = PROP_ORIENTATION,
};

#define BIS_FOLD_UNFOLDED FALSE
#define BIS_FOLD_FOLDED TRUE
#define BIS_FOLD_MAX 2
#define GTK_ORIENTATION_MAX 2
#define BIS_SWIPE_BORDER 32

struct _BisAlbumPage {
  GObject parent_instance;

  GtkWidget *widget;
  char *name;
  gboolean navigatable;

  /* Convenience storage for per-child temporary frequently computed values. */
  GtkAllocation alloc;
  GtkRequisition min;
  GtkRequisition nat;
  gboolean visible;
  GtkWidget *last_focus;
};

G_DEFINE_FINAL_TYPE (BisAlbumPage, bis_album_page, G_TYPE_OBJECT)

enum {
  PAGE_PROP_0,
  PAGE_PROP_CHILD,
  PAGE_PROP_NAME,
  PAGE_PROP_NAVIGATABLE,
  LAST_PAGE_PROP
};

static GParamSpec *page_props[LAST_PAGE_PROP];

struct _BisAlbum {
  GtkWidget parent_instance;

  GList *children;
  /* It is probably cheaper to store and maintain a reversed copy of the
   * children list that to reverse the list every time we need to allocate or
   * draw children for RTL languages on a horizontal widget.
   */
  GList *children_reversed;
  BisAlbumPage *visible_child;
  BisAlbumPage *last_visible_child;

  gboolean folded;
  BisFoldThresholdPolicy fold_threshold_policy;

  gboolean homogeneous;

  GtkOrientation orientation;

  BisAlbumTransitionType transition_type;

  BisSwipeTracker *tracker;

  struct {
    guint duration;

    double current_pos;

    double start_progress;
    double end_progress;

    BisAnimation *animation;
  } mode_transition;

  /* Child transition variables. */
  struct {
    double progress;

    gboolean is_gesture_active;
    gboolean is_cancelled;

    gboolean transition_running;
    BisAnimation *animation;

    int last_visible_widget_width;
    int last_visible_widget_height;

    gboolean can_navigate_back;
    gboolean can_navigate_forward;

    GtkPanDirection active_direction;
    int swipe_direction;
  } child_transition;

  BisShadowHelper *shadow_helper;
  gboolean can_unfold;

  GtkSelectionModel *pages;
};

static GParamSpec *props[LAST_PROP];

static void bis_album_buildable_init (GtkBuildableIface *iface);
static void bis_album_swipeable_init (BisSwipeableInterface *iface);

G_DEFINE_FINAL_TYPE_WITH_CODE (BisAlbum, bis_album, GTK_TYPE_WIDGET,
                               G_IMPLEMENT_INTERFACE (GTK_TYPE_ORIENTABLE, NULL)
                               G_IMPLEMENT_INTERFACE (GTK_TYPE_BUILDABLE, bis_album_buildable_init)
                               G_IMPLEMENT_INTERFACE (BIS_TYPE_SWIPEABLE, bis_album_swipeable_init))

static GtkBuildableIface *parent_buildable_iface;

static void
bis_album_page_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  BisAlbumPage *self = BIS_ALBUM_PAGE (object);

  switch (property_id) {
  case PAGE_PROP_CHILD:
    g_value_set_object (value, bis_album_page_get_child (self));
    break;
  case PAGE_PROP_NAME:
    g_value_set_string (value, bis_album_page_get_name (self));
    break;
  case PAGE_PROP_NAVIGATABLE:
    g_value_set_boolean (value, bis_album_page_get_navigatable (self));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

static void
bis_album_page_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  BisAlbumPage *self = BIS_ALBUM_PAGE (object);

  switch (property_id) {
  case PAGE_PROP_CHILD:
    g_set_object (&self->widget, g_value_get_object (value));
    break;
  case PAGE_PROP_NAME:
    bis_album_page_set_name (self, g_value_get_string (value));
    break;
  case PAGE_PROP_NAVIGATABLE:
    bis_album_page_set_navigatable (self, g_value_get_boolean (value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

static void
bis_album_page_finalize (GObject *object)
{
  BisAlbumPage *self = BIS_ALBUM_PAGE (object);

  g_clear_object (&self->widget);
  g_clear_pointer (&self->name, g_free);

  if (self->last_focus)
    g_object_remove_weak_pointer (G_OBJECT (self->last_focus),
                                  (gpointer *) &self->last_focus);

  G_OBJECT_CLASS (bis_album_page_parent_class)->finalize (object);
}

static void
bis_album_page_class_init (BisAlbumPageClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = bis_album_page_get_property;
  object_class->set_property = bis_album_page_set_property;
  object_class->finalize = bis_album_page_finalize;

  /**
   * BisAlbumPage:child: (attributes org.gtk.Property.get=bis_album_page_get_child)
   *
   * The album child to which the page belongs.
   *
   * Since: 1.0
   */
  page_props[PAGE_PROP_CHILD] =
    g_param_spec_object ("child", NULL, NULL,
                         GTK_TYPE_WIDGET,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  /**
   * BisAlbumPage:name: (attributes org.gtk.Property.get=bis_album_page_get_name org.gtk.Property.set=bis_album_page_set_name)
   *
   * The name of the child page.
   *
   * Since: 1.0
   */
  page_props[PAGE_PROP_NAME] =
    g_param_spec_string ("name", NULL, NULL,
                         NULL,
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BisAlbumPage:navigatable: (attributes org.gtk.Property.get=bis_album_page_get_navigatable org.gtk.Property.set=bis_album_page_set_navigatable)
   *
   * Whether the child can be navigated to when folded.
   *
   * If `FALSE`, the child will be ignored by
   * [method@Album.get_adjacent_child], [method@Album.navigate], and swipe
   * gestures.
   *
   * This can be used used to prevent switching to widgets like separators.
   *
   * Since: 1.0
   */
  page_props[PAGE_PROP_NAVIGATABLE] =
    g_param_spec_boolean ("navigatable", NULL, NULL,
                          TRUE,
                          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, LAST_PAGE_PROP, page_props);
}

static void
bis_album_page_init (BisAlbumPage *self)
{
  self->navigatable = TRUE;
}

#define BIS_TYPE_ALBUM_PAGES (bis_album_pages_get_type ())

G_DECLARE_FINAL_TYPE (BisAlbumPages, bis_album_pages, BIS, ALBUM_PAGES, GObject)

struct _BisAlbumPages
{
  GObject parent_instance;
  BisAlbum *album;
};

static GType
bis_album_pages_get_item_type (GListModel *model)
{
  return BIS_TYPE_ALBUM_PAGE;
}

static guint
bis_album_pages_get_n_items (GListModel *model)
{
  BisAlbumPages *self = BIS_ALBUM_PAGES (model);

  return g_list_length (self->album->children);
}

static gpointer
bis_album_pages_get_item (GListModel *model,
                            guint       position)
{
  BisAlbumPages *self = BIS_ALBUM_PAGES (model);
  BisAlbumPage *page;

  page = g_list_nth_data (self->album->children, position);

  if (!page)
    return NULL;

  return g_object_ref (page);
}

static void
bis_album_pages_list_model_init (GListModelInterface *iface)
{
  iface->get_item_type = bis_album_pages_get_item_type;
  iface->get_n_items = bis_album_pages_get_n_items;
  iface->get_item = bis_album_pages_get_item;
}

static gboolean
bis_album_pages_is_selected (GtkSelectionModel *model,
                               guint              position)
{
  BisAlbumPages *self = BIS_ALBUM_PAGES (model);
  BisAlbumPage *page;

  page = g_list_nth_data (self->album->children, position);

  return page && page == self->album->visible_child;
}

static gboolean
bis_album_pages_select_item (GtkSelectionModel *model,
                               guint              position,
                               gboolean           exclusive)
{
  BisAlbumPages *self = BIS_ALBUM_PAGES (model);
  BisAlbumPage *page;

  page = g_list_nth_data (self->album->children, position);

  bis_album_set_visible_child (self->album, page->widget);

  return TRUE;
}

static void
bis_album_pages_selection_model_init (GtkSelectionModelInterface *iface)
{
  iface->is_selected = bis_album_pages_is_selected;
  iface->select_item = bis_album_pages_select_item;
}

G_DEFINE_FINAL_TYPE_WITH_CODE (BisAlbumPages, bis_album_pages, G_TYPE_OBJECT,
                               G_IMPLEMENT_INTERFACE (G_TYPE_LIST_MODEL, bis_album_pages_list_model_init)
                               G_IMPLEMENT_INTERFACE (GTK_TYPE_SELECTION_MODEL, bis_album_pages_selection_model_init))

static void
bis_album_pages_init (BisAlbumPages *pages)
{
}

static void
bis_album_pages_class_init (BisAlbumPagesClass *klass)
{
}

static BisAlbumPages *
bis_album_pages_new (BisAlbum *album)
{
  BisAlbumPages *pages;

  pages = g_object_new (BIS_TYPE_ALBUM_PAGES, NULL);
  pages->album = album;

  return pages;
}

static inline BisNavigationDirection
adjust_direction_for_rtl (BisAlbum             *self,
                          BisNavigationDirection  direction)
{
  if (self->orientation == GTK_ORIENTATION_HORIZONTAL &&
      gtk_widget_get_direction (GTK_WIDGET (self)) == GTK_TEXT_DIR_RTL) {
    if (direction == BIS_NAVIGATION_DIRECTION_BACK)
      return BIS_NAVIGATION_DIRECTION_FORWARD;
    else
      return BIS_NAVIGATION_DIRECTION_BACK;
  }

  return direction;
}

static BisAlbumPage *
find_page_for_widget (BisAlbum *self,
                      GtkWidget  *widget)
{
  BisAlbumPage *page;
  GList *l;

  for (l = self->children; l; l = l->next) {
    page = l->data;

    if (page->widget == widget)
      return page;
  }

  return NULL;
}

static BisAlbumPage *
find_page_for_name (BisAlbum *self,
                    const char *name)
{
  BisAlbumPage *page;
  GList *l;

  for (l = self->children; l; l = l->next) {
    page = l->data;

    if (g_strcmp0 (page->name, name) == 0)
      return page;
  }

  return NULL;
}

static BisAlbumPage *
find_swipeable_page (BisAlbum             *self,
                     BisNavigationDirection  direction)
{
  BisAlbumPage *page = NULL;
  GList *l;

  l = g_list_find (self->children, self->visible_child);

  if (!l)
    return NULL;

  do {
    l = (direction == BIS_NAVIGATION_DIRECTION_BACK) ? l->prev : l->next;

    if (!l)
      break;

    page = l->data;
  } while (page && !page->navigatable);

  return page;
}

static GList *
get_directed_children (BisAlbum *self)
{
  return self->orientation == GTK_ORIENTATION_HORIZONTAL &&
         gtk_widget_get_direction (GTK_WIDGET (self)) == GTK_TEXT_DIR_RTL ?
         self->children_reversed : self->children;
}

static GtkPanDirection
get_pan_direction (BisAlbum *self,
                   gboolean    new_child_first)
{
  if (self->orientation == GTK_ORIENTATION_HORIZONTAL) {
    if (gtk_widget_get_direction (GTK_WIDGET (self)) == GTK_TEXT_DIR_RTL)
      return new_child_first ? GTK_PAN_DIRECTION_LEFT : GTK_PAN_DIRECTION_RIGHT;
    else
      return new_child_first ? GTK_PAN_DIRECTION_RIGHT : GTK_PAN_DIRECTION_LEFT;
  }
  else
    return new_child_first ? GTK_PAN_DIRECTION_DOWN : GTK_PAN_DIRECTION_UP;
}

static int
get_child_window_x (BisAlbum     *self,
                    BisAlbumPage *page,
                    int             width)
{
  gboolean is_rtl;
  int rtl_multiplier;

  if (!self->child_transition.transition_running)
    return 0;

  if (self->child_transition.active_direction != GTK_PAN_DIRECTION_LEFT &&
      self->child_transition.active_direction != GTK_PAN_DIRECTION_RIGHT)
    return 0;

  is_rtl = gtk_widget_get_direction (GTK_WIDGET (self)) == GTK_TEXT_DIR_RTL;
  rtl_multiplier = is_rtl ? -1 : 1;

  if ((self->child_transition.active_direction == GTK_PAN_DIRECTION_RIGHT) == is_rtl) {
    if ((self->transition_type == BIS_ALBUM_TRANSITION_TYPE_OVER ||
         self->transition_type == BIS_ALBUM_TRANSITION_TYPE_SLIDE) &&
        page == self->visible_child)
      return width * (1 - self->child_transition.progress) * rtl_multiplier;

    if ((self->transition_type == BIS_ALBUM_TRANSITION_TYPE_UNDER ||
         self->transition_type == BIS_ALBUM_TRANSITION_TYPE_SLIDE) &&
        page == self->last_visible_child)
      return -width * self->child_transition.progress * rtl_multiplier;
  } else {
    if ((self->transition_type == BIS_ALBUM_TRANSITION_TYPE_UNDER ||
         self->transition_type == BIS_ALBUM_TRANSITION_TYPE_SLIDE) &&
        page == self->visible_child)
      return -width * (1 - self->child_transition.progress) * rtl_multiplier;

    if ((self->transition_type == BIS_ALBUM_TRANSITION_TYPE_OVER ||
         self->transition_type == BIS_ALBUM_TRANSITION_TYPE_SLIDE) &&
        page == self->last_visible_child)
      return width * self->child_transition.progress * rtl_multiplier;
  }

  return 0;
}

static int
get_child_window_y (BisAlbum     *self,
                    BisAlbumPage *page,
                    int             height)
{
  if (!self->child_transition.transition_running)
    return 0;

  if (self->child_transition.active_direction != GTK_PAN_DIRECTION_UP &&
      self->child_transition.active_direction != GTK_PAN_DIRECTION_DOWN)
    return 0;

  if (self->child_transition.active_direction == GTK_PAN_DIRECTION_UP) {
    if ((self->transition_type == BIS_ALBUM_TRANSITION_TYPE_OVER ||
         self->transition_type == BIS_ALBUM_TRANSITION_TYPE_SLIDE) &&
        page == self->visible_child)
      return height * (1 - self->child_transition.progress);

    if ((self->transition_type == BIS_ALBUM_TRANSITION_TYPE_UNDER ||
         self->transition_type == BIS_ALBUM_TRANSITION_TYPE_SLIDE) &&
        page == self->last_visible_child)
      return -height * self->child_transition.progress;
  } else {
    if ((self->transition_type == BIS_ALBUM_TRANSITION_TYPE_UNDER ||
         self->transition_type == BIS_ALBUM_TRANSITION_TYPE_SLIDE) &&
        page == self->visible_child)
      return -height * (1 - self->child_transition.progress);

    if ((self->transition_type == BIS_ALBUM_TRANSITION_TYPE_OVER ||
         self->transition_type == BIS_ALBUM_TRANSITION_TYPE_SLIDE) &&
        page == self->last_visible_child)
      return height * self->child_transition.progress;
  }

  return 0;
}

static void
set_child_transition_running (BisAlbum *self,
                              gboolean    running)
{
  if (self->child_transition.transition_running == running)
    return;

  self->child_transition.transition_running = running;
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_CHILD_TRANSITION_RUNNING]);
}

static void
child_transition_cb (double      value,
                     BisAlbum *self)
{
  self->child_transition.progress = value;

  if (!self->homogeneous)
    gtk_widget_queue_resize (GTK_WIDGET (self));
  else
    gtk_widget_queue_allocate (GTK_WIDGET (self));
}

static void
child_transition_done_cb (BisAlbum *self)
{
  if (self->child_transition.is_cancelled) {
    if (self->last_visible_child != NULL) {
      if (self->folded) {
        gtk_widget_set_child_visible (self->last_visible_child->widget, TRUE);
        gtk_widget_set_child_visible (self->visible_child->widget, FALSE);
      }
      self->visible_child = self->last_visible_child;
      self->last_visible_child = NULL;
    }

    self->child_transition.is_cancelled = FALSE;

    g_object_freeze_notify (G_OBJECT (self));
    g_object_notify_by_pspec (G_OBJECT (self), props[PROP_VISIBLE_CHILD]);
    g_object_notify_by_pspec (G_OBJECT (self), props[PROP_VISIBLE_CHILD_NAME]);
    g_object_thaw_notify (G_OBJECT (self));
  } else {
    if (self->last_visible_child != NULL) {
      if (self->folded)
        gtk_widget_set_child_visible (self->last_visible_child->widget, FALSE);
      self->last_visible_child = NULL;
    }
  }

  bis_animation_reset (self->child_transition.animation);

  set_child_transition_running (self, FALSE);

  self->child_transition.swipe_direction = 0;
}

static void
set_visible_child (BisAlbum     *self,
                   BisAlbumPage *page)
{
  GtkWidget *widget = GTK_WIDGET (self);
  GtkRoot *root;
  GtkWidget *focus;
  gboolean contains_focus = FALSE;
  GtkPanDirection transition_direction = GTK_PAN_DIRECTION_LEFT;
  guint old_pos = GTK_INVALID_LIST_POSITION;
  guint new_pos = GTK_INVALID_LIST_POSITION;
  gboolean skip_transition = FALSE;

  /* If we are being destroyed, do not bother with transitions and
   * notifications.
   */
  if (gtk_widget_in_destruction (widget))
    return;

  /* If none, pick first visible. */
  if (!page) {
    GList *l;

    for (l = self->children; l; l = l->next) {
      BisAlbumPage *p = l->data;

      if (gtk_widget_get_visible (p->widget)) {
        page = p;

        break;
      }
    }
  }

  if (page == self->visible_child)
    return;

  if (self->pages) {
    guint position;
    GList *l;

    for (l = self->children, position = 0; l; l = l->next, position++) {
      BisAlbumPage *p = l->data;
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

  if (self->child_transition.transition_running)
    bis_animation_skip (self->child_transition.animation);

  if (self->visible_child && self->visible_child->widget) {
    if (gtk_widget_is_visible (widget)) {
      self->last_visible_child = self->visible_child;
      self->child_transition.last_visible_widget_width = gtk_widget_get_width (self->last_visible_child->widget);
      self->child_transition.last_visible_widget_height = gtk_widget_get_height (self->last_visible_child->widget);
    } else {
      gtk_widget_set_child_visible (self->visible_child->widget, !self->folded);
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

  if (page == NULL || self->last_visible_child == NULL)
    skip_transition = TRUE;
  else {
    gboolean new_first = FALSE;
    GList *l;

    for (l = self->children; l; l = l->next) {
      if (page == l->data) {
        new_first = TRUE;

        break;
      }
      if (self->last_visible_child == l->data)
        break;
    }

    transition_direction = get_pan_direction (self, new_first);
  }

  if (self->folded) {
    if (self->homogeneous)
      gtk_widget_queue_allocate (widget);
    else
      gtk_widget_queue_resize (widget);

    self->child_transition.active_direction = transition_direction;
    self->child_transition.progress = 0;
    self->child_transition.is_cancelled = FALSE;

    if (!self->child_transition.is_gesture_active) {
      bis_spring_animation_set_value_from (BIS_SPRING_ANIMATION (self->child_transition.animation), 0);
      bis_spring_animation_set_value_to (BIS_SPRING_ANIMATION (self->child_transition.animation), 1);
      bis_spring_animation_set_initial_velocity (BIS_SPRING_ANIMATION (self->child_transition.animation), 0);

      set_child_transition_running (self, TRUE);

      if (skip_transition)
        bis_animation_skip (self->child_transition.animation);
      else
        bis_animation_play (self->child_transition.animation);
    }
  }

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

  g_object_freeze_notify (G_OBJECT (self));
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_VISIBLE_CHILD]);
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_VISIBLE_CHILD_NAME]);
  g_object_thaw_notify (G_OBJECT (self));
}

static void
mode_transition_cb (double      value,
                    BisAlbum *self)
{
  self->mode_transition.current_pos = value;

  if (self->homogeneous)
    gtk_widget_queue_allocate (GTK_WIDGET (self));
  else
    gtk_widget_queue_resize (GTK_WIDGET (self));
}

static void
start_mode_transition (BisAlbum *self,
                       double      target)
{
  if (bis_timed_animation_get_value_to (BIS_TIMED_ANIMATION (self->mode_transition.animation)) == target)
    return;

  bis_animation_skip (self->child_transition.animation);

  bis_timed_animation_set_value_from (BIS_TIMED_ANIMATION (self->mode_transition.animation),
                                      self->mode_transition.current_pos);
  bis_timed_animation_set_value_to (BIS_TIMED_ANIMATION (self->mode_transition.animation),
                                    target);

  if (self->can_unfold)
    bis_animation_play (self->mode_transition.animation);
  else
    bis_animation_skip (self->mode_transition.animation);
}

static void
set_folded (BisAlbum *self,
            gboolean    folded)
{
  if (self->folded == folded)
    return;

  self->folded = folded;

  start_mode_transition (self, folded ? 0.0 : 1.0);

  if (folded) {
    gtk_widget_add_css_class (GTK_WIDGET (self), "folded");
    gtk_widget_remove_css_class (GTK_WIDGET (self), "unfolded");
  } else {
    gtk_widget_remove_css_class (GTK_WIDGET (self), "folded");
    gtk_widget_add_css_class (GTK_WIDGET (self), "unfolded");
  }

  g_object_notify_by_pspec (G_OBJECT (self),
                            props[PROP_FOLDED]);
}

static inline int
get_page_size (BisAlbum     *self,
               BisAlbumPage *page,
               GtkOrientation  orientation)
{
  GtkRequisition *req;

  if (self->fold_threshold_policy == BIS_FOLD_THRESHOLD_POLICY_MINIMUM)
    req = &page->min;
  else
    req = &page->nat;

  return orientation == GTK_ORIENTATION_HORIZONTAL ? req->width : req->height;
}

static void
bis_album_size_allocate_folded (BisAlbum *self,
                                  int         width,
                                  int         height)
{
  GtkWidget *widget = GTK_WIDGET (self);
  GtkOrientation orientation = gtk_orientable_get_orientation (GTK_ORIENTABLE (widget));
  GList *directed_children, *children;
  BisAlbumPage *page, *visible_child;
  int start_size, end_size, visible_size;
  int remaining_start_size, remaining_end_size, remaining_size;
  int current_pad;
  int start_position, end_position;
  BisAlbumTransitionType mode_transition_type;
  GtkTextDirection direction;
  gboolean under;

  directed_children = get_directed_children (self);
  visible_child = self->visible_child;

  if (!visible_child)
    return;

  for (children = directed_children; children; children = children->next) {
    page = children->data;

    if (!page->widget)
      continue;

    if (page->widget == visible_child->widget)
      continue;

    if (self->last_visible_child &&
        page->widget == self->last_visible_child->widget)
      continue;

    page->visible = FALSE;
  }

  if (visible_child->widget == NULL)
    return;

  /* FIXME is this needed? */
  if (!gtk_widget_get_visible (visible_child->widget)) {
    visible_child->visible = FALSE;

    return;
  }

  visible_child->visible = TRUE;

  mode_transition_type = self->transition_type;

  /* Avoid useless computations and allow visible child transitions. */
  if (self->mode_transition.current_pos <= 0.0) {
    /* Child transitions should be applied only when folded and when no mode
     * transition is ongoing.
     */
    for (children = directed_children; children; children = children->next) {
      page = children->data;

      if (page != visible_child &&
          page != self->last_visible_child) {
        page->visible = FALSE;

        continue;
      }

      page->alloc.x = get_child_window_x (self, page, width);
      page->alloc.y = get_child_window_y (self, page, height);
      page->alloc.width = width;
      page->alloc.height = height;
      page->visible = TRUE;
    }

    return;
  }

  /* Compute visible child size. */
  visible_size = orientation == GTK_ORIENTATION_HORIZONTAL ?
    MIN (width,  MAX (get_page_size (self, visible_child, orientation), (int) (width  * (1.0 - self->mode_transition.current_pos)))) :
    MIN (height, MAX (get_page_size (self, visible_child, orientation), (int) (height * (1.0 - self->mode_transition.current_pos))));

  /* Compute the start size. */
  start_size = 0;
  for (children = directed_children; children; children = children->next) {
    page = children->data;

    if (page == visible_child)
      break;

    start_size += get_page_size (self, page, orientation);
  }

  /* Compute the end size. */
  end_size = 0;
  for (children = g_list_last (directed_children); children; children = children->prev) {
    page = children->data;

    if (page == visible_child)
      break;

    end_size += get_page_size (self, page, orientation);
  }

  /* Compute pads. */
  remaining_size = orientation == GTK_ORIENTATION_HORIZONTAL ?
    width - visible_size :
    height - visible_size;
  remaining_start_size = (int) (remaining_size * ((double) start_size / (double) (start_size + end_size)));
  remaining_end_size = remaining_size - remaining_start_size;

  /* Store start and end allocations. */
  switch (orientation) {
  case GTK_ORIENTATION_HORIZONTAL:
    direction = gtk_widget_get_direction (GTK_WIDGET (self));
    under = (mode_transition_type == BIS_ALBUM_TRANSITION_TYPE_OVER && direction == GTK_TEXT_DIR_LTR) ||
            (mode_transition_type == BIS_ALBUM_TRANSITION_TYPE_UNDER && direction == GTK_TEXT_DIR_RTL);
    start_position = under ? 0 : remaining_start_size - start_size;
    self->mode_transition.start_progress = under ? (double) remaining_size / start_size : 1;
    under = (mode_transition_type == BIS_ALBUM_TRANSITION_TYPE_UNDER && direction == GTK_TEXT_DIR_LTR) ||
            (mode_transition_type == BIS_ALBUM_TRANSITION_TYPE_OVER && direction == GTK_TEXT_DIR_RTL);
    end_position = under ? width - end_size : remaining_start_size + visible_size;
    self->mode_transition.end_progress = under ? (double) remaining_end_size / end_size : 1;
    break;
  case GTK_ORIENTATION_VERTICAL:
    under = mode_transition_type == BIS_ALBUM_TRANSITION_TYPE_OVER;
    start_position = under ? 0 : remaining_start_size - start_size;
    self->mode_transition.start_progress = under ? (double) remaining_size / start_size : 1;
    under = mode_transition_type == BIS_ALBUM_TRANSITION_TYPE_UNDER;
    end_position = remaining_start_size + visible_size;
    self->mode_transition.end_progress = under ? (double) remaining_end_size / end_size : 1;
    break;
  default:
    g_assert_not_reached ();
  }

  /* Allocate visible child. */
  if (orientation == GTK_ORIENTATION_HORIZONTAL) {
    visible_child->alloc.width = visible_size;
    visible_child->alloc.height = height;
    visible_child->alloc.x = remaining_start_size;
    visible_child->alloc.y = 0;
    visible_child->visible = TRUE;
  }
  else {
    visible_child->alloc.width = width;
    visible_child->alloc.height = visible_size;
    visible_child->alloc.x = 0;
    visible_child->alloc.y = remaining_start_size;
    visible_child->visible = TRUE;
  }

  /* Allocate starting children. */
  current_pad = start_position;

  for (children = directed_children; children; children = children->next) {
    page = children->data;

    if (page == visible_child)
      break;

    if (orientation == GTK_ORIENTATION_HORIZONTAL) {
      page->alloc.width = get_page_size (self, page, orientation);
      page->alloc.height = height;
      page->alloc.x = current_pad;
      page->alloc.y = 0;
      page->visible = page->alloc.x + page->alloc.width > 0;

      current_pad += page->alloc.width;
    }
    else {
      page->alloc.width = width;
      page->alloc.height = get_page_size (self, page, orientation);
      page->alloc.x = 0;
      page->alloc.y = current_pad;
      page->visible = page->alloc.y + page->alloc.height > 0;

      current_pad += page->alloc.height;
    }
  }

  /* Allocate ending children. */
  current_pad = end_position;

  if (!children || !children->next)
    return;

  for (children = children->next; children; children = children->next) {
    page = children->data;

    if (orientation == GTK_ORIENTATION_HORIZONTAL) {
      page->alloc.width = get_page_size (self, page, orientation);
      page->alloc.height = height;
      page->alloc.x = current_pad;
      page->alloc.y = 0;
      page->visible = page->alloc.x < width;

      current_pad += page->alloc.width;
    }
    else {
      page->alloc.width = width;
      page->alloc.height = get_page_size (self, page, orientation);
      page->alloc.x = 0;
      page->alloc.y = current_pad;
      page->visible = page->alloc.y < height;

      current_pad += page->alloc.height;
    }
  }
}

static void
bis_album_size_allocate_unfolded (BisAlbum *self,
                                    int         width,
                                    int         height)
{
  GtkWidget *widget = GTK_WIDGET (self);
  GtkOrientation orientation = gtk_orientable_get_orientation (GTK_ORIENTABLE (widget));
  GList *directed_children, *children;
  BisAlbumPage *page, *visible_child;
  int min_size, extra_size;
  int per_child_extra = 0, n_extra_widgets = 0;
  int n_visible_children, n_expand_children;
  int start_pad = 0, end_pad = 0;
  int i = 0, position = 0;
  BisAlbumTransitionType mode_transition_type;
  GtkTextDirection direction;
  gboolean under;
  GtkRequestedSize *sizes;

  visible_child = self->visible_child;
  if (!visible_child)
    return;

  directed_children = get_directed_children (self);

  n_visible_children = n_expand_children = 0;
  for (children = directed_children; children; children = children->next) {
    page = children->data;

    page->visible = page->widget != NULL && gtk_widget_get_visible (page->widget);

    if (page->visible) {
      n_visible_children++;
      if (gtk_widget_compute_expand (page->widget, orientation))
        n_expand_children++;
    }
    else {
      page->min.width = page->min.height = 0;
      page->nat.width = page->nat.height = 0;
    }
  }

  sizes = g_newa (GtkRequestedSize, n_visible_children);

  /* Compute repartition of extra space. */

  min_size = 0;
  if (orientation == GTK_ORIENTATION_HORIZONTAL) {
    for (children = directed_children; children; children = children->next) {
      page = children->data;

      if (!page->visible)
        continue;

      min_size += page->min.width;

      sizes[i].minimum_size = page->min.width;
      sizes[i].natural_size = page->nat.width;
      i++;
    }

    extra_size = MAX (min_size, width);
  } else {
    for (children = directed_children; children; children = children->next) {
      page = children->data;

      if (!page->visible)
        continue;

      min_size += page->min.height;

      sizes[i].minimum_size = page->min.height;
      sizes[i].natural_size = page->nat.height;
      i++;
    }

    extra_size = MAX (min_size, height);
  }

  g_assert (extra_size >= 0);

  /* Bring children up to size */
  extra_size -= min_size;
  extra_size = MAX (0, extra_size);
  extra_size = gtk_distribute_natural_allocation (extra_size, n_visible_children, sizes);

  /* Calculate space which hasn't been distributed yet,
   * and is available for expanding children.
   */
  if (n_expand_children > 0) {
    per_child_extra = extra_size / n_expand_children;
    n_extra_widgets = extra_size % n_expand_children;
  }

  /* Allocate sizes */
  i = 0;
  for (children = directed_children; children; children = children->next) {
    int allocated_size;

    page = children->data;

    if (!page->visible)
      continue;

    allocated_size = sizes[i].minimum_size;

    if (gtk_widget_compute_expand (page->widget, orientation)) {
      allocated_size += per_child_extra;

      if (n_extra_widgets > 0) {
        allocated_size++;
        n_extra_widgets--;
      }
    }

    if (orientation == GTK_ORIENTATION_HORIZONTAL) {
      page->alloc.x = position;
      page->alloc.y = 0;
      page->alloc.width = allocated_size;
      page->alloc.height = height;
    } else {
      page->alloc.x = 0;
      page->alloc.y = position;
      page->alloc.width = width;
      page->alloc.height = allocated_size;
    }

    position += allocated_size;
    i++;
  }

  /* Apply animations. */

  if (orientation == GTK_ORIENTATION_HORIZONTAL) {
    start_pad = (int) ((visible_child->alloc.x) * (1.0 - self->mode_transition.current_pos));
    end_pad = (int) ((width - (visible_child->alloc.x + visible_child->alloc.width)) * (1.0 - self->mode_transition.current_pos));
  }
  else {
    start_pad = (int) ((visible_child->alloc.y) * (1.0 - self->mode_transition.current_pos));
    end_pad = (int) ((height - (visible_child->alloc.y + visible_child->alloc.height)) * (1.0 - self->mode_transition.current_pos));
  }

  mode_transition_type = self->transition_type;
  direction = gtk_widget_get_direction (GTK_WIDGET (self));

  if (orientation == GTK_ORIENTATION_HORIZONTAL)
    under = (mode_transition_type == BIS_ALBUM_TRANSITION_TYPE_OVER && direction == GTK_TEXT_DIR_LTR) ||
            (mode_transition_type == BIS_ALBUM_TRANSITION_TYPE_UNDER && direction == GTK_TEXT_DIR_RTL);
  else
    under = mode_transition_type == BIS_ALBUM_TRANSITION_TYPE_OVER;
  for (children = directed_children; children; children = children->next) {
    page = children->data;

    if (page == visible_child)
      break;

    if (!page->visible)
      continue;

    if (under)
      continue;

    if (orientation == GTK_ORIENTATION_HORIZONTAL)
      page->alloc.x -= start_pad;
    else
      page->alloc.y -= start_pad;
  }

  self->mode_transition.start_progress = under ? self->mode_transition.current_pos : 1;

  if (orientation == GTK_ORIENTATION_HORIZONTAL)
    under = (mode_transition_type == BIS_ALBUM_TRANSITION_TYPE_UNDER && direction == GTK_TEXT_DIR_LTR) ||
            (mode_transition_type == BIS_ALBUM_TRANSITION_TYPE_OVER && direction == GTK_TEXT_DIR_RTL);
  else
    under = mode_transition_type == BIS_ALBUM_TRANSITION_TYPE_UNDER;
  for (children = g_list_last (directed_children); children; children = children->prev) {
    page = children->data;

    if (page == visible_child)
      break;

    if (!page->visible)
      continue;

    if (under)
      continue;

    if (orientation == GTK_ORIENTATION_HORIZONTAL)
      page->alloc.x += end_pad;
    else
      page->alloc.y += end_pad;
  }

  self->mode_transition.end_progress = under ? self->mode_transition.current_pos : 1;

  if (orientation == GTK_ORIENTATION_HORIZONTAL) {
    visible_child->alloc.x -= start_pad;
    visible_child->alloc.width += start_pad + end_pad;
  }
  else {
    visible_child->alloc.y -= start_pad;
    visible_child->alloc.height += start_pad + end_pad;
  }
}

static BisAlbumPage *
get_top_overlap_child (BisAlbum *self)
{
  gboolean is_rtl, start;

  if (!self->last_visible_child)
    return self->visible_child;

  is_rtl = gtk_widget_get_direction (GTK_WIDGET (self)) == GTK_TEXT_DIR_RTL;

  start = (self->child_transition.active_direction == GTK_PAN_DIRECTION_LEFT && !is_rtl) ||
          (self->child_transition.active_direction == GTK_PAN_DIRECTION_RIGHT && is_rtl) ||
           self->child_transition.active_direction == GTK_PAN_DIRECTION_UP;

  switch (self->transition_type) {
  case BIS_ALBUM_TRANSITION_TYPE_SLIDE:
    /* Nothing overlaps in this case */
    return NULL;
  case BIS_ALBUM_TRANSITION_TYPE_OVER:
    return start ? self->visible_child : self->last_visible_child;
  case BIS_ALBUM_TRANSITION_TYPE_UNDER:
    return start ? self->last_visible_child : self->visible_child;
  default:
    g_assert_not_reached ();
  }
}

static void
update_tracker_orientation (BisAlbum *self)
{
  gboolean reverse;

  reverse = (self->orientation == GTK_ORIENTATION_HORIZONTAL &&
             gtk_widget_get_direction (GTK_WIDGET (self)) == GTK_TEXT_DIR_RTL);

  g_object_set (self->tracker,
                "orientation", self->orientation,
                "reversed", reverse,
                NULL);
}

static void
update_child_visible (BisAlbum     *self,
                      BisAlbumPage *page)
{
  gboolean enabled;

  enabled = gtk_widget_get_visible (page->widget);

  if (self->visible_child == NULL && enabled)
    set_visible_child (self, page);
  else if (self->visible_child == page && !enabled)
    set_visible_child (self, NULL);

  if (page == self->last_visible_child) {
    gtk_widget_set_child_visible (self->last_visible_child->widget, FALSE);
    self->last_visible_child = NULL;
  }
}

static void
album_child_visibility_notify_cb (GObject    *obj,
                                    GParamSpec *pspec,
                                    gpointer    user_data)
{
  BisAlbum *self = BIS_ALBUM (user_data);
  GtkWidget *child = GTK_WIDGET (obj);
  BisAlbumPage *page;

  page = find_page_for_widget (self, child);
  g_return_if_fail (page != NULL);

  update_child_visible (self, page);
}

static gboolean
can_navigate_in_direction (BisAlbum             *self,
                           BisNavigationDirection  direction)
{
  switch (direction) {
  case BIS_NAVIGATION_DIRECTION_BACK:
    return self->child_transition.can_navigate_back;
  case BIS_NAVIGATION_DIRECTION_FORWARD:
    return self->child_transition.can_navigate_forward;
  default:
    g_assert_not_reached ();
  }
}

static void
set_orientation (BisAlbum     *self,
                 GtkOrientation  orientation)
{
  if (self->orientation == orientation)
    return;

  self->orientation = orientation;
  update_tracker_orientation (self);
  gtk_widget_queue_resize (GTK_WIDGET (self));
  g_object_notify (G_OBJECT (self), "orientation");
}

static void
back_forward_button_pressed_cb (GtkGesture *gesture,
                                int         n_press,
                                double      x,
                                double      y,
                                BisAlbum *self)
{
  guint button;
  BisNavigationDirection direction;

  if (n_press > 1) {
    gtk_gesture_set_state (gesture, GTK_EVENT_SEQUENCE_DENIED);
    return;
  }

  button = gtk_gesture_single_get_current_button (GTK_GESTURE_SINGLE (gesture));

  /* Unfortunately, there are no constants for these buttons */
  if (button == 8) {
    direction = BIS_NAVIGATION_DIRECTION_BACK;
  } else if (button == 9) {
    direction = BIS_NAVIGATION_DIRECTION_FORWARD;
  } else {
    gtk_gesture_set_state (gesture, GTK_EVENT_SEQUENCE_DENIED);
    return;
  }

  direction = adjust_direction_for_rtl (self, direction);

  if (can_navigate_in_direction (self, direction) &&
      bis_album_navigate (self, direction)) {
    gtk_gesture_set_state (gesture, GTK_EVENT_SEQUENCE_CLAIMED);
    return;
  }

  gtk_gesture_set_state (gesture, GTK_EVENT_SEQUENCE_DENIED);
}

static void
prepare_cb (BisSwipeTracker        *tracker,
            BisNavigationDirection  direction,
            BisAlbum             *self)
{
  self->child_transition.swipe_direction = direction;

  if (self->child_transition.transition_running) {
    bis_animation_pause (self->child_transition.animation);
    self->child_transition.is_gesture_active = TRUE;
    self->child_transition.is_cancelled = FALSE;
  } else {
    BisAlbumPage *page = NULL;

    if (can_navigate_in_direction (self, direction) && self->folded)
      page = find_swipeable_page (self, direction);

    if (page) {
      self->child_transition.is_gesture_active = TRUE;

      g_object_freeze_notify (G_OBJECT (self));
      set_visible_child (self, page);
      set_child_transition_running (self, TRUE);
      g_object_thaw_notify (G_OBJECT (self));
    }
  }
}

static void
update_swipe_cb (BisSwipeTracker *tracker,
                 double           progress,
                 BisAlbum      *self)
{
  child_transition_cb (ABS (progress), self);
}

static void
end_swipe_cb (BisSwipeTracker *tracker,
              double           velocity,
              double           to,
              BisAlbum      *self)
{
 if (!self->child_transition.is_gesture_active)
    return;

  bis_spring_animation_set_value_from (BIS_SPRING_ANIMATION (self->child_transition.animation),
                                      self->child_transition.progress);
  bis_spring_animation_set_value_to (BIS_SPRING_ANIMATION (self->child_transition.animation),
                                    ABS (to));
  self->child_transition.is_cancelled = (to == 0);

  if (!G_APPROX_VALUE (self->child_transition.progress, ABS (to), FLT_EPSILON))
    bis_spring_animation_set_initial_velocity (BIS_SPRING_ANIMATION (self->child_transition.animation),
                                               -velocity / bis_swipeable_get_distance (BIS_SWIPEABLE (self)));
  else
    bis_spring_animation_set_initial_velocity (BIS_SPRING_ANIMATION (self->child_transition.animation),
                                               -velocity);

  bis_animation_play (self->child_transition.animation);

  self->child_transition.is_gesture_active = FALSE;

  gtk_widget_queue_draw (GTK_WIDGET (self));
}

static void
add_page (BisAlbum     *self,
          BisAlbumPage *page,
          BisAlbumPage *sibling_page)
{
  GList *l;

  g_return_if_fail (page->widget != NULL);

  if (page->name) {
    for (l = self->children; l; l = l->next) {
      BisAlbumPage *p = l->data;

      if (p->name && !g_strcmp0 (p->name, page->name)) {
        g_warning ("While adding page: duplicate child name in BisAlbum: %s", page->name);
        break;
      }
    }
  }

  g_object_ref (page);

  if (!sibling_page) {
    self->children = g_list_prepend (self->children, page);
    self->children_reversed = g_list_append (self->children_reversed, page);
  } else {
    int sibling_pos = g_list_index (self->children, sibling_page);
    int length = g_list_length (self->children);

    self->children =
      g_list_insert (self->children, page, sibling_pos + 1);
    self->children_reversed =
      g_list_insert (self->children_reversed, page, length - sibling_pos - 1);
  }

  gtk_widget_set_child_visible (page->widget, FALSE);

  if (self->transition_type == BIS_ALBUM_TRANSITION_TYPE_OVER)
    gtk_widget_insert_before (page->widget, GTK_WIDGET (self),
                              sibling_page ? sibling_page->widget : NULL);
  else
    gtk_widget_insert_after (page->widget, GTK_WIDGET (self),
                              sibling_page ? sibling_page->widget : NULL);

  if (self->pages) {
    int position = g_list_index (self->children, page);

    g_list_model_items_changed (G_LIST_MODEL (self->pages), position, 0, 1);
  }

  g_signal_connect (page->widget, "notify::visible",
                    G_CALLBACK (album_child_visibility_notify_cb), self);

  if (self->visible_child == NULL &&
      gtk_widget_get_visible (page->widget))
    set_visible_child (self, page);
  if (!self->folded ||
      self->homogeneous ||
      self->visible_child == page)
    gtk_widget_queue_resize (GTK_WIDGET (self));
}

static void
album_remove (BisAlbum *self,
                GtkWidget  *child,
                gboolean    in_dispose)
{
  BisAlbumPage *page;
  gboolean was_visible;

  page = find_page_for_widget (self, child);
  if (!page)
    return;

  self->children = g_list_remove (self->children, page);
  self->children_reversed = g_list_remove (self->children_reversed, page);

  g_signal_handlers_disconnect_by_func (child,
                                        album_child_visibility_notify_cb,
                                        self);

  was_visible = gtk_widget_get_visible (child);

  g_clear_object (&page->widget);

  if (self->visible_child == page)
    {
      if (in_dispose)
        self->visible_child = NULL;
      else
        set_visible_child (self, NULL);
    }

  if (self->last_visible_child == page)
    self->last_visible_child = NULL;

  gtk_widget_unparent (child);

  g_object_unref (page);

  if (was_visible)
    gtk_widget_queue_resize (GTK_WIDGET (self));
}

static gboolean
back_forward_shortcut_cb (BisAlbum *self,
                          GVariant   *args)
{
  BisNavigationDirection direction;

  g_variant_get (args, "h", &direction);

  direction = adjust_direction_for_rtl (self, direction);

  return can_navigate_in_direction (self, direction) &&
         bis_album_navigate (self, direction);
}

static gboolean
alt_arrows_shortcut_cb (BisAlbum *self,
                        GVariant   *args)
{
  BisNavigationDirection direction;
  GtkOrientation orientation;
  g_variant_get (args, "(hh)", &direction, &orientation);

  if (self->orientation != orientation)
    return GDK_EVENT_PROPAGATE;

  direction = adjust_direction_for_rtl (self, direction);

  return can_navigate_in_direction (self, direction) &&
         bis_album_navigate (self, direction);
}

static void
bis_album_measure (GtkWidget      *widget,
                     GtkOrientation  orientation,
                     int             for_size,
                     int            *minimum,
                     int            *natural,
                     int            *minimum_baseline,
                     int            *natural_baseline)
{
  BisAlbum *self = BIS_ALBUM (widget);
  GList *l;
  int visible_children;
  int child_min, max_min, visible_min, last_visible_min;
  int child_nat, max_nat, sum_nat;
  gboolean same_orientation;

  visible_children = 0;
  child_min = max_min = visible_min = last_visible_min = 0;
  child_nat = max_nat = sum_nat = 0;
  for (l = self->children; l; l = l->next) {
    BisAlbumPage *page = l->data;

    if (page->widget == NULL || !gtk_widget_get_visible (page->widget))
      continue;

    visible_children++;

    gtk_widget_measure (page->widget, orientation, for_size,
                        &child_min, &child_nat, NULL, NULL);

    max_min = MAX (max_min, child_min);
    max_nat = MAX (max_nat, child_nat);
    sum_nat += child_nat;
  }

  if (self->visible_child != NULL)
    gtk_widget_measure (self->visible_child->widget, orientation, for_size,
                        &visible_min, NULL, NULL, NULL);

  if (self->last_visible_child != NULL) {
    gtk_widget_measure (self->last_visible_child->widget, orientation, for_size,
                        &last_visible_min, NULL, NULL, NULL);
  } else {
    last_visible_min = visible_min;
  }

  same_orientation = orientation == gtk_orientable_get_orientation (GTK_ORIENTABLE (self));

  if (minimum) {
    if (same_orientation || self->homogeneous)
      *minimum = max_min;
    else {
      *minimum = bis_lerp (last_visible_min, visible_min, self->child_transition.progress);
      *minimum = bis_lerp (*minimum, max_min, self->mode_transition.current_pos);
    }
  }

  if (natural) {
    if (same_orientation && self->can_unfold)
      *natural = sum_nat;
    else
      *natural = max_nat;
  }

  if (minimum_baseline)
    *minimum_baseline = -1;
  if (natural_baseline)
    *natural_baseline = -1;
}

static void
allocate_shadow (BisAlbum *self,
                 int         width,
                 int         height,
                 int         baseline)
{
  BisAlbumPage *overlap_child;
  gboolean is_transition;
  gboolean is_vertical;
  gboolean is_rtl;
  gboolean is_over;
  GtkAllocation shadow_rect;
  double shadow_progress, mode_progress;
  GtkPanDirection shadow_direction;

  is_transition = self->child_transition.transition_running ||
                  bis_animation_get_state (self->mode_transition.animation) == BIS_ANIMATION_PLAYING;

  overlap_child = get_top_overlap_child (self);

  shadow_rect.x = 0;
  shadow_rect.y = 0;
  shadow_rect.width = width;
  shadow_rect.height = height;

  is_vertical = gtk_orientable_get_orientation (GTK_ORIENTABLE (self)) == GTK_ORIENTATION_VERTICAL;
  is_rtl = gtk_widget_get_direction (GTK_WIDGET (self)) == GTK_TEXT_DIR_RTL;
  is_over = self->transition_type == BIS_ALBUM_TRANSITION_TYPE_OVER;

  if (is_vertical) {
    if (!is_over)
      shadow_direction = GTK_PAN_DIRECTION_UP;
    else
      shadow_direction = GTK_PAN_DIRECTION_DOWN;
  } else {
    if (is_over == is_rtl)
      shadow_direction = GTK_PAN_DIRECTION_LEFT;
    else
      shadow_direction = GTK_PAN_DIRECTION_RIGHT;
  }

  if (!is_transition ||
      self->transition_type == BIS_ALBUM_TRANSITION_TYPE_SLIDE ||
      !overlap_child) {
    shadow_progress = 1;
  } else {
    if (is_vertical) {
      if (!is_over) {
        shadow_rect.y = overlap_child->alloc.y + overlap_child->alloc.height;
        shadow_rect.height -= shadow_rect.y;
        mode_progress = self->mode_transition.end_progress;
      } else {
        shadow_rect.height = overlap_child->alloc.y;
        mode_progress = self->mode_transition.start_progress;
      }
    } else {
      if (is_over == is_rtl) {
        shadow_rect.x = overlap_child->alloc.x + overlap_child->alloc.width;
        shadow_rect.width -= shadow_rect.x;
        mode_progress = self->mode_transition.end_progress;
      } else {
        shadow_rect.width = overlap_child->alloc.x;
        mode_progress = self->mode_transition.start_progress;
      }
    }

    if (bis_animation_get_state (self->mode_transition.animation) == BIS_ANIMATION_PLAYING) {
      shadow_progress = mode_progress;
    } else {
      GtkPanDirection direction = self->child_transition.active_direction;
      GtkPanDirection left_or_right = is_rtl ? GTK_PAN_DIRECTION_RIGHT : GTK_PAN_DIRECTION_LEFT;

      if (direction == GTK_PAN_DIRECTION_UP || direction == left_or_right)
        shadow_progress = self->child_transition.progress;
      else
        shadow_progress = 1 - self->child_transition.progress;

      if (is_over)
        shadow_progress = 1 - shadow_progress;

      /* Normalize the shadow rect size so that we can cache the shadow */
      if (shadow_direction == GTK_PAN_DIRECTION_RIGHT)
        shadow_rect.x -= (width - shadow_rect.width);
      else if (shadow_direction == GTK_PAN_DIRECTION_DOWN)
        shadow_rect.y -= (height - shadow_rect.height);

      shadow_rect.width = width;
      shadow_rect.height = height;
    }
  }

  bis_shadow_helper_size_allocate (self->shadow_helper, shadow_rect.width, shadow_rect.height,
                                   baseline, shadow_rect.x, shadow_rect.y,
                                   shadow_progress, shadow_direction);
}

static void
bis_album_size_allocate (GtkWidget *widget,
                           int        width,
                           int        height,
                           int        baseline)
{
  BisAlbum *self = BIS_ALBUM (widget);
  GtkOrientation orientation = gtk_orientable_get_orientation (GTK_ORIENTABLE (widget));
  GList *directed_children, *children;
  gboolean folded;

  directed_children = get_directed_children (self);

  /* Prepare children information. */
  for (children = directed_children; children; children = children->next) {
    BisAlbumPage *page = children->data;

    gtk_widget_get_preferred_size (page->widget, &page->min, &page->nat);
    page->alloc.x = page->alloc.y = page->alloc.width = page->alloc.height = 0;
    page->visible = FALSE;
  }

  /* Check whether the children should be stacked or not. */
  if (self->can_unfold) {
    int nat_box_size = 0, nat_max_size = 0, min_box_size = 0, min_max_size = 0, visible_children = 0;

    if (orientation == GTK_ORIENTATION_HORIZONTAL) {

      for (children = directed_children; children; children = children->next) {
        BisAlbumPage *page = children->data;

        /* FIXME Check the child is visible. */
        if (!page->widget)
          continue;

        if (page->nat.width <= 0)
          continue;

        nat_box_size += page->nat.width;
        min_box_size += page->min.width;
        nat_max_size = MAX (nat_max_size, page->nat.width);
        min_max_size = MAX (min_max_size, page->min.width);
        visible_children++;
      }

      if (self->fold_threshold_policy == BIS_FOLD_THRESHOLD_POLICY_NATURAL)
        folded = visible_children > 1 && width < nat_box_size;
      else
        folded = visible_children > 1 && width < min_box_size;
    }
    else {
      for (children = directed_children; children; children = children->next) {
        BisAlbumPage *page = children->data;

        /* FIXME Check the child is visible. */
        if (!page->widget)
          continue;

        if (page->nat.height <= 0)
          continue;

        nat_box_size += page->nat.height;
        min_box_size += page->min.height;
        nat_max_size = MAX (nat_max_size, page->nat.height);
        min_max_size = MAX (min_max_size, page->min.height);
        visible_children++;
      }

      if (self->fold_threshold_policy == BIS_FOLD_THRESHOLD_POLICY_NATURAL)
        folded = visible_children > 1 && height < nat_box_size;
      else
        folded = visible_children > 1 && height < min_box_size;
    }
  } else {
    folded = TRUE;
  }

  set_folded (self, folded);

  /* Allocate size to the children. */
  if (folded)
    bis_album_size_allocate_folded (self, width, height);
  else
    bis_album_size_allocate_unfolded (self, width, height);

  /* Apply visibility and allocation. */
  for (children = directed_children; children; children = children->next) {
    BisAlbumPage *page = children->data;

    gtk_widget_set_child_visible (page->widget, page->visible);

    if (!page->visible)
      continue;

    gtk_widget_size_allocate (page->widget, &page->alloc, baseline);

    if (gtk_widget_get_realized (widget))
      gtk_widget_show (page->widget);
  }

  allocate_shadow (self, width, height, baseline);
}

static void
bis_album_snapshot (GtkWidget   *widget,
                      GtkSnapshot *snapshot)
{
  BisAlbum *self = BIS_ALBUM (widget);
  GList *stacked_children, *l;
  BisAlbumPage *overlap_child;
  gboolean is_transition;
  gboolean is_vertical;
  gboolean is_rtl;
  gboolean is_over;
  GdkRectangle shadow_rect;

  overlap_child = get_top_overlap_child (self);

  is_transition = self->child_transition.transition_running ||
                  bis_animation_get_state (self->mode_transition.animation) == BIS_ANIMATION_PLAYING;

  if (!is_transition ||
      self->transition_type == BIS_ALBUM_TRANSITION_TYPE_SLIDE ||
      !overlap_child) {
    GTK_WIDGET_CLASS (bis_album_parent_class)->snapshot (widget, snapshot);

    return;
  }

  stacked_children = self->transition_type == BIS_ALBUM_TRANSITION_TYPE_UNDER ?
                     self->children_reversed : self->children;

  is_vertical = gtk_orientable_get_orientation (GTK_ORIENTABLE (widget)) == GTK_ORIENTATION_VERTICAL;
  is_rtl = gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL;
  is_over = self->transition_type == BIS_ALBUM_TRANSITION_TYPE_OVER;

  shadow_rect.x = 0;
  shadow_rect.y = 0;
  shadow_rect.width = gtk_widget_get_width (widget);
  shadow_rect.height = gtk_widget_get_height (widget);

  if (is_vertical) {
    if (!is_over) {
      shadow_rect.y = overlap_child->alloc.y + overlap_child->alloc.height;
      shadow_rect.height -= shadow_rect.y;
    } else {
      shadow_rect.height = overlap_child->alloc.y;
    }
  } else {
    if (is_over == is_rtl) {
      shadow_rect.x = overlap_child->alloc.x + overlap_child->alloc.width;
      shadow_rect.width -= shadow_rect.x;
    } else {
      shadow_rect.width = overlap_child->alloc.x;
    }
  }

  gtk_snapshot_push_clip (snapshot,
                          &GRAPHENE_RECT_INIT (shadow_rect.x,
                                               shadow_rect.y,
                                               shadow_rect.width,
                                               shadow_rect.height));

  for (l = stacked_children; l; l = l->next) {
    BisAlbumPage *page = l->data;

    if (page == overlap_child) {
      gtk_snapshot_pop (snapshot);

      if (is_vertical) {
        if (!is_over) {
          shadow_rect.height = shadow_rect.y;
          shadow_rect.y = 0;
        } else {
          shadow_rect.y = overlap_child->alloc.y;
          shadow_rect.height = gtk_widget_get_height (widget) - shadow_rect.y;
        }
      } else {
        if (is_over == is_rtl) {
          shadow_rect.width = shadow_rect.x;
          shadow_rect.x = 0;
        } else {
          shadow_rect.x = overlap_child->alloc.x;
          shadow_rect.width = gtk_widget_get_width (widget) - shadow_rect.x;
        }
      }

      gtk_snapshot_push_clip (snapshot,
                              &GRAPHENE_RECT_INIT (shadow_rect.x,
                                                   shadow_rect.y,
                                                   shadow_rect.width,
                                                   shadow_rect.height));
    }

    gtk_widget_snapshot_child (widget, page->widget, snapshot);
  }

  gtk_snapshot_pop (snapshot);

  bis_shadow_helper_snapshot (self->shadow_helper, snapshot);
}

static void
bis_album_direction_changed (GtkWidget        *widget,
                               GtkTextDirection  previous_direction)
{
  update_tracker_orientation (BIS_ALBUM (widget));
}

static void
bis_album_get_property (GObject    *object,
                          guint       prop_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
  BisAlbum *self = BIS_ALBUM (object);

  switch (prop_id) {
  case PROP_CAN_UNFOLD:
    g_value_set_boolean (value, bis_album_get_can_unfold (self));
    break;
  case PROP_FOLDED:
    g_value_set_boolean (value, bis_album_get_folded (self));
    break;
  case PROP_FOLD_THRESHOLD_POLICY:
    g_value_set_enum (value, bis_album_get_fold_threshold_policy (self));
    break;
  case PROP_HOMOGENEOUS:
    g_value_set_boolean (value, bis_album_get_homogeneous (self));
    break;
  case PROP_VISIBLE_CHILD:
    g_value_set_object (value, bis_album_get_visible_child (self));
    break;
  case PROP_VISIBLE_CHILD_NAME:
    g_value_set_string (value, bis_album_get_visible_child_name (self));
    break;
  case PROP_TRANSITION_TYPE:
    g_value_set_enum (value, bis_album_get_transition_type (self));
    break;
  case PROP_MODE_TRANSITION_DURATION:
    g_value_set_uint (value, bis_album_get_mode_transition_duration (self));
    break;
  case PROP_CHILD_TRANSITION_PARAMS:
    g_value_set_boxed (value, bis_album_get_child_transition_params (self));
    break;
  case PROP_CHILD_TRANSITION_RUNNING:
    g_value_set_boolean (value, bis_album_get_child_transition_running (self));
    break;
  case PROP_CAN_NAVIGATE_BACK:
    g_value_set_boolean (value, bis_album_get_can_navigate_back (self));
    break;
  case PROP_CAN_NAVIGATE_FORWARD:
    g_value_set_boolean (value, bis_album_get_can_navigate_forward (self));
    break;
  case PROP_PAGES:
    g_value_take_object (value, bis_album_get_pages (self));
    break;
  case PROP_ORIENTATION:
    g_value_set_enum (value, self->orientation);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
bis_album_set_property (GObject      *object,
                          guint         prop_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  BisAlbum *self = BIS_ALBUM (object);

  switch (prop_id) {
  case PROP_CAN_UNFOLD:
    bis_album_set_can_unfold (self, g_value_get_boolean (value));
    break;
  case PROP_FOLD_THRESHOLD_POLICY:
    bis_album_set_fold_threshold_policy (self, g_value_get_enum (value));
    break;
  case PROP_HOMOGENEOUS:
    bis_album_set_homogeneous (self, g_value_get_boolean (value));
    break;
  case PROP_VISIBLE_CHILD:
    bis_album_set_visible_child (self, g_value_get_object (value));
    break;
  case PROP_VISIBLE_CHILD_NAME:
    bis_album_set_visible_child_name (self, g_value_get_string (value));
    break;
  case PROP_TRANSITION_TYPE:
    bis_album_set_transition_type (self, g_value_get_enum (value));
    break;
  case PROP_MODE_TRANSITION_DURATION:
    bis_album_set_mode_transition_duration (self, g_value_get_uint (value));
    break;
  case PROP_CHILD_TRANSITION_PARAMS:
    bis_album_set_child_transition_params (self, g_value_get_boxed (value));
    break;
  case PROP_CAN_NAVIGATE_BACK:
    bis_album_set_can_navigate_back (self, g_value_get_boolean (value));
    break;
  case PROP_CAN_NAVIGATE_FORWARD:
    bis_album_set_can_navigate_forward (self, g_value_get_boolean (value));
    break;
  case PROP_ORIENTATION:
    set_orientation (self, g_value_get_enum (value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
bis_album_dispose (GObject *object)
{
  BisAlbum *self = BIS_ALBUM (object);
  GtkWidget *child;

  g_clear_object (&self->shadow_helper);

  if (self->pages)
    g_list_model_items_changed (G_LIST_MODEL (self->pages), 0,
                                g_list_length (self->children), 0);

  while ((child = gtk_widget_get_first_child (GTK_WIDGET (self))))
    album_remove (self, child, TRUE);

  g_clear_object (&self->mode_transition.animation);
  g_clear_object (&self->child_transition.animation);

  G_OBJECT_CLASS (bis_album_parent_class)->dispose (object);
}

static void
bis_album_finalize (GObject *object)
{
  BisAlbum *self = BIS_ALBUM (object);

  self->visible_child = NULL;

  if (self->pages)
    g_object_remove_weak_pointer (G_OBJECT (self->pages),
                                  (gpointer *) &self->pages);

  G_OBJECT_CLASS (bis_album_parent_class)->finalize (object);
}

static void
bis_album_class_init (BisAlbumClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->get_property = bis_album_get_property;
  object_class->set_property = bis_album_set_property;
  object_class->dispose = bis_album_dispose;
  object_class->finalize = bis_album_finalize;

  widget_class->measure = bis_album_measure;
  widget_class->size_allocate = bis_album_size_allocate;
  widget_class->snapshot = bis_album_snapshot;
  widget_class->direction_changed = bis_album_direction_changed;
  widget_class->get_request_mode = bis_widget_get_request_mode;
  widget_class->compute_expand = bis_widget_compute_expand;

  g_object_class_override_property (object_class,
                                    PROP_ORIENTATION,
                                    "orientation");

  /**
   * BisAlbum:can-unfold: (attributes org.gtk.Property.get=bis_album_get_can_unfold org.gtk.Property.set=bis_album_set_can_unfold)
   *
   * Whether or not the album can unfold.
   *
   * Since: 1.0
   */
  props[PROP_CAN_UNFOLD] =
    g_param_spec_boolean ("can-unfold", NULL, NULL,
                          TRUE,
                          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BisAlbum:folded: (attributes org.gtk.Property.get=bis_album_get_folded)
   *
   * Whether the album is folded.
   *
   * The album will be folded if the size allocated to it is smaller than the
   * sum of the minimum or natural sizes of the children (see
   * [property@Album:fold-threshold-policy]), it will be unfolded otherwise.
   *
   * Since: 1.0
   */
  props[PROP_FOLDED] =
    g_param_spec_boolean ("folded", NULL, NULL,
                          FALSE,
                          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  /**
   * BisAlbum:fold-threshold-policy: (attributes org.gtk.Property.get=bis_album_get_fold_threshold_policy org.gtk.Property.set=bis_album_set_fold_threshold_policy)
   *
   * Determines when the album will fold.
   *
   * If set to `BIS_FOLD_THRESHOLD_POLICY_MINIMUM`, it will only fold when the
   * children cannot fit anymore. With `BIS_FOLD_THRESHOLD_POLICY_NATURAL`, it
   * will fold as soon as children don't get their natural size.
   *
   * This can be useful if you have a long ellipsizing label and want to let it
   * ellipsize instead of immediately folding.
   *
   * Since: 1.0
   */
  props[PROP_FOLD_THRESHOLD_POLICY] =
    g_param_spec_enum ("fold-threshold-policy", NULL, NULL,
                       BIS_TYPE_FOLD_THRESHOLD_POLICY,
                       BIS_FOLD_THRESHOLD_POLICY_MINIMUM,
                       G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BisAlbum:homogeneous: (attributes org.gtk.Property.get=bis_album_get_homogeneous org.gtk.Property.set=bis_album_set_homogeneous)
   *
   * Whether the album allocates the same size for all children when folded.
   *
   * If set to `FALSE`, different children can have different size along the
   * opposite orientation.
   *
   * Since: 1.0
   */
  props[PROP_HOMOGENEOUS] =
    g_param_spec_boolean ("homogeneous", NULL, NULL,
                          TRUE,
                          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BisAlbum:visible-child: (attributes org.gtk.Property.get=bis_album_get_visible_child org.gtk.Property.set=bis_album_set_visible_child)
   *
   * The widget currently visible when the album is folded.
   *
   * The transition is determined by [property@Album:transition-type] and
   * [property@Album:child-transition-params]. The transition can be cancelled
   * by the user, in which case visible child will change back to the previously
   * visible child.
   *
   * Since: 1.0
   */
  props[PROP_VISIBLE_CHILD] =
    g_param_spec_object ("visible-child", NULL, NULL,
                         GTK_TYPE_WIDGET,
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BisAlbum:visible-child-name: (attributes org.gtk.Property.get=bis_album_get_visible_child_name org.gtk.Property.set=bis_album_set_visible_child_name)
   *
   * The name of the widget currently visible when the album is folded.
   *
   * See [property@Album:visible-child].
   *
   * Since: 1.0
   */
  props[PROP_VISIBLE_CHILD_NAME] =
    g_param_spec_string ("visible-child-name", NULL, NULL,
                         NULL,
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BisAlbum:transition-type: (attributes org.gtk.Property.get=bis_album_get_transition_type org.gtk.Property.set=bis_album_set_transition_type)
   *
   * The type of animation used for transitions between modes and children.
   *
   * The transition type can be changed without problems at runtime, so it is
   * possible to change the animation based on the mode or child that is about
   * to become current.
   *
   * Since: 1.0
   */
  props[PROP_TRANSITION_TYPE] =
    g_param_spec_enum ("transition-type", NULL, NULL,
                       BIS_TYPE_ALBUM_TRANSITION_TYPE, BIS_ALBUM_TRANSITION_TYPE_OVER,
                       G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BisAlbum:mode-transition-duration: (attributes org.gtk.Property.get=bis_album_get_mode_transition_duration org.gtk.Property.set=bis_album_set_mode_transition_duration)
   *
   * The mode transition animation duration, in milliseconds.
   *
   * Since: 1.0
   */
  props[PROP_MODE_TRANSITION_DURATION] =
    g_param_spec_uint ("mode-transition-duration", NULL, NULL,
                       0, G_MAXUINT, 250,
                       G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BisAlbum:child-transition-params: (attributes org.gtk.Property.get=bis_album_get_child_transition_params org.gtk.Property.set=bis_album_set_child_transition_params)
   *
   * The child transition spring parameters.
   *
   * The default value is equivalent to:
   *
   * ```c
   * bis_spring_params_new (1, 0.5, 500)
   * ```
   *
   * Since: 1.0
   */
  props[PROP_CHILD_TRANSITION_PARAMS] =
    g_param_spec_boxed ("child-transition-params", NULL, NULL,
                        BIS_TYPE_SPRING_PARAMS,
                        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BisAlbum:child-transition-running: (attributes org.gtk.Property.get=bis_album_get_child_transition_running)
   *
   * Whether a child transition is currently running.
   *
   * Since: 1.0
   */
  props[PROP_CHILD_TRANSITION_RUNNING] =
    g_param_spec_boolean ("child-transition-running", NULL, NULL,
                          FALSE,
                          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  /**
   * BisAlbum:can-navigate-back: (attributes org.gtk.Property.get=bis_album_get_can_navigate_back org.gtk.Property.set=bis_album_set_can_navigate_back)
   *
   * Whether gestures and shortcuts for navigating backward are enabled.
   *
   * The supported gestures are:
   *
   * - One-finger swipe on touchscreens
   * - Horizontal scrolling on touchpads (usually two-finger swipe)
   * - Back/forward mouse buttons
   *
   * The keyboard back/forward keys are also supported, as well as the
   * <kbd>Alt</kbd>+<kbd>←</kbd> shortcut for horizontal orientation, or
   * <kbd>Alt</kbd>+<kbd>↑</kbd> for vertical orientation.
   *
   * If the orientation is horizontal, for right-to-left locales, gestures and
   * shortcuts are reversed.
   *
   * Only children that have [property@AlbumPage:navigatable] set to `TRUE`
   * can be navigated to.
   *
   * Since: 1.0
   */
  props[PROP_CAN_NAVIGATE_BACK] =
    g_param_spec_boolean ("can-navigate-back", NULL, NULL,
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BisAlbum:can-navigate-forward: (attributes org.gtk.Property.get=bis_album_get_can_navigate_forward org.gtk.Property.set=bis_album_set_can_navigate_forward)
   *
   * Whether gestures and shortcuts for navigating forward are enabled.
   *
   * The supported gestures are:
   *
   * - One-finger swipe on touchscreens
   * - Horizontal scrolling on touchpads (usually two-finger swipe)
   * - Back/forward mouse buttons
   *
   * The keyboard back/forward keys are also supported, as well as the
   * <kbd>Alt</kbd>+<kbd>→</kbd> shortcut for horizontal orientation, or
   * <kbd>Alt</kbd>+<kbd>↓</kbd> for vertical orientation.
   *
   * If the orientation is horizontal, for right-to-left locales, gestures and
   * shortcuts are reversed.
   *
   * Only children that have [property@AlbumPage:navigatable] set to `TRUE`
   * can be navigated to.
   *
   * Since: 1.0
   */
  props[PROP_CAN_NAVIGATE_FORWARD] =
    g_param_spec_boolean ("can-navigate-forward", NULL, NULL,
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BisAlbum:pages: (attributes org.gtk.Property.get=bis_album_get_pages)
   *
   * A selection model with the album's pages.
   *
   * This can be used to keep an up-to-date view. The model also implements
   * [iface@Gtk.SelectionModel] and can be used to track and change the visible
   * page.
   *
   * Since: 1.0
   */
 props[PROP_PAGES] =
    g_param_spec_object ("pages", NULL, NULL,
                         GTK_TYPE_SELECTION_MODEL,
                         G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, LAST_PROP, props);

  gtk_widget_class_set_css_name (widget_class, "album");

  gtk_widget_class_add_binding (widget_class, GDK_KEY_Back, 0,
                                (GtkShortcutFunc) back_forward_shortcut_cb,
                                "h", BIS_NAVIGATION_DIRECTION_BACK);
  gtk_widget_class_add_binding (widget_class, GDK_KEY_Forward, 0,
                                (GtkShortcutFunc) back_forward_shortcut_cb,
                                "h", BIS_NAVIGATION_DIRECTION_FORWARD);

  gtk_widget_class_add_binding (widget_class, GDK_KEY_Left, GDK_ALT_MASK,
                                (GtkShortcutFunc) alt_arrows_shortcut_cb,
                                "(hh)", BIS_NAVIGATION_DIRECTION_BACK,
                                GTK_ORIENTATION_HORIZONTAL);
  gtk_widget_class_add_binding (widget_class,  GDK_KEY_Right, GDK_ALT_MASK,
                                (GtkShortcutFunc) alt_arrows_shortcut_cb,
                                "(hh)", BIS_NAVIGATION_DIRECTION_FORWARD,
                                GTK_ORIENTATION_HORIZONTAL);

  gtk_widget_class_add_binding (widget_class, GDK_KEY_Up, GDK_ALT_MASK,
                                (GtkShortcutFunc) alt_arrows_shortcut_cb,
                                "(hh)", BIS_NAVIGATION_DIRECTION_BACK,
                                GTK_ORIENTATION_VERTICAL);
  gtk_widget_class_add_binding (widget_class,  GDK_KEY_Down, GDK_ALT_MASK,
                                (GtkShortcutFunc) alt_arrows_shortcut_cb,
                                "(hh)", BIS_NAVIGATION_DIRECTION_FORWARD,
                                GTK_ORIENTATION_VERTICAL);
}

static void
bis_album_init (BisAlbum *self)
{
  GtkWidget *widget = GTK_WIDGET (self);
  GtkEventController *controller;
  BisAnimationTarget *target;

  gtk_widget_set_overflow (GTK_WIDGET (self), GTK_OVERFLOW_HIDDEN);

  self->children = NULL;
  self->children_reversed = NULL;
  self->visible_child = NULL;
  self->folded = FALSE;
  self->fold_threshold_policy = BIS_FOLD_THRESHOLD_POLICY_MINIMUM;
  self->homogeneous = TRUE;
  self->transition_type = BIS_ALBUM_TRANSITION_TYPE_OVER;
  self->mode_transition.duration = 250;
  self->mode_transition.current_pos = 1.0;
  self->can_unfold = TRUE;

  controller = GTK_EVENT_CONTROLLER (gtk_gesture_click_new ());
  gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (controller), 0);
  g_signal_connect_object (controller, "pressed", G_CALLBACK (back_forward_button_pressed_cb), self, 0);
  gtk_widget_add_controller (widget, controller);

  self->tracker = bis_swipe_tracker_new (BIS_SWIPEABLE (self));

  g_object_set (self->tracker, "orientation", self->orientation, "enabled", FALSE, NULL);

  g_signal_connect_object (self->tracker, "prepare", G_CALLBACK (prepare_cb), self, 0);
  g_signal_connect_object (self->tracker, "update-swipe", G_CALLBACK (update_swipe_cb), self, 0);
  g_signal_connect_object (self->tracker, "end-swipe", G_CALLBACK (end_swipe_cb), self, 0);

  self->shadow_helper = bis_shadow_helper_new (widget);

  gtk_widget_add_css_class (widget, "unfolded");

  target = bis_callback_animation_target_new ((BisAnimationTargetFunc) mode_transition_cb,
                                              self, NULL);
  self->mode_transition.animation =
    bis_timed_animation_new (GTK_WIDGET (self), 0, 1,
                             self->mode_transition.duration, target);

  target = bis_callback_animation_target_new ((BisAnimationTargetFunc) child_transition_cb,
                                              self, NULL);
  self->child_transition.animation =
    bis_spring_animation_new (GTK_WIDGET (self), 0, 1,
                              bis_spring_params_new (1, 0.5, 500), target);
  bis_spring_animation_set_clamp (BIS_SPRING_ANIMATION (self->child_transition.animation),
                                  TRUE);
  g_signal_connect_swapped (self->child_transition.animation, "done",
                            G_CALLBACK (child_transition_done_cb), self);
}

static void
bis_album_buildable_add_child (GtkBuildable *buildable,
                                 GtkBuilder   *builder,
                                 GObject      *child,
                                 const char   *type)
{
  BisAlbum *self = BIS_ALBUM (buildable);

  if (BIS_IS_ALBUM_PAGE (child))
    add_page (self, BIS_ALBUM_PAGE (child),
              self->children ? g_list_last (self->children)->data : NULL);
  else if (GTK_IS_WIDGET (child))
    bis_album_append (self, GTK_WIDGET (child));
  else
    parent_buildable_iface->add_child (buildable, builder, child, type);
}

static void
bis_album_buildable_init (GtkBuildableIface *iface)
{
  parent_buildable_iface = g_type_interface_peek_parent (iface);

  iface->add_child = bis_album_buildable_add_child;
}

static double
bis_album_get_distance (BisSwipeable *swipeable)
{
  BisAlbum *self = BIS_ALBUM (swipeable);

  if (self->orientation == GTK_ORIENTATION_HORIZONTAL)
    return gtk_widget_get_width (GTK_WIDGET (self));
  else
    return gtk_widget_get_height (GTK_WIDGET (self));
}

static double *
bis_album_get_snap_points (BisSwipeable *swipeable,
                             int          *n_snap_points)
{
  BisAlbum *self = BIS_ALBUM (swipeable);
  int n;
  double *points, lower, upper;

  if (self->child_transition.transition_running) {
    int current_direction;
    gboolean is_rtl = gtk_widget_get_direction (GTK_WIDGET (self)) == GTK_TEXT_DIR_RTL;

    switch (self->child_transition.active_direction) {
    case GTK_PAN_DIRECTION_UP:
      current_direction = 1;
      break;
    case GTK_PAN_DIRECTION_DOWN:
      current_direction = -1;
      break;
    case GTK_PAN_DIRECTION_LEFT:
      current_direction = is_rtl ? -1 : 1;
      break;
    case GTK_PAN_DIRECTION_RIGHT:
      current_direction = is_rtl ? 1 : -1;
      break;
    default:
      g_assert_not_reached ();
    }

    lower = MIN (0, current_direction);
    upper = MAX (0, current_direction);
  } else {
    BisAlbumPage *page = NULL;

    if (can_navigate_in_direction (self, self->child_transition.swipe_direction) && self->folded)
      page = find_swipeable_page (self, self->child_transition.swipe_direction);

    lower = MIN (0, page ? self->child_transition.swipe_direction : 0);
    upper = MAX (0, page ? self->child_transition.swipe_direction : 0);
  }

  n = (lower != upper) ? 2 : 1;

  points = g_new0 (double, n);
  points[0] = lower;
  points[n - 1] = upper;

  if (n_snap_points)
    *n_snap_points = n;

  return points;
}

static double
bis_album_get_progress (BisSwipeable *swipeable)
{
  BisAlbum *self = BIS_ALBUM (swipeable);
  gboolean new_first = FALSE;
  GList *children;

  if (!self->child_transition.transition_running)
    return 0;

  for (children = self->children; children; children = children->next) {
    if (self->last_visible_child == children->data) {
      new_first = TRUE;

      break;
    }
    if (self->visible_child == children->data)
      break;
  }

  return self->child_transition.progress * (new_first ? 1 : -1);
}

static double
bis_album_get_cancel_progress (BisSwipeable *swipeable)
{
  return 0;
}

static void
bis_album_get_swipe_area (BisSwipeable           *swipeable,
                            BisNavigationDirection  navigation_direction,
                            gboolean                is_drag,
                            GdkRectangle           *rect)
{
  BisAlbum *self = BIS_ALBUM (swipeable);
  int width = gtk_widget_get_width (GTK_WIDGET (self));
  int height = gtk_widget_get_height (GTK_WIDGET (self));
  double progress = 0;

  rect->x = 0;
  rect->y = 0;
  rect->width = width;
  rect->height = height;

  if (!is_drag)
    return;

  if (self->transition_type == BIS_ALBUM_TRANSITION_TYPE_SLIDE)
    return;

  if (self->child_transition.transition_running)
    progress = self->child_transition.progress;

  if (self->orientation == GTK_ORIENTATION_HORIZONTAL) {
    gboolean is_rtl = gtk_widget_get_direction (GTK_WIDGET (self)) == GTK_TEXT_DIR_RTL;

    if (self->transition_type == BIS_ALBUM_TRANSITION_TYPE_OVER &&
         navigation_direction == BIS_NAVIGATION_DIRECTION_FORWARD) {
      rect->width = MAX (progress * width, BIS_SWIPE_BORDER);
      rect->x = is_rtl ? 0 : width - rect->width;
    } else if (self->transition_type == BIS_ALBUM_TRANSITION_TYPE_UNDER &&
               navigation_direction == BIS_NAVIGATION_DIRECTION_BACK) {
      rect->width = MAX (progress * width, BIS_SWIPE_BORDER);
      rect->x = is_rtl ? width - rect->width : 0;
    }
  } else {
    if (self->transition_type == BIS_ALBUM_TRANSITION_TYPE_OVER &&
        navigation_direction == BIS_NAVIGATION_DIRECTION_FORWARD) {
      rect->height = MAX (progress * height, BIS_SWIPE_BORDER);
      rect->y = height - rect->height;
    } else if (self->transition_type == BIS_ALBUM_TRANSITION_TYPE_UNDER &&
               navigation_direction == BIS_NAVIGATION_DIRECTION_BACK) {
      rect->height = MAX (progress * height, BIS_SWIPE_BORDER);
      rect->y = 0;
    }
  }
}

static void
bis_album_swipeable_init (BisSwipeableInterface *iface)
{
  iface->get_distance = bis_album_get_distance;
  iface->get_snap_points = bis_album_get_snap_points;
  iface->get_progress = bis_album_get_progress;
  iface->get_cancel_progress = bis_album_get_cancel_progress;
  iface->get_swipe_area = bis_album_get_swipe_area;
}

/**
 * bis_album_page_get_child: (attributes org.gtk.Method.get_property=child)
 * @self: a album page
 *
 * Gets the album child to which @self belongs.
 *
 * Returns: (transfer none): the child to which @self belongs
 *
 * Since: 1.0
 */
GtkWidget *
bis_album_page_get_child (BisAlbumPage *self)
{
  g_return_val_if_fail (BIS_IS_ALBUM_PAGE (self), NULL);

  return self->widget;
}

/**
 * bis_album_page_get_name: (attributes org.gtk.Method.get_property=name)
 * @self: a album page
 *
 * Gets the name of @self.
 *
 * Returns: (nullable): the name of @self.
 *
 * Since: 1.0
 */
const char *
bis_album_page_get_name (BisAlbumPage *self)
{
  g_return_val_if_fail (BIS_IS_ALBUM_PAGE (self), NULL);

  return self->name;
}

/**
 * bis_album_page_set_name: (attributes org.gtk.Method.set_property=name)
 * @self: a album page
 * @name: (nullable): the new value to set
 *
 * Sets the name of the @self.
 *
 * Since: 1.0
 */
void
bis_album_page_set_name (BisAlbumPage *self,
                           const char     *name)
{
  BisAlbum *album = NULL;

  g_return_if_fail (BIS_IS_ALBUM_PAGE (self));

  if (self->widget &&
    gtk_widget_get_parent (self->widget) &&
    BIS_IS_ALBUM (gtk_widget_get_parent (self->widget))) {
    GList *l;

    album = BIS_ALBUM (gtk_widget_get_parent (self->widget));

    for (l = album->children; l; l = l->next) {
      BisAlbumPage *page = l->data;

      if (self == page)
        continue;

      if (g_strcmp0 (page->name, name) == 0)
        {
          g_warning ("Duplicate child name in BisAlbum: %s", name);
          break;
        }
    }
  }

  if (name == self->name)
    return;

  g_free (self->name);
  self->name = g_strdup (name);
  g_object_notify_by_pspec (G_OBJECT (self), page_props[PAGE_PROP_NAME]);

  if (album && album->visible_child == self)
    g_object_notify_by_pspec (G_OBJECT (album), props[PROP_VISIBLE_CHILD_NAME]);
}

/**
 * bis_album_page_get_navigatable: (attributes org.gtk.Method.get_property=navigatable)
 * @self: a album page
 *
 * Gets whether the child can be navigated to when folded.
 *
 * Returns: whether @self can be navigated to when folded
 *
 * Since: 1.0
 */
gboolean
bis_album_page_get_navigatable (BisAlbumPage *self)
{
  g_return_val_if_fail (BIS_IS_ALBUM_PAGE (self), FALSE);

  return self->navigatable;
}

/**
 * bis_album_page_set_navigatable: (attributes org.gtk.Method.set_property=navigatable)
 * @self: a album page
 * @navigatable: whether @self can be navigated to when folded
 *
 * Sets whether @self can be navigated to when folded.
 *
 * If `FALSE`, the child will be ignored by [method@Album.get_adjacent_child],
 * [method@Album.navigate], and swipe gestures.
 *
 * This can be used used to prevent switching to widgets like separators.
 *
 * Since: 1.0
 */
void
bis_album_page_set_navigatable (BisAlbumPage *self,
                                  gboolean        navigatable)
{
  g_return_if_fail (BIS_IS_ALBUM_PAGE (self));

  navigatable = !!navigatable;

  if (navigatable == self->navigatable)
    return;

  self->navigatable = navigatable;

  if (self->widget && gtk_widget_get_parent (self->widget)) {
    BisAlbum *album = BIS_ALBUM (gtk_widget_get_parent (self->widget));

    if (self == album->visible_child)
      set_visible_child (album, NULL);
  }

  g_object_notify_by_pspec (G_OBJECT (self), page_props[PAGE_PROP_NAVIGATABLE]);
}

/**
 * bis_album_new:
 *
 * Creates a new `BisAlbum`.
 *
 * Returns: the new created `BisAlbum`
 *
 * Since: 1.0
 */
GtkWidget *
bis_album_new (void)
{
  return g_object_new (BIS_TYPE_ALBUM, NULL);
}

/**
 * bis_album_append:
 * @self: a album
 * @child: the widget to add
 *
 * Adds a child to @self.
 *
 * Returns: (transfer none): the [class@AlbumPage] for @child
 *
 * Since: 1.0
 */
BisAlbumPage *
bis_album_append (BisAlbum *self,
                    GtkWidget  *child)
{
  GtkWidget *sibling;

  g_return_val_if_fail (BIS_IS_ALBUM (self), NULL);
  g_return_val_if_fail (GTK_IS_WIDGET (child), NULL);
  g_return_val_if_fail (gtk_widget_get_parent (child) == NULL, NULL);

  if (self->children)
    sibling = bis_album_page_get_child (g_list_last (self->children)->data);
  else
    sibling = NULL;

  return bis_album_insert_child_after (self, child, sibling);
}

/**
 * bis_album_prepend:
 * @self: a album
 * @child: the widget to prepend
 *
 * Inserts @child at the first position in @self.
 *
 * Returns: (transfer none): the [class@AlbumPage] for @child
 *
 * Since: 1.0
 */
BisAlbumPage *
bis_album_prepend (BisAlbum *self,
                     GtkWidget  *child)
{
  g_return_val_if_fail (BIS_IS_ALBUM (self), NULL);
  g_return_val_if_fail (GTK_IS_WIDGET (child), NULL);
  g_return_val_if_fail (gtk_widget_get_parent (child) == NULL, NULL);

  return bis_album_insert_child_after (self, child, NULL);
}

/**
 * bis_album_insert_child_after:
 * @self: a album
 * @child: the widget to insert
 * @sibling: (nullable): the sibling after which to insert @child
 *
 * Inserts @child in the position after @sibling in the list of children.
 *
 * If @sibling is `NULL`, inserts @child at the first position.
 *
 * Returns: (transfer none): the [class@AlbumPage] for @child
 *
 * Since: 1.0
 */
BisAlbumPage *
bis_album_insert_child_after (BisAlbum *self,
                                GtkWidget  *child,
                                GtkWidget  *sibling)
{
  BisAlbumPage *page;

  g_return_val_if_fail (BIS_IS_ALBUM (self), NULL);
  g_return_val_if_fail (GTK_IS_WIDGET (child), NULL);
  g_return_val_if_fail (sibling == NULL || GTK_IS_WIDGET (sibling), NULL);

  g_return_val_if_fail (gtk_widget_get_parent (child) == NULL, NULL);
  g_return_val_if_fail (sibling == NULL || gtk_widget_get_parent (sibling) == GTK_WIDGET (self), NULL);

  page = g_object_new (BIS_TYPE_ALBUM_PAGE, NULL);
  page->widget = g_object_ref (child);

  add_page (self, page, find_page_for_widget (self, sibling));

  g_object_unref (page);

  return page;

}

/**
 * bis_album_reorder_child_after:
 * @self: a album
 * @child: the widget to move, must be a child of @self
 * @sibling: (nullable): the sibling to move @child after
 *
 * Moves @child to the position after @sibling in the list of children.
 *
 * If @sibling is `NULL`, moves @child to the first position.
 *
 * Since: 1.0
 */
void
bis_album_reorder_child_after (BisAlbum *self,
                                 GtkWidget  *child,
                                 GtkWidget  *sibling)
{
  BisAlbumPage *child_page;
  BisAlbumPage *sibling_page;
  int sibling_page_pos;
  int previous_position;

  g_return_if_fail (BIS_IS_ALBUM (self));
  g_return_if_fail (GTK_IS_WIDGET (child));
  g_return_if_fail (sibling == NULL || GTK_IS_WIDGET (sibling));

  g_return_if_fail (gtk_widget_get_parent (child) == GTK_WIDGET (self));
  g_return_if_fail (sibling == NULL || gtk_widget_get_parent (sibling) == GTK_WIDGET (self));

  if (child == sibling)
    return;

  previous_position = g_list_index (self->children, child) - 1;

  /* Cancel a gesture if there's one in progress */
  bis_swipe_tracker_reset (self->tracker);

  child_page = find_page_for_widget (self, child);
  self->children = g_list_remove (self->children, child_page);
  self->children_reversed = g_list_remove (self->children_reversed, child_page);

  sibling_page = find_page_for_widget (self, sibling);
  sibling_page_pos = g_list_index (self->children, sibling_page);

  self->children =
    g_list_insert (self->children, child_page,
                   sibling_page_pos + 1);
  self->children_reversed =
    g_list_insert (self->children_reversed, child_page,
                   g_list_length (self->children) - sibling_page_pos - 1);

  if (self->pages) {
    /* Copied from gtk_list_list_model_item_moved() */
    guint position = g_list_index (self->children, child_page);
    guint min, max;

    if (previous_position < 0)
      previous_position = 0;
    else if (position > previous_position)
      previous_position++;

    if (position == previous_position)
      return;

    min = MIN (position, previous_position);
    max = MAX (position, previous_position) + 1;
    g_list_model_items_changed (G_LIST_MODEL (self->pages), min, max - min, max - min);
  }
}

/**
 * bis_album_remove:
 * @self: a album
 * @child: the child to remove
 *
 * Removes a child widget from @self.
 *
 * Since: 1.0
 */
void
bis_album_remove (BisAlbum *self,
                    GtkWidget  *child)
{
  GList *l;
  guint position;

  g_return_if_fail (BIS_IS_ALBUM (self));
  g_return_if_fail (GTK_IS_WIDGET (child));
  g_return_if_fail (gtk_widget_get_parent (child) == GTK_WIDGET (self));

  for (l = self->children, position = 0; l; l = l->next, position++) {
    BisAlbumPage *page = l->data;

    if (page->widget == child)
      break;
  }

  album_remove (self, child, FALSE);

  if (self->pages)
    g_list_model_items_changed (G_LIST_MODEL (self->pages), position, 1, 0);
}

/**
 * bis_album_get_page:
 * @self: a album
 * @child: a child of @self
 *
 * Returns the [class@AlbumPage] object for @child.
 *
 * Returns: (transfer none): the page object for @child
 *
 * Since: 1.0
 */
BisAlbumPage *
bis_album_get_page (BisAlbum *self,
                      GtkWidget  *child)
{
  g_return_val_if_fail (BIS_IS_ALBUM (self), NULL);
  g_return_val_if_fail (GTK_IS_WIDGET (child), NULL);

  return find_page_for_widget (self, child);
}

/**
 * bis_album_set_can_unfold: (attributes org.gtk.Method.set_property=can-unfold)
 * @self: a album
 * @can_unfold: whether @self can unfold
 *
 * Sets whether @self can unfold.
 *
 * Since: 1.0
 */
void
bis_album_set_can_unfold (BisAlbum *self,
                            gboolean    can_unfold)
{
  g_return_if_fail (BIS_IS_ALBUM (self));

  can_unfold = !!can_unfold;

  if (self->can_unfold == can_unfold)
    return;

  self->can_unfold = can_unfold;

  gtk_widget_queue_allocate (GTK_WIDGET (self));

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_CAN_UNFOLD]);
}

/**
 * bis_album_get_can_unfold: (attributes org.gtk.Method.get_property=can-unfold)
 * @self: a album
 *
 * Gets whether @self can unfold.
 *
 * Returns: whether @self can unfold
 *
 * Since: 1.0
 */
gboolean
bis_album_get_can_unfold (BisAlbum *self)
{
  g_return_val_if_fail (BIS_IS_ALBUM (self), FALSE);

  return self->can_unfold;
}

/**
 * bis_album_get_folded: (attributes org.gtk.Method.get_property=folded)
 * @self: a album
 *
 * Gets whether @self is folded.
 *
 * The album will be folded if the size allocated to it is smaller than the
 * sum of the minimum or natural sizes of the children (see
 * [property@Album:fold-threshold-policy]), it will be unfolded otherwise.
 *
 * Returns: whether @self is folded.
 *
 * Since: 1.0
 */
gboolean
bis_album_get_folded (BisAlbum *self)
{
  g_return_val_if_fail (BIS_IS_ALBUM (self), FALSE);

  return self->folded;
}

/**
 * bis_album_get_fold_threshold_policy: (attributes org.gtk.Method.get_property=fold-threshold-policy)
 * @self: a album
 *
 * Gets the fold threshold policy for @self.
 *
 * Since: 1.0
 */
BisFoldThresholdPolicy
bis_album_get_fold_threshold_policy (BisAlbum *self)
{
  g_return_val_if_fail (BIS_IS_ALBUM (self), BIS_FOLD_THRESHOLD_POLICY_MINIMUM);

  return self->fold_threshold_policy;
}


/**
 * bis_album_set_fold_threshold_policy: (attributes org.gtk.Method.set_property=fold-threshold-policy)
 * @self: a album
 * @policy: the policy to use
 *
 * Sets the fold threshold policy for @self.
 *
 * If set to `BIS_FOLD_THRESHOLD_POLICY_MINIMUM`, it will only fold when the
 * children cannot fit anymore. With `BIS_FOLD_THRESHOLD_POLICY_NATURAL`, it
 * will fold as soon as children don't get their natural size.
 *
 * This can be useful if you have a long ellipsizing label and want to let it
 * ellipsize instead of immediately folding.
 *
 * Since: 1.0
 */
void
bis_album_set_fold_threshold_policy (BisAlbum             *self,
                                       BisFoldThresholdPolicy  policy)
{
  g_return_if_fail (BIS_IS_ALBUM (self));
  g_return_if_fail (policy <= BIS_FOLD_THRESHOLD_POLICY_NATURAL);

  if (self->fold_threshold_policy == policy)
    return;

  self->fold_threshold_policy = policy;

  gtk_widget_queue_allocate (GTK_WIDGET (self));

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_FOLD_THRESHOLD_POLICY]);
}

/**
 * bis_album_get_homogeneous: (attributes org.gtk.Method.get_property=homogeneous)
 * @self: a album
 *
 * Gets whether @self is homogeneous.
 *
 * Returns: whether @self is homogeneous
 *
 * Since: 1.0
 */
gboolean
bis_album_get_homogeneous (BisAlbum *self)
{
  g_return_val_if_fail (BIS_IS_ALBUM (self), FALSE);

  return self->homogeneous;
}

/**
 * bis_album_set_homogeneous: (attributes org.gtk.Method.set_property=homogeneous)
 * @self: a album
 * @homogeneous: whether to make @self homogeneous
 *
 * Sets @self to be homogeneous or not.
 *
 * If set to `FALSE`, different children can have different size along the
 * opposite orientation.
 *
 * Since: 1.0
 */
void
bis_album_set_homogeneous (BisAlbum *self,
                             gboolean    homogeneous)
{
  g_return_if_fail (BIS_IS_ALBUM (self));

  homogeneous = !!homogeneous;

  if (self->homogeneous == homogeneous)
    return;

  self->homogeneous = homogeneous;

  gtk_widget_queue_resize (GTK_WIDGET (self));

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_HOMOGENEOUS]);
}

/**
 * bis_album_get_visible_child: (attributes org.gtk.Method.get_property=visible-child)
 * @self: a album
 *
 * Gets the widget currently visible when the album is folded.
 *
 * Returns: (nullable) (transfer none): the visible child
 *
 * Since: 1.0
 */
GtkWidget *
bis_album_get_visible_child (BisAlbum *self)
{
  g_return_val_if_fail (BIS_IS_ALBUM (self), NULL);

  if (self->visible_child == NULL)
    return NULL;

  return self->visible_child->widget;
}

/**
 * bis_album_set_visible_child: (attributes org.gtk.Method.set_property=visible-child)
 * @self: a album
 * @visible_child: the new child
 *
 * Sets the widget currently visible when the album is folded.
 *
 * The transition is determined by [property@Album:transition-type] and
 * [property@Album:child-transition-params]. The transition can be cancelled
 * by the user, in which case visible child will change back to the previously
 * visible child.
 *
 * Since: 1.0
 */
void
bis_album_set_visible_child (BisAlbum *self,
                               GtkWidget  *visible_child)
{
  BisAlbumPage *page;
  gboolean contains_child;

  g_return_if_fail (BIS_IS_ALBUM (self));
  g_return_if_fail (GTK_IS_WIDGET (visible_child));

  page = find_page_for_widget (self, visible_child);

  contains_child = page != NULL;

  g_return_if_fail (contains_child);

  set_visible_child (self, page);
}

/**
 * bis_album_get_visible_child_name: (attributes org.gtk.Method.get_property=visible-child-name)
 * @self: a album
 *
 * Gets the name of the currently visible child widget.
 *
 * Returns: (nullable) (transfer none): the name of the visible child
 *
 * Since: 1.0
 */
const char *
bis_album_get_visible_child_name (BisAlbum *self)
{
  g_return_val_if_fail (BIS_IS_ALBUM (self), NULL);

  if (self->visible_child == NULL)
    return NULL;

  return self->visible_child->name;
}

/**
 * bis_album_set_visible_child_name: (attributes org.gtk.Method.set_property=visible-child-name)
 * @self: a album
 * @name: the name of a child
 *
 * Makes the child with the name @name visible.
 *
 * See [property@Album:visible-child].
 *
 * Since: 1.0
 */
void
bis_album_set_visible_child_name (BisAlbum *self,
                                    const char *name)
{
  BisAlbumPage *page;
  gboolean contains_child;

  g_return_if_fail (BIS_IS_ALBUM (self));
  g_return_if_fail (name != NULL);

  page = find_page_for_name (self, name);
  contains_child = page != NULL;

  g_return_if_fail (contains_child);

  set_visible_child (self, page);
}

/**
 * bis_album_get_transition_type: (attributes org.gtk.Method.get_property=transition-type)
 * @self: a album
 *
 * Gets the type of animation used for transitions between modes and children.
 *
 * Returns: the current transition type of @self
 *
 * Since: 1.0
 */
BisAlbumTransitionType
bis_album_get_transition_type (BisAlbum *self)
{
  g_return_val_if_fail (BIS_IS_ALBUM (self), BIS_ALBUM_TRANSITION_TYPE_OVER);

  return self->transition_type;
}

/**
 * bis_album_set_transition_type: (attributes org.gtk.Method.set_property=transition-type)
 * @self: a album
 * @transition: the new transition type
 *
 * Sets the type of animation used for transitions between modes and children.
 *
 * The transition type can be changed without problems at runtime, so it is
 * possible to change the animation based on the mode or child that is about to
 * become current.
 *
 * Since: 1.0
 */
void
bis_album_set_transition_type (BisAlbum               *self,
                                 BisAlbumTransitionType  transition)
{
  GList *l;

  g_return_if_fail (BIS_IS_ALBUM (self));
  g_return_if_fail (transition <= BIS_ALBUM_TRANSITION_TYPE_SLIDE);

  if (self->transition_type == transition)
    return;

  self->transition_type = transition;

  for (l = self->children; l; l = l->next) {
    BisAlbumPage *page = l->data;

    if (self->transition_type == BIS_ALBUM_TRANSITION_TYPE_OVER)
      gtk_widget_insert_before (page->widget, GTK_WIDGET (self), NULL);
    else
      gtk_widget_insert_after (page->widget, GTK_WIDGET (self), NULL);
  }

  g_object_notify_by_pspec (G_OBJECT (self),
                            props[PROP_TRANSITION_TYPE]);
}

/**
 * bis_album_get_mode_transition_duration: (attributes org.gtk.Method.get_property=mode-transition-duration)
 * @self: a album
 *
 * Gets the mode transition animation duration for @self.
 *
 * Returns: the mode transition duration, in milliseconds.
 *
 * Since: 1.0
 */
guint
bis_album_get_mode_transition_duration (BisAlbum *self)
{
  g_return_val_if_fail (BIS_IS_ALBUM (self), 0);

  return self->mode_transition.duration;
}

/**
 * bis_album_set_mode_transition_duration: (attributes org.gtk.Method.set_property=mode-transition-duration)
 * @self: a album
 * @duration: the new duration, in milliseconds
 *
 * Sets the mode transition animation duration for @self.
 *
 * Since: 1.0
 */
void
bis_album_set_mode_transition_duration (BisAlbum *self,
                                          guint       duration)
{
  g_return_if_fail (BIS_IS_ALBUM (self));

  if (self->mode_transition.duration == duration)
    return;

  self->mode_transition.duration = duration;

  bis_timed_animation_set_duration (BIS_TIMED_ANIMATION (self->mode_transition.animation),
                                    duration);

  g_object_notify_by_pspec (G_OBJECT (self),
                            props[PROP_MODE_TRANSITION_DURATION]);
}

/**
 * bis_album_get_child_transition_params: (attributes org.gtk.Method.get_property=child-transition-params)
 * @self: a album
 *
 * Gets the child transition spring parameters for @self.
 *
 * Returns: the child transition parameters
 *
 * Since: 1.0
 */
BisSpringParams *
bis_album_get_child_transition_params (BisAlbum *self)
{
  g_return_val_if_fail (BIS_IS_ALBUM (self), NULL);

  return bis_spring_animation_get_spring_params (BIS_SPRING_ANIMATION (self->child_transition.animation));
}

/**
 * bis_album_set_child_transition_params: (attributes org.gtk.Method.set_property=child-transition-params)
 * @self: a album
 * @params: the new parameters
 *
 * Sets the child transition spring parameters for @self.
 *
 * The default value is equivalent to:
 *
 * ```c
 * bis_spring_params_new (1, 0.5, 500)
 * ```
 *
 * Since: 1.0
 */
void
bis_album_set_child_transition_params (BisAlbum      *self,
                                         BisSpringParams *params)
{
  g_return_if_fail (BIS_IS_ALBUM (self));
  g_return_if_fail (params != NULL);

  if (bis_album_get_child_transition_params (self) == params)
    return;

  bis_spring_animation_set_spring_params (BIS_SPRING_ANIMATION (self->child_transition.animation),
                                          params);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_CHILD_TRANSITION_PARAMS]);
}

/**
 * bis_album_get_child_transition_running: (attributes org.gtk.Method.get_property=child-transition-running)
 * @self: a album
 *
 * Gets whether a child transition is currently running for @self.
 *
 * Returns: whether a transition is currently running
 *
 * Since: 1.0
 */
gboolean
bis_album_get_child_transition_running (BisAlbum *self)
{
  g_return_val_if_fail (BIS_IS_ALBUM (self), FALSE);

  return self->child_transition.transition_running;
}

/**
 * bis_album_get_can_navigate_back: (attributes org.gtk.Method.get_property=can-navigate-back)
 * @self: a album
 *
 * Gets whether gestures and shortcuts for navigating backward are enabled.
 *
 * Returns: Whether gestures and shortcuts are enabled.
 *
 * Since: 1.0
 */
gboolean
bis_album_get_can_navigate_back (BisAlbum *self)
{
  g_return_val_if_fail (BIS_IS_ALBUM (self), FALSE);

  return self->child_transition.can_navigate_back;
}

/**
 * bis_album_set_can_navigate_back: (attributes org.gtk.Method.set_property=can-navigate-back)
 * @self: a album
 * @can_navigate_back: the new value
 *
 * Sets whether gestures and shortcuts for navigating backward are enabled.
 *
 * The supported gestures are:
 *
 * - One-finger swipe on touchscreens
 * - Horizontal scrolling on touchpads (usually two-finger swipe)
 * - Back/forward mouse buttons
 *
 * The keyboard back/forward keys are also supported, as well as the
 * <kbd>Alt</kbd>+<kbd>←</kbd> shortcut for horizontal orientation, or
 * <kbd>Alt</kbd>+<kbd>↑</kbd> for vertical orientation.
 *
 * If the orientation is horizontal, for right-to-left locales, gestures and
 * shortcuts are reversed.
 *
 * Only children that have [property@AlbumPage:navigatable] set to `TRUE` can
 * be navigated to.
 *
 * Since: 1.0
 */
void
bis_album_set_can_navigate_back (BisAlbum *self,
                                   gboolean    can_navigate_back)
{
  g_return_if_fail (BIS_IS_ALBUM (self));

  can_navigate_back = !!can_navigate_back;

  if (self->child_transition.can_navigate_back == can_navigate_back)
    return;

  self->child_transition.can_navigate_back = can_navigate_back;
  bis_swipe_tracker_set_enabled (self->tracker, can_navigate_back || self->child_transition.can_navigate_forward);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_CAN_NAVIGATE_BACK]);
}

/**
 * bis_album_get_can_navigate_forward: (attributes org.gtk.Method.get_property=can-navigate-forward)
 * @self: a album
 *
 * Gets whether gestures and shortcuts for navigating forward are enabled.
 *
 * Returns: Whether gestures and shortcuts are enabled.
 *
 * Since: 1.0
 */
gboolean
bis_album_get_can_navigate_forward (BisAlbum *self)
{
  g_return_val_if_fail (BIS_IS_ALBUM (self), FALSE);

  return self->child_transition.can_navigate_forward;
}

/**
 * bis_album_set_can_navigate_forward: (attributes org.gtk.Method.set_property=can-navigate-forward)
 * @self: a album
 * @can_navigate_forward: the new value
 *
 * Sets whether gestures and shortcuts for navigating forward are enabled.
 *
 * The supported gestures are:
 *
 * - One-finger swipe on touchscreens
 * - Horizontal scrolling on touchpads (usually two-finger swipe)
 * - Back/forward mouse buttons
 *
 * The keyboard back/forward keys are also supported, as well as the
 * <kbd>Alt</kbd>+<kbd>→</kbd> shortcut for horizontal orientation, or
 * <kbd>Alt</kbd>+<kbd>↓</kbd> for vertical orientation.
 *
 * If the orientation is horizontal, for right-to-left locales, gestures and
 * shortcuts are reversed.
 *
 * Only children that have [property@AlbumPage:navigatable] set to `TRUE` can
 * be navigated to.
 *
 * Since: 1.0
 */
void
bis_album_set_can_navigate_forward (BisAlbum *self,
                                      gboolean    can_navigate_forward)
{
  g_return_if_fail (BIS_IS_ALBUM (self));

  can_navigate_forward = !!can_navigate_forward;

  if (self->child_transition.can_navigate_forward == can_navigate_forward)
    return;

  self->child_transition.can_navigate_forward = can_navigate_forward;
  bis_swipe_tracker_set_enabled (self->tracker, self->child_transition.can_navigate_back || can_navigate_forward);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_CAN_NAVIGATE_FORWARD]);
}

/**
 * bis_album_get_adjacent_child:
 * @self: a album
 * @direction: the direction
 *
 * Finds the previous or next navigatable child.
 *
 * This will be the same child [method@Album.navigate] or swipe gestures will
 * navigate to.
 *
 * If there's no child to navigate to, `NULL` will be returned instead.
 *
 * See [property@AlbumPage:navigatable].
 *
 * Returns: (nullable) (transfer none): the previous or next child
 *
 * Since: 1.0
 */
GtkWidget *
bis_album_get_adjacent_child (BisAlbum             *self,
                                BisNavigationDirection  direction)
{
  BisAlbumPage *page;

  g_return_val_if_fail (BIS_IS_ALBUM (self), NULL);

  page = find_swipeable_page (self, direction);

  return page ? page->widget : NULL;
}

/**
 * bis_album_navigate:
 * @self: a album
 * @direction: the direction
 *
 * Navigates to the previous or next child.
 *
 * The child must have the [property@AlbumPage:navigatable] property set to
 * `TRUE`, otherwise it will be skipped.
 *
 * This will be the same child as returned by
 * [method@Album.get_adjacent_child] or navigated to via swipe gestures.
 *
 * Returns: whether the visible child was changed
 *
 * Since: 1.0
 */
gboolean
bis_album_navigate (BisAlbum             *self,
                      BisNavigationDirection  direction)
{
  BisAlbumPage *page;

  g_return_val_if_fail (BIS_IS_ALBUM (self), FALSE);
  g_return_val_if_fail (direction == BIS_NAVIGATION_DIRECTION_BACK ||
                        direction == BIS_NAVIGATION_DIRECTION_FORWARD, FALSE);

  page = find_swipeable_page (self, direction);

  if (!page)
    return FALSE;

  set_visible_child (self, page);

  return TRUE;
}

/**
 * bis_album_get_child_by_name:
 * @self: a album
 * @name: the name of the child to find
 *
 * Finds the child of @self with @name.
 *
 * Returns `NULL` if there is no child with this name.
 *
 * See [property@AlbumPage:name].
 *
 * Returns: (transfer none) (nullable): the requested child of @self
 *
 * Since: 1.0
 */
GtkWidget *
bis_album_get_child_by_name (BisAlbum  *self,
                               const char  *name)
{
  BisAlbumPage *page;

  g_return_val_if_fail (BIS_IS_ALBUM (self), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  page = find_page_for_name (self, name);

  return page ? page->widget : NULL;
}

/**
 * bis_album_get_pages: (attributes org.gtk.Method.get_property=pages)
 * @self: a album
 *
 * Returns a [iface@Gio.ListModel] that contains the pages of the album.
 *
 * This can be used to keep an up-to-date view. The model also implements
 * [iface@Gtk.SelectionModel] and can be used to track and change the visible
 * page.
 *
 * Returns: (transfer full): a `GtkSelectionModel` for the album's children
 *
 * Since: 1.0
 */
GtkSelectionModel *
bis_album_get_pages (BisAlbum *self)
{
  g_return_val_if_fail (BIS_IS_ALBUM (self), NULL);

  if (self->pages)
    return g_object_ref (self->pages);

  self->pages = GTK_SELECTION_MODEL (bis_album_pages_new (self));
  g_object_add_weak_pointer (G_OBJECT (self->pages), (gpointer *) &self->pages);

  return self->pages;
}
