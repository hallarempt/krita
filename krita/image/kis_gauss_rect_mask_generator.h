/*
 *  Copyright (c) 2010 Lukáš Tvrdý <lukast.dev@gmail.com>
 *  Copyright (c) 2011 Geoffry Song <goffrie@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef _KIS_GAUSS_RECT_MASK_GENERATOR_H_
#define _KIS_GAUSS_RECT_MASK_GENERATOR_H_

#include "kritaimage_export.h"



/**
 * This mask generator uses a Gaussian-blurred rectangle
 */
class KRITAIMAGE_EXPORT KisGaussRectangleMaskGenerator : public KisMaskGenerator
{

public:

    KisGaussRectangleMaskGenerator(qreal diameter, qreal ratio, qreal fh, qreal fv, int spikes, bool antialiasEdges);
    virtual ~KisGaussRectangleMaskGenerator();

    virtual quint8 valueAt(qreal x, qreal y) const;
    void setScale(qreal scaleX, qreal scaleY);

private:

    qreal norme(qreal a, qreal b) const {
        return a*a + b*b;
    }

private:
    struct Private;
    Private* const d;
};

#endif
