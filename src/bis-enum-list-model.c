/*
 * Copyright (C) 2018-2020 Purism SPC
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "config.h"

#include "bis-enum-list-model.h"

#include "bis-macros-private.h"

#include <gio/gio.h>

/**
 * BisEnumListModel:
 *
 * A [iface@Gio.ListModel] representing values of a given enum.
 *
 * `BisEnumListModel` contains objects of type [class@EnumListItem].
 *
 * Since: 1.0
 */

struct _BisEnumListModel
{
  GObject parent_instance;

  GType enum_type;
  GEnumClass *enum_class;

  BisEnumListItem **objects;
};

enum {
  PROP_0,
  PROP_ENUM_TYPE,
  LAST_PROP,
};

static GParamSpec *props[LAST_PROP];

static void bis_enum_list_model_list_model_init (GListModelInterface *iface);

G_DEFINE_FINAL_TYPE_WITH_CODE (BisEnumListModel, bis_enum_list_model, G_TYPE_OBJECT,
                               G_IMPLEMENT_INTERFACE (G_TYPE_LIST_MODEL, bis_enum_list_model_list_model_init))

/**
 * BisEnumListItem:
 *
 * `BisEnumListItem` is the type of items in a [class@EnumListModel].
 *
 * Since: 1.0
 */

struct _BisEnumListItem
{
  GObject parent_instance;

  GEnumValue enum_value;
};

enum {
  VALUE_PROP_0,
  VALUE_PROP_VALUE,
  VALUE_PROP_NAME,
  VALUE_PROP_NICK,
  LAST_VALUE_PROP,
};

static GParamSpec *value_props[LAST_VALUE_PROP];

G_DEFINE_FINAL_TYPE (BisEnumListItem, bis_enum_list_item, G_TYPE_OBJECT)

static void
bis_enum_list_item_get_property (GObject    *object,
                                 guint       prop_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  BisEnumListItem *self = BIS_ENUM_LIST_ITEM (object);

  switch (prop_id) {
  case VALUE_PROP_VALUE:
    g_value_set_int (value, bis_enum_list_item_get_value (self));
    break;
  case VALUE_PROP_NAME:
    g_value_set_string (value, bis_enum_list_item_get_name (self));
    break;
  case VALUE_PROP_NICK:
    g_value_set_string (value, bis_enum_list_item_get_nick (self));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
bis_enum_list_item_class_init (BisEnumListItemClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = bis_enum_list_item_get_property;

  /**
   * BisEnumListItem:value: (attributes org.gtk.Property.get=bis_enum_list_item_get_value)
   *
   * The enum value.
   *
   * Since: 1.0
   */
  value_props[VALUE_PROP_VALUE] =
    g_param_spec_int ("value", NULL, NULL,
                      G_MININT, G_MAXINT, 0,
                      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  /**
   * BisEnumListItem:name: (attributes org.gtk.Property.get=bis_enum_list_item_get_name)
   *
   * The enum value name.
   *
   * Since: 1.0
   */
  value_props[VALUE_PROP_NAME] =
    g_param_spec_string ("name", NULL, NULL,
                         NULL,
                         G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  /**
   * BisEnumListItem:nick: (attributes org.gtk.Property.get=bis_enum_list_item_get_nick)
   *
   * The enum value nick.
   *
   * Since: 1.0
   */
  value_props[VALUE_PROP_NICK] =
    g_param_spec_string ("nick", NULL, NULL,
                         NULL,
                         G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, LAST_VALUE_PROP, value_props);
}

static void
bis_enum_list_item_init (BisEnumListItem *self)
{
}

static BisEnumListItem *
bis_enum_list_item_new (GEnumValue *enum_value)
{
  BisEnumListItem *self = g_object_new (BIS_TYPE_ENUM_LIST_ITEM, NULL);

  self->enum_value = *enum_value;

  return self;
}

/**
 * bis_enum_list_item_get_value: (attributes org.gtk.Method.get_property=value)
 *
 * Gets the enum value.
 *
 * Returns: the enum value
 *
 * Since: 1.0
 */
int
bis_enum_list_item_get_value (BisEnumListItem *self)
{
  g_return_val_if_fail (BIS_IS_ENUM_LIST_ITEM (self), 0);

  return self->enum_value.value;
}

/**
 * bis_enum_list_item_get_name: (attributes org.gtk.Method.get_property=name)
 *
 * Gets the enum value name.
 *
 * Returns: the enum value name
 *
 * Since: 1.0
 */
const char *
bis_enum_list_item_get_name (BisEnumListItem *self)
{
  g_return_val_if_fail (BIS_IS_ENUM_LIST_ITEM (self), NULL);

  return self->enum_value.value_name;
}

/**
 * bis_enum_list_item_get_nick: (attributes org.gtk.Method.get_property=nick)
 *
 * Gets the enum value nick.
 *
 * Returns: the enum value nick
 *
 * Since: 1.0
 */
const char *
bis_enum_list_item_get_nick (BisEnumListItem *self)
{
  g_return_val_if_fail (BIS_IS_ENUM_LIST_ITEM (self), NULL);

  return self->enum_value.value_nick;
}

static void
bis_enum_list_model_constructed (GObject *object)
{
  BisEnumListModel *self = BIS_ENUM_LIST_MODEL (object);
  guint i;

  self->enum_class = g_type_class_ref (self->enum_type);

  self->objects = g_new0 (BisEnumListItem *, self->enum_class->n_values);

  for (i = 0; i < self->enum_class->n_values; i++)
    self->objects[i] = bis_enum_list_item_new (&self->enum_class->values[i]);

  G_OBJECT_CLASS (bis_enum_list_model_parent_class)->constructed (object);
}

static void
bis_enum_list_model_finalize (GObject *object)
{
  BisEnumListModel *self = BIS_ENUM_LIST_MODEL (object);

  g_clear_pointer (&self->enum_class, g_type_class_unref);
  g_clear_pointer (&self->objects, g_free);

  G_OBJECT_CLASS (bis_enum_list_model_parent_class)->finalize (object);
}

static void
bis_enum_list_model_get_property (GObject    *object,
                                  guint       prop_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  BisEnumListModel *self = BIS_ENUM_LIST_MODEL (object);

  switch (prop_id) {
  case PROP_ENUM_TYPE:
    g_value_set_gtype (value, bis_enum_list_model_get_enum_type (self));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
bis_enum_list_model_set_property (GObject      *object,
                                  guint         prop_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  BisEnumListModel *self = BIS_ENUM_LIST_MODEL (object);

  switch (prop_id) {
  case PROP_ENUM_TYPE:
    self->enum_type = g_value_get_gtype (value);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
bis_enum_list_model_class_init (BisEnumListModelClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed = bis_enum_list_model_constructed;
  object_class->finalize = bis_enum_list_model_finalize;
  object_class->get_property = bis_enum_list_model_get_property;
  object_class->set_property = bis_enum_list_model_set_property;

  /**
   * BisEnumListModel:enum-type: (attributes org.gtk.Property.get=bis_enum_list_model_get_enum_type)
   *
   * The type of the enum represented by the model.
   *
   * Since: 1.0
   */
  props[PROP_ENUM_TYPE] =
    g_param_spec_gtype ("enum-type", NULL, NULL,
                        G_TYPE_ENUM,
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, LAST_PROP, props);
}

static void
bis_enum_list_model_init (BisEnumListModel *self)
{
}

static GType
bis_enum_list_model_get_item_type (GListModel *list)
{
  return BIS_TYPE_ENUM_LIST_ITEM;
}

static guint
bis_enum_list_model_get_n_items (GListModel *list)
{
  BisEnumListModel *self = BIS_ENUM_LIST_MODEL (list);

  return self->enum_class->n_values;
}

static gpointer
bis_enum_list_model_get_item (GListModel *list,
                              guint       position)
{
  BisEnumListModel *self = BIS_ENUM_LIST_MODEL (list);

  if (position >= self->enum_class->n_values)
    return NULL;

  return g_object_ref (self->objects[position]);
}

static void
bis_enum_list_model_list_model_init (GListModelInterface *iface)
{
  iface->get_item_type = bis_enum_list_model_get_item_type;
  iface->get_n_items = bis_enum_list_model_get_n_items;
  iface->get_item = bis_enum_list_model_get_item;
}

/**
 * bis_enum_list_model_new:
 * @enum_type: the type of the enum to construct the model from
 *
 * Creates a new `BisEnumListModel` for @enum_type.
 *
 * Returns: the newly created `BisEnumListModel`
 *
 * Since: 1.0
 */
BisEnumListModel *
bis_enum_list_model_new (GType enum_type)
{
  return g_object_new (BIS_TYPE_ENUM_LIST_MODEL,
                       "enum-type", enum_type,
                       NULL);
}

/**
 * bis_enum_list_model_get_enum_type: (attributes org.gtk.Method.get_property=enum-type)
 *
 * Gets the type of the enum represented by @self.
 *
 * Returns: the enum type
 *
 * Since: 1.0
 */
GType
bis_enum_list_model_get_enum_type (BisEnumListModel *self)
{
  g_return_val_if_fail (BIS_IS_ENUM_LIST_MODEL (self), G_TYPE_INVALID);

  return self->enum_type;
}

/**
 * bis_enum_list_model_find_position:
 * @value: an enum value
 *
 * Finds the position of a given enum value in @self.
 *
 * Since: 1.0
 */
guint
bis_enum_list_model_find_position (BisEnumListModel *self,
                                   int               value)
{
  guint i;

  g_return_val_if_fail (BIS_IS_ENUM_LIST_MODEL (self), 0);

  for (i = 0; i < self->enum_class->n_values; i++)
    if (self->enum_class->values[i].value == value)
      return i;

  g_critical ("%s does not contain value %d",
              G_ENUM_CLASS_TYPE_NAME (self->enum_class), value);

  return 0;
}
