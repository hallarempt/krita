/*
 *  Copyright (c) 2010 Lukáš Tvrdý <lukast.dev@gmail.com>
 *  Copyright (c) 2010 Ricardo Cabello <hello@mrdoob.com>
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

#include "kis_sketch_paintop.h"
#include "kis_sketch_paintop_settings.h"

#include <cmath>
#include <QRect>

#include <KoColor.h>
#include <KoColorSpace.h>

#include <kis_image.h>
#include <kis_debug.h>

#include <kis_global.h>
#include <kis_paint_device.h>
#include <kis_painter.h>
#include <kis_types.h>
#include <kis_paintop.h>
#include <kis_paint_information.h>
#include <kis_fixed_paint_device.h>

#include <kis_pressure_opacity_option.h>
#include <kis_dab_cache.h>
#include "kis_lod_transform.h"


#include <QtGlobal>

/*
* Based on Harmony project http://github.com/mrdoob/harmony/
*/
// chrome : diff 0.2, sketchy : 0.3, fur: 0.5
// fur : distance / thresholdDistance

// shaded: opacity per line :/
// ((1 - (d / 1000)) * 0.1 * BRUSH_PRESSURE), offset == 0
// chrome: color per line :/
//this.context.strokeStyle = "rgba(" + Math.floor(Math.random() * COLOR[0]) + ", " + Math.floor(Math.random() * COLOR[1]) + ", " + Math.floor(Math.random() * COLOR[2]) + ", " + 0.1 * BRUSH_PRESSURE + " )";

// long fur
// from: count + offset * -random
// to: i point - (offset * -random)  + random * 2
// probability distance / thresholdDistnace

// shaded: probabity : paint always - 0.0 density

KisSketchPaintOp::KisSketchPaintOp(const KisSketchPaintOpSettings *settings, KisPainter * painter, KisNodeSP node, KisImageSP image)
    : KisPaintOp(painter)
{
    Q_UNUSED(image);
    Q_UNUSED(node);

    m_opacityOption.readOptionSetting(settings);
    m_sizeOption.readOptionSetting(settings);
    m_rotationOption.readOptionSetting(settings);
    m_sketchProperties.readOptionSetting(settings);
    m_brushOption.readOptionSetting(settings);
    m_densityOption.readOptionSetting(settings);
    m_lineWidthOption.readOptionSetting(settings);
    m_offsetScaleOption.readOptionSetting(settings);

    m_brush = m_brushOption.brush();
    m_dabCache = new KisDabCache(m_brush);

    m_opacityOption.resetAllSensors();
    m_sizeOption.resetAllSensors();
    m_rotationOption.resetAllSensors();

    m_painter = 0;
    m_count = 0;
}

KisSketchPaintOp::~KisSketchPaintOp()
{
    delete m_painter;
    delete m_dabCache;
}

void KisSketchPaintOp::drawConnection(const QPointF& start, const QPointF& end, double lineWidth)
{
    if (lineWidth == 1.0) {
        m_painter->drawThickLine(start, end, lineWidth, lineWidth);
    }
    else {
        m_painter->drawLine(start, end, lineWidth, true);
    }
}

void KisSketchPaintOp::updateBrushMask(const KisPaintInformation& info, qreal scale, qreal rotation)
{
    QRect dstRect;
    m_maskDab = m_dabCache->fetchDab(m_dab->colorSpace(),
                                     painter()->paintColor(),
                                     info.pos(),
                                     scale, scale, rotation,
                                     info, 1.0,
                                     &dstRect);

    m_brushBoundingBox = dstRect;
    m_hotSpot = QPointF(0.5 * m_brushBoundingBox.width(),
                        0.5 * m_brushBoundingBox.height());
}

void KisSketchPaintOp::paintLine(const KisPaintInformation &pi1, const KisPaintInformation &pi2, KisDistanceInformation *currentDistance)
{
    Q_UNUSED(currentDistance);

    if (!m_brush || !painter()) return;

    if (!m_dab) {
        m_dab = source()->createCompositionSourceDevice();
        m_painter = new KisPainter(m_dab);
        m_painter->setPaintColor(painter()->paintColor());
    }
    else {
        m_dab->clear();
    }

    QPointF prevMouse = pi1.pos();
    QPointF mousePosition = pi2.pos();
    m_points.append(mousePosition);


    const qreal lodAdditionalScale = KisLodTransform::lodToScale(painter()->device());
    const qreal scale = lodAdditionalScale * m_sizeOption.apply(pi2);
    if ((scale * m_brush->width()) <= 0.01 || (scale * m_brush->height()) <= 0.01) return;

    const qreal currentLineWidth = qMax(0.9, lodAdditionalScale * m_lineWidthOption.apply(pi2, m_sketchProperties.lineWidth));

    const qreal currentOffsetScale = m_offsetScaleOption.apply(pi2, m_sketchProperties.offset);
    const double rotation = m_rotationOption.apply(pi2);
    const double currentProbability = m_densityOption.apply(pi2, m_sketchProperties.probability);

    // shaded: does not draw this line, chrome does, fur does
    if (m_sketchProperties.makeConnection) {
        drawConnection(prevMouse, mousePosition, currentLineWidth);
    }

    setCurrentScale(scale);
    setCurrentRotation(rotation);

    qreal thresholdDistance = 0.0;

    // update the mask for simple mode only once
    // determine the radius
    if (m_count == 0 && m_sketchProperties.simpleMode) {
        updateBrushMask(pi2, 1.0, 0.0);
        //m_radius = qMax(m_maskDab->bounds().width(),m_maskDab->bounds().height()) * 0.5;
        m_radius = 0.5 * qMax(m_brush->width(), m_brush->height());
    }

    if (!m_sketchProperties.simpleMode) {
        updateBrushMask(pi2, scale, rotation);
        m_radius = qMax(m_maskDab->bounds().width(), m_maskDab->bounds().height()) * 0.5;
        thresholdDistance = pow(m_radius, 2);
    }

    if (m_sketchProperties.simpleMode) {
        // update the radius according scale in simple mode
        thresholdDistance = pow(m_radius * scale, 2);
    }

    // determine density
    const qreal density = thresholdDistance * currentProbability;

    // probability behaviour
    qreal probability = 1.0 - currentProbability;

    QColor painterColor = painter()->paintColor().toQColor();
    QColor randomColor;
    KoColor color(m_dab->colorSpace());

    int w = m_maskDab->bounds().width();
    quint8 opacityU8 = 0;
    quint8 * pixel;
    qreal distance;
    QPoint  positionInMask;
    QPointF diff;

    int size = m_points.size();
    // MAIN LOOP
    for (int i = 0; i < size; i++) {
        diff = m_points.at(i) - mousePosition;
        distance = diff.x() * diff.x() + diff.y() * diff.y();

        // circle test
        bool makeConnection = false;
        if (m_sketchProperties.simpleMode) {
            if (distance < thresholdDistance) {
                makeConnection = true;
            }
            // mask test
        }
        else {
            if (m_brushBoundingBox.contains(m_points.at(i))) {
                positionInMask = (diff + m_hotSpot).toPoint();
                uint pos = ((positionInMask.y() * w + positionInMask.x()) * m_maskDab->pixelSize());
                if (pos < m_maskDab->allocatedPixels() * m_maskDab->pixelSize()) {
                    pixel = m_maskDab->data() + pos;
                    opacityU8 = m_maskDab->colorSpace()->opacityU8(pixel);
                    if (opacityU8 != 0) {
                        makeConnection = true;
                    }
                }
            }

        }

        if (!makeConnection) {
            // check next point
            continue;
        }

        if (m_sketchProperties.distanceDensity) {
            probability =  distance / density;
        }

        KisRandomSourceSP randomSource = pi2.randomSource();

        // density check
        if (randomSource->generateNormalized() >= probability) {
            QPointF offsetPt = diff * currentOffsetScale;

            if (m_sketchProperties.randomRGB) {
                /**
                 * Since the order of calculation of function
                 * parameters is not defined by C++ standard, we
                 * should generate values in an external code snippet
                 * which has a definite order of execution.
                 */
                qreal r1 = randomSource->generateNormalized();
                qreal r2 = randomSource->generateNormalized();
                qreal r3 = randomSource->generateNormalized();

                // some color transformation per line goes here
                randomColor.setRgbF(r1 * painterColor.redF(),
                                    r2 * painterColor.greenF(),
                                    r3 * painterColor.blueF());
                color.fromQColor(randomColor);
                m_painter->setPaintColor(color);
            }

            // distance based opacity
            quint8 opacity = OPACITY_OPAQUE_U8;
            if (m_sketchProperties.distanceOpacity) {
                opacity *= qRound((1.0 - (distance / thresholdDistance)));
            }

            if (m_sketchProperties.randomOpacity) {
                opacity *= randomSource->generateNormalized();
            }

            m_painter->setOpacity(opacity);

            if (m_sketchProperties.magnetify) {
                drawConnection(mousePosition + offsetPt, m_points.at(i) - offsetPt, currentLineWidth);
            }
            else {
                drawConnection(mousePosition + offsetPt, mousePosition - offsetPt, currentLineWidth);
            }



        }
    }// end of MAIN LOOP

    m_count++;

    QRect rc = m_dab->extent();
    quint8 origOpacity = m_opacityOption.apply(painter(), pi2);

    painter()->bitBlt(rc.x(), rc.y(), m_dab, rc.x(), rc.y(), rc.width(), rc.height());
    painter()->renderMirrorMask(rc, m_dab);
    painter()->setOpacity(origOpacity);
}



KisSpacingInformation KisSketchPaintOp::paintAt(const KisPaintInformation& info)
{
    KisDistanceInformation di;
    paintLine(info, info, &di);
    return di.currentSpacing();
}
