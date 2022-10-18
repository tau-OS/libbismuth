/*
 * Copyright (C) 2021 Purism SPC
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * Author: Alexander Mikhaylenko <alexander.mikhaylenko@puri.sm>
 */

#include "config.h"

#include "bis-version.h"

/**
 * bis_get_major_version:
 *
 * Returns the major version number of the Bismuth library.
 *
 * For example, in libbismuth version 1.2.3 this is 1.
 *
 * This function is in the library, so it represents the libbismuth library your
 * code is running against. Contrast with the [const@MAJOR_VERSION] constant,
 * which represents the major version of the libbismuth headers you have
 * included when compiling your code.
 *
 * Returns: the major version number of the Bismuth library
 */
guint
bis_get_major_version (void)
{
  return BIS_MAJOR_VERSION;
}

/**
 * bis_get_minor_version:
 *
 * Returns the minor version number of the Bismuth library.
 *
 * For example, in libbismuth version 1.2.3 this is 2.
 *
 * This function is in the library, so it represents the libbismuth library your
 * code is running against. Contrast with the [const@MAJOR_VERSION] constant,
 * which represents the minor version of the libbismuth headers you have
 * included when compiling your code.
 *
 * Returns: the minor version number of the Bismuth library
 */
guint
bis_get_minor_version (void)
{
  return BIS_MINOR_VERSION;
}

/**
 * bis_get_micro_version:
 *
 * Returns the micro version number of the Bismuth library.
 *
 * For example, in libbismuth version 1.2.3 this is 3.
 *
 * This function is in the library, so it represents the libbismuth library your
 * code is running against. Contrast with the [const@MAJOR_VERSION] constant,
 * which represents the micro version of the libbismuth headers you have
 * included when compiling your code.
 *
 * Returns: the micro version number of the Bismuth library
 */
guint
bis_get_micro_version (void)
{
  return BIS_MICRO_VERSION;
}
