/*
 *  Copyright (c) 2007 Boudewijn Rempt <boud@valdyas.org>
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

#include "kis_clone_layer.h"

#include <kis_debug.h>
#include <klocalizedstring.h>

#include <KoIcon.h>
#include <kis_icon.h>

#include <KoColorSpace.h>
#include <KoCompositeOpRegistry.h>

#include "kis_default_bounds.h"
#include "kis_paint_device.h"
#include "kis_image.h"
#include "kis_painter.h"
#include "kis_node_visitor.h"
#include "kis_processing_visitor.h"
#include "kis_paint_layer.h"

#include <QStack>
#include <kis_effect_mask.h>
#include "kis_lod_capable_layer_offset.h"


struct Q_DECL_HIDDEN KisCloneLayer::Private
{
    Private(KisDefaultBoundsBaseSP defaultBounds)
        : offset(defaultBounds)
    {
    }

    KisLodCapableLayerOffset offset;

    KisLayerSP copyFrom;
    KisCloneInfo copyFromInfo;
    CopyLayerType type;
};

KisCloneLayer::KisCloneLayer(KisLayerSP from, KisImageWSP image, const QString &name, quint8 opacity)
        : KisLayer(image, name, opacity)
        , m_d(new Private(new KisDefaultBounds(image)))
{
    m_d->copyFrom = from;
    m_d->type = COPY_PROJECTION;

    // When loading the layer we copy from might not exist yet
    if (m_d->copyFrom) {
        m_d->copyFrom->registerClone(this);
    }
}

KisCloneLayer::KisCloneLayer(const KisCloneLayer& rhs)
        : KisLayer(rhs)
        , m_d(new Private(new KisDefaultBounds(rhs.image())))
{
    m_d->copyFrom = rhs.copyFrom();
    m_d->type = rhs.copyType();
    m_d->offset = rhs.m_d->offset;

    if (m_d->copyFrom) {
        m_d->copyFrom->registerClone(this);
    }
}

KisCloneLayer::~KisCloneLayer()
{
    if (m_d->copyFrom) {
        m_d->copyFrom->unregisterClone(this);
    }
    delete m_d;
}

KisLayerSP KisCloneLayer::reincarnateAsPaintLayer() const
{
    KisPaintDeviceSP newOriginal = new KisPaintDevice(*original());
    KisPaintLayerSP newLayer = new KisPaintLayer(image(), name(), opacity(), newOriginal);
    newLayer->setX(newLayer->x() + x());
    newLayer->setY(newLayer->y() + y());
    newLayer->setCompositeOpId(compositeOpId());
    newLayer->mergeNodeProperties(nodeProperties());

    return newLayer;
}

bool KisCloneLayer::allowAsChild(KisNodeSP node) const
{
    return node->inherits("KisMask");
}

KisPaintDeviceSP KisCloneLayer::paintDevice() const
{
    return 0;
}

KisPaintDeviceSP KisCloneLayer::original() const
{
    Q_ASSERT(m_d->copyFrom);

    KisPaintDeviceSP retval;
    switch (m_d->type) {
    case COPY_PROJECTION:
        retval = m_d->copyFrom->projection();
        break;

    case COPY_ORIGINAL:
    default:
        retval = m_d->copyFrom->original();
    }

    return retval;
}

bool KisCloneLayer::needProjection() const
{
return m_d->offset.x() || m_d->offset.y();
}

void KisCloneLayer::copyOriginalToProjection(const KisPaintDeviceSP original,
        KisPaintDeviceSP projection,
        const QRect& rect) const
{
    QRect copyRect = rect;
    copyRect.translate(-m_d->offset.x(), -m_d->offset.y());

    KisPainter::copyAreaOptimized(rect.topLeft(), original, projection, copyRect);
}

void KisCloneLayer::setDirtyOriginal(const QRect &rect)
{
    /**
     * The original will be updated when the clone becomes visible
     * again.
     */
    if (!visible(true)) return;

    /**
     *  HINT: this method is present for historical reasons only.
     *        Long time ago the updates were calculated in
     *        "copyOriginalToProjection" coordinate system. Now
     *        everything is done in "original()" space.
     */
    KisLayer::setDirty(rect);
}

void KisCloneLayer::notifyParentVisibilityChanged(bool value)
{
    KisLayer::setDirty(image()->bounds());
    KisLayer::notifyParentVisibilityChanged(value);
}

QRect KisCloneLayer::needRectOnSourceForMasks(const QRect &rc) const
{
    QStack<QRect> applyRects_unused;
    bool rectVariesFlag;

    QList<KisEffectMaskSP> effectMasks = this->effectMasks();
    if (effectMasks.isEmpty()) return QRect();

    QRect needRect = this->masksNeedRect(effectMasks,
                                         rc,
                                         applyRects_unused,
                                         rectVariesFlag);

    if (needRect.isEmpty() ||
        (!rectVariesFlag && needRect == rc)) {

        return QRect();
    }

    return needRect;
}

qint32 KisCloneLayer::x() const
{
    return m_d->offset.x();
}
qint32 KisCloneLayer::y() const
{
    return m_d->offset.y();
}
void KisCloneLayer::setX(qint32 x)
{
    m_d->offset.setX(x);
}
void KisCloneLayer::setY(qint32 y)
{
    m_d->offset.setY(y);
}

QRect KisCloneLayer::extent() const
{
    QRect rect = original()->extent();

    // HINT: no offset now. See a comment in setDirtyOriginal()
    return rect | projection()->extent();
}

QRect KisCloneLayer::exactBounds() const
{
    QRect rect = original()->exactBounds();

    // HINT: no offset now. See a comment in setDirtyOriginal()
    return rect | projection()->exactBounds();
}

QRect KisCloneLayer::accessRect(const QRect &rect, PositionToFilthy pos) const
{
    QRect resultRect = rect;

    if(pos & (N_FILTHY_PROJECTION | N_FILTHY)) {
        if (m_d->offset.x() || m_d->offset.y()) {
            resultRect |= rect.translated(-m_d->offset.x(), -m_d->offset.y());
        }

        /**
         * KisUpdateOriginalVisitor will try to recalculate some area
         * on the clone's source, so this extra rectangle should also
         * be taken into account
         */
        resultRect |= needRectOnSourceForMasks(rect);
    }

    return resultRect;
}

QRect KisCloneLayer::outgoingChangeRect(const QRect &rect) const
{
    return rect.translated(m_d->offset.x(), m_d->offset.y());
}

bool KisCloneLayer::accept(KisNodeVisitor & v)
{
    return v.visit(this);
}

void KisCloneLayer::accept(KisProcessingVisitor &visitor, KisUndoAdapter *undoAdapter)
{
    return visitor.visit(this, undoAdapter);
}

void KisCloneLayer::setCopyFrom(KisLayerSP fromLayer)
{
    if (m_d->copyFrom) {
        m_d->copyFrom->unregisterClone(this);
    }

    m_d->copyFrom = fromLayer;

    if (m_d->copyFrom) {
        m_d->copyFrom->registerClone(this);
    }
}

KisLayerSP KisCloneLayer::copyFrom() const
{
    return m_d->copyFrom;
}

void KisCloneLayer::setCopyType(CopyLayerType type)
{
    m_d->type = type;
}

CopyLayerType KisCloneLayer::copyType() const
{
    return m_d->type;
}

KisCloneInfo KisCloneLayer::copyFromInfo() const
{
    return m_d->copyFrom ? KisCloneInfo(m_d->copyFrom) : m_d->copyFromInfo;
}

void KisCloneLayer::setCopyFromInfo(KisCloneInfo info)
{
    Q_ASSERT(!m_d->copyFrom);
    m_d->copyFromInfo = info;
}

QIcon KisCloneLayer::icon() const
{
    return KisIconUtils::loadIcon("edit-copy");
}

KisNodeModel::PropertyList KisCloneLayer::sectionModelProperties() const
{
    KisNodeModel::PropertyList l = KisLayer::sectionModelProperties();
    if (m_d->copyFrom)
        l << KisNodeModel::Property(i18n("Copy From"), m_d->copyFrom->name());
    return l;
}

void KisCloneLayer::syncLodCache()
{
    KisLayer::syncLodCache();
    m_d->offset.syncLodOffset();
}

