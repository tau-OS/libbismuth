/*
 * Copyright (C) 2017 Purism SPC
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once

#if !defined(_BISMUTH_INSIDE) && !defined(BISMUTH_COMPILATION)
#error "Only <bismuth.h> can be included directly."
#endif

#if defined(BIS_DISABLE_DEPRECATION_WARNINGS) || defined(BISMUTH_COMPILATION)
#  define _BIS_DEPRECATED
#  define _BIS_DEPRECATED_FOR(f)
#  define _BIS_DEPRECATED_MACRO
#  define _BIS_DEPRECATED_MACRO_FOR(f)
#  define _BIS_DEPRECATED_ENUMERATOR
#  define _BIS_DEPRECATED_ENUMERATOR_FOR(f)
#  define _BIS_DEPRECATED_TYPE
#  define _BIS_DEPRECATED_TYPE_FOR(f)
#else
#  define _BIS_DEPRECATED                G_DEPRECATED
#  define _BIS_DEPRECATED_FOR(f)         G_DEPRECATED_FOR(f)
#  define _BIS_DEPRECATED_MACRO          G_DEPRECATED
#  define _BIS_DEPRECATED_MACRO_FOR(f)   G_DEPRECATED_FOR(f)
#  define _BIS_DEPRECATED_ENUMERATOR          G_DEPRECATED
#  define _BIS_DEPRECATED_ENUMERATOR_FOR(f)   G_DEPRECATED_FOR(f)
#  define _BIS_DEPRECATED_TYPE           G_DEPRECATED
#  define _BIS_DEPRECATED_TYPE_FOR(f)    G_DEPRECATED_FOR(f)
#endif
