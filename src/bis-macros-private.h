/*
 * Copyright (C) 2021 Purism SPC
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once

#if !defined(_BISMUTH_INSIDE) && !defined(BISMUTH_COMPILATION)
#error "Only <bismuth.h> can be included directly."
#endif

#include <glib-object.h>

G_BEGIN_DECLS

#if !GLIB_CHECK_VERSION(2, 70, 0)
#define G_DEFINE_FINAL_TYPE           G_DEFINE_TYPE
#define G_DEFINE_FINAL_TYPE_WITH_CODE G_DEFINE_TYPE_WITH_CODE
#endif

#define BIS_CRITICAL_CANNOT_REMOVE_CHILD(parent, child) \
G_STMT_START { \
  g_critical ("%s:%d: tried to remove non-child %p of type '%s' from %p of type '%s'", \
              __FILE__, __LINE__, \
              (child), \
              G_OBJECT_TYPE_NAME ((GObject*) (child)), \
              (parent), \
              G_OBJECT_TYPE_NAME ((GObject*) (parent))); \
} G_STMT_END

G_END_DECLS
