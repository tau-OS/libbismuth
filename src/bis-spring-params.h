/*
 * Copyright (C) 2021 Manuel Genovés <manuel.genoves@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#pragma once

#include "bis-version.h"

#include <glib-object.h>

G_BEGIN_DECLS

#define BIS_TYPE_SPRING_PARAMS (bis_spring_params_get_type())

typedef struct _BisSpringParams BisSpringParams;

BIS_AVAILABLE_IN_ALL
GType bis_spring_params_get_type (void) G_GNUC_CONST;

BIS_AVAILABLE_IN_ALL
BisSpringParams *bis_spring_params_new         (double damping_ratio,
                                                double mass,
                                                double stiffness) G_GNUC_WARN_UNUSED_RESULT;
BIS_AVAILABLE_IN_ALL
BisSpringParams *bis_spring_params_new_full    (double damping,
                                                double mass,
                                                double stiffness) G_GNUC_WARN_UNUSED_RESULT;

BIS_AVAILABLE_IN_ALL
BisSpringParams *bis_spring_params_ref   (BisSpringParams *self);
BIS_AVAILABLE_IN_ALL
void             bis_spring_params_unref (BisSpringParams *self);

BIS_AVAILABLE_IN_ALL
double bis_spring_params_get_damping       (BisSpringParams *self);
BIS_AVAILABLE_IN_ALL
double bis_spring_params_get_damping_ratio (BisSpringParams *self);
BIS_AVAILABLE_IN_ALL
double bis_spring_params_get_mass          (BisSpringParams *self);
BIS_AVAILABLE_IN_ALL
double bis_spring_params_get_stiffness     (BisSpringParams *self);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (BisSpringParams, bis_spring_params_unref)

G_END_DECLS
