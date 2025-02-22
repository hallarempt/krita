/*
 *  Copyright (c) 2005 Boudewijn Rempt <boud@valdyas.org>
 *
 *  This library is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published
 *  by the Free Software Foundation; either version 2.1 of the License, or
 *  (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef _KO_COLORCONVERSIONS_H_
#define _KO_COLORCONVERSIONS_H_

#include <QtGlobal>
#include "kritapigment_export.h"

/**
 * A number of often-used conversions between color models
 */

// 8-bit integer versions. RGBSL are 0-255, H is 0-360.
KRITAPIGMENT_EXPORT void rgb_to_hsv(int R, int G, int B, int *H, int *S, int *V);
KRITAPIGMENT_EXPORT void hsv_to_rgb(int H, int S, int V, int *R, int *G, int *B);

// Floating point versions. RGBSL are 0-1, H is 0-360.
KRITAPIGMENT_EXPORT void RGBToHSV(float r, float g, float b, float *h, float *s, float *v);
KRITAPIGMENT_EXPORT void HSVToRGB(float h, float s, float v, float *r, float *g, float *b);

KRITAPIGMENT_EXPORT void RGBToHSL(float r, float g, float b, float *h, float *s, float *l);
KRITAPIGMENT_EXPORT void HSLToRGB(float h, float sl, float l, float *r, float *g, float *b);

KRITAPIGMENT_EXPORT void rgb_to_hls(quint8 r, quint8 g, quint8 b, float * h, float * l, float * s);

KRITAPIGMENT_EXPORT float hue_value(float n1, float n2, float hue);

KRITAPIGMENT_EXPORT void hls_to_rgb(float h, float l, float s, quint8 * r, quint8 * g, quint8 * b);

KRITAPIGMENT_EXPORT void rgb_to_hls(quint8 r, quint8 g, quint8 b, int * h, int * l, int * s);
KRITAPIGMENT_EXPORT void hls_to_rgb(int h, int l, int s, quint8 * r, quint8 * g, quint8 * b);

//HSI and HSY' functions.
//These are modified to calculate a cylliner, this is good for colour selectors sliders.
//All eight expect 0.0-1.0 for all parameters.
//HSI measures the Tone, Intensity, by adding the r, g and b components and then normalising that.
KRITAPIGMENT_EXPORT void HSIToRGB(const qreal h, const qreal s, const qreal i, qreal *red, qreal *green, qreal *blue);
KRITAPIGMENT_EXPORT void RGBToHSI(qreal r, qreal g, qreal b, qreal *h, qreal *s, qreal *i);

//HSY' measures the tone, Luma, by weighting the r, g, and b components before adding them up.
//The R, G, B reffers to the weights, and defaults to the 601 rec for luma.
KRITAPIGMENT_EXPORT void RGBToHSY( qreal r, qreal g, qreal b, qreal *h, qreal *s, qreal *y, qreal R=0.299, qreal G=0.587, qreal B=0.114);
KRITAPIGMENT_EXPORT void HSYToRGB(const qreal h, const qreal s, const qreal y, qreal *red, qreal *green, qreal *blue, qreal R=0.299, qreal G=0.587, qreal B=0.114);


//HCI and HCY' functions.
//These are the original conversion functions, producing cones. Put in for completion.
//There's HCI to RGB is based on the HCY to RGB one for now, it may not be correct.
KRITAPIGMENT_EXPORT void HCIToRGB(const qreal h, const qreal s, const qreal i, qreal *red, qreal *green, qreal *blue);
KRITAPIGMENT_EXPORT void RGBToHCI(const qreal r, const qreal g, const qreal b, qreal *h, qreal *c, qreal *i);

KRITAPIGMENT_EXPORT void HCYToRGB(const qreal h, const qreal s, const qreal y, qreal *red, qreal *green, qreal *blue,  qreal R=0.299, qreal G=0.587, qreal B=0.114);
KRITAPIGMENT_EXPORT void RGBToHCY(const qreal r, const qreal g, const qreal b, qreal *h, qreal *c, qreal *y,  qreal R=0.299, qreal G=0.587, qreal B=0.114);


#endif // _KO_COLORCONVERSIONS_H_

