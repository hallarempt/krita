/*
 *  Copyright (c) 2004 Cyrille Berger <cberger@cberger.net>
 *  Copyright (c) 2010 Lukáš Tvrdý <lukast.dev@gmail.com>
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
#ifndef _KIS_AUTOBRUSH_RESOURCE_H_
#define _KIS_AUTOBRUSH_RESOURCE_H_

#include "kritabrush_export.h"
#include "kis_brush.h"

class KisMaskGenerator;

/**
 * XXX: docs!
 */
class BRUSH_EXPORT KisAutoBrush : public KisBrush
{

public:

    KisAutoBrush(KisMaskGenerator* as, qreal angle, qreal randomness, qreal density = 1.0);

    virtual ~KisAutoBrush();

public:

    virtual KisFixedPaintDeviceSP paintDevice(const KoColorSpace*,
            double, double,
            const KisPaintInformation&,
            double = 0, double = 0) const {
        return 0; // The autobrush does NOT support images!
    }

    virtual void generateMaskAndApplyMaskOrCreateDab(KisFixedPaintDeviceSP dst,
            KisBrush::ColoringInformation* src,
            double scaleX, double scaleY, double angle,
            const KisPaintInformation& info,
            double subPixelX = 0, double subPixelY = 0,
            qreal softnessFactor = DEFAULT_SOFTNESS_FACTOR) const;

    virtual QPainterPath outline() const;

public:

    bool load() {
        return false;
    }

    virtual bool loadFromDevice(QIODevice *) {
        return false;
    }

    bool save()  {
        return false;
    }

    bool saveToDevice(QIODevice*) const {
        return false;
    }

    void toXML(QDomDocument& , QDomElement&) const;
    const KisMaskGenerator* maskGenerator() const;
    qreal randomness() const;
    qreal density() const;

    void lodLimitations(KisPaintopLodLimitations *l) const;

private:

    QImage createBrushPreview();

    struct Private;
    Private* const d;
};
#endif // _KIS_AUTOBRUSH_RESOURCE_H_
