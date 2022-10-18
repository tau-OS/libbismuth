/*
 * Copyright (C) 2018-2021 Purism SPC
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include "config.h"

#include "bis-main-private.h"

#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>

static int bis_initialized = FALSE;

/**
 * bis_init:
 *
 * Initializes Libbismuth.
 *
 * This function can be used instead of [func@Gtk.init] as it initializes GTK
 * implicitly.
 *
 * There's no need to call this function if you're using [class@Application].
 *
 * If Libbismuth has already been initialized, the function will simply return.
 *
 * This makes sure translations, types, themes, and icons for the Bismuth
 * library are set up properly.
 *
 * Since: 1.0
 */
void
bis_init (void)
{
  if (bis_initialized)
    return;

  gtk_init ();

  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
  bis_init_public_types ();
  bis_initialized = TRUE;
}

/**
 * bis_is_initialized:
 *
 * Use this function to check if libbismuth has been initialized with
 * [func@init].
 *
 * Returns: the initialization status
 */
gboolean
bis_is_initialized (void)
{
  return bis_initialized;
}
