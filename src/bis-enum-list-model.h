/*
 * Copyright (C) 2020 Purism SPC
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once

#if !defined(_BISMUTH_INSIDE) && !defined(BISMUTH_COMPILATION)
#error "Only <bismuth.h> can be included directly."
#endif

#include "bis-version.h"

#include <glib-object.h>

G_BEGIN_DECLS

#define BIS_TYPE_ENUM_LIST_ITEM (bis_enum_list_item_get_type())

BIS_AVAILABLE_IN_ALL
G_DECLARE_FINAL_TYPE (BisEnumListItem, bis_enum_list_item, BIS, ENUM_LIST_ITEM, GObject)

BIS_AVAILABLE_IN_ALL
int bis_enum_list_item_get_value (BisEnumListItem *self);

BIS_AVAILABLE_IN_ALL
const char *bis_enum_list_item_get_name (BisEnumListItem *self);

BIS_AVAILABLE_IN_ALL
const char *bis_enum_list_item_get_nick (BisEnumListItem *self);

#define BIS_TYPE_ENUM_LIST_MODEL (bis_enum_list_model_get_type())

BIS_AVAILABLE_IN_ALL
G_DECLARE_FINAL_TYPE (BisEnumListModel, bis_enum_list_model, BIS, ENUM_LIST_MODEL, GObject)

BIS_AVAILABLE_IN_ALL
BisEnumListModel *bis_enum_list_model_new (GType enum_type) G_GNUC_WARN_UNUSED_RESULT;

BIS_AVAILABLE_IN_ALL
GType bis_enum_list_model_get_enum_type (BisEnumListModel *self);

BIS_AVAILABLE_IN_ALL
guint bis_enum_list_model_find_position (BisEnumListModel *self,
                                         int               value);

G_END_DECLS
