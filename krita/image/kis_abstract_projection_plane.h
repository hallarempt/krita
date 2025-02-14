/*
 *  Copyright (c) 2015 Dmitry Kazakov <dimula73@gmail.com>
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

#ifndef __KIS_ABSTRACT_PROJECTION_PLANE_H
#define __KIS_ABSTRACT_PROJECTION_PLANE_H

#include "kis_types.h"
#include "kis_layer.h"

class QRect;
class KisPainter;


/**
 * An interface of the node to the compositioning
 * system. Compositioning system KisAsyncMerger knows nothing about
 * the internals of the layer, it just knows that the layer can:
 *
 * 1) recalculate() its internal representation if it is filthy
 *
 * 2) apply() itself onto a global projection using, writing all its data
 *            to the projection.
 *
 * 3) report parameters of these operations using needRect(),
 *    changeRect() and accessRect() methods, which mean the same as
 *    the corresponding methods of KisNode, but include more
 *    transformations for the layer, e.g. Layer Styles and
 *    Pass-through mode.
 */
class KRITAIMAGE_EXPORT  KisAbstractProjectionPlane
{
public:
    KisAbstractProjectionPlane();
    virtual ~KisAbstractProjectionPlane();

    /**
     * Is called by the async merger when the node is filthy and
     * should recalculate its internal representation. For usual
     * layers it means just calling updateProjection().
     */
    virtual QRect recalculate(const QRect& rect, KisNodeSP filthyNode) = 0;

    /**
     * Writes the data of the projection plane onto a global
     * projection using \p painter object.
     */
    virtual void apply(KisPainter *painter, const QRect &rect) = 0;

    /**
     * Works like KisNode::needRect(), but includes more
     * transformations of the layer
     */
    virtual QRect needRect(const QRect &rect, KisLayer::PositionToFilthy pos = KisLayer::N_FILTHY) const = 0;

    /**
     * Works like KisNode::changeRect(), but includes more
     * transformations of the layer
     */
    virtual QRect changeRect(const QRect &rect, KisLayer::PositionToFilthy pos = KisLayer::N_FILTHY) const = 0;

    /**
     * Works like KisNode::needRect(), but includes more
     * transformations of the layer
     */
    virtual QRect accessRect(const QRect &rect, KisLayer::PositionToFilthy pos = KisLayer::N_FILTHY) const = 0;

    /**
     * Synchronizes the LoD cache of all the internal paint devices
     * included into the projection.
     */
    virtual void syncLodCache() = 0;
};

/**
 * A simple implementation of KisAbstractProjectionPlane interface
 * that does nothing.
 */
class KisDumbProjectionPlane : public KisAbstractProjectionPlane
{
public:
    QRect recalculate(const QRect& rect, KisNodeSP filthyNode);
    void apply(KisPainter *painter, const QRect &rect);

    QRect needRect(const QRect &rect, KisLayer::PositionToFilthy pos) const;
    QRect changeRect(const QRect &rect, KisLayer::PositionToFilthy pos) const;
    QRect accessRect(const QRect &rect, KisLayer::PositionToFilthy pos) const;

    void syncLodCache();
};

#endif /* __KIS_ABSTRACT_PROJECTION_PLANE_H */
