/*
 *  Copyright (c) 2007 Boudewijn Rempt <boud@valdyas.org>
 *
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

#include <KoIcon.h>
#include <kis_icon.h>
#include <KoCompositeOpRegistry.h>

#include "kis_layer.h"
#include "kis_filter_mask.h"
#include "filter/kis_filter.h"
#include "filter/kis_filter_configuration.h"
#include "filter/kis_filter_registry.h"
#include "kis_selection.h"
#include "kis_processing_information.h"
#include "kis_node.h"
#include "kis_node_visitor.h"
#include "kis_processing_visitor.h"
#include "kis_busy_progress_indicator.h"
#include "kis_transaction.h"
#include "kis_painter.h"

KisFilterMask::KisFilterMask()
    : KisEffectMask(),
      KisNodeFilterInterface(0, false)
{
    setCompositeOpId(COMPOSITE_COPY);
}

KisFilterMask::~KisFilterMask()
{
}

KisFilterMask::KisFilterMask(const KisFilterMask& rhs)
        : KisEffectMask(rhs)
        , KisNodeFilterInterface(rhs)
{
}

QIcon KisFilterMask::icon() const
{
    return KisIconUtils::loadIcon("view-filter");
}

void KisFilterMask::setFilter(KisFilterConfiguration * filterConfig)
{
    if (parent() && parent()->inherits("KisLayer")) {
        filterConfig->setChannelFlags(qobject_cast<KisLayer*>(parent().data())->channelFlags());
    }
    KisNodeFilterInterface::setFilter(filterConfig);
}

QRect KisFilterMask::decorateRect(KisPaintDeviceSP &src,
                                  KisPaintDeviceSP &dst,
                                  const QRect & rc,
                                  PositionToFilthy maskPos) const
{
    Q_UNUSED(maskPos);

    KisSafeFilterConfigurationSP filterConfig = filter();

    Q_ASSERT(nodeProgressProxy());
    Q_ASSERT_X(src != dst, "KisFilterMask::decorateRect",
               "src must be != dst, because we cant create transactions "
               "during merge, as it breaks reentrancy");

    if (!filterConfig) {
        return QRect();
    }

    KisFilterSP filter =
        KisFilterRegistry::instance()->value(filterConfig->name());

    if (!filter) {
        warnKrita << "Could not retrieve filter \"" << filterConfig->name() << "\"";
        return QRect();
    }

    KIS_ASSERT_RECOVER_NOOP(this->busyProgressIndicator());
    this->busyProgressIndicator()->update();

    filter->process(src, dst, 0, rc, filterConfig.data(), 0);

    QRect r = filter->changedRect(rc, filterConfig.data(), dst->defaultBounds()->currentLevelOfDetail());
    return r;
}

bool KisFilterMask::accept(KisNodeVisitor &v)
{
    return v.visit(this);
}

void KisFilterMask::accept(KisProcessingVisitor &visitor, KisUndoAdapter *undoAdapter)
{
    return visitor.visit(this, undoAdapter);
}

/**
 * FIXME: try to cache filter pointer inside a Private block
 */
QRect KisFilterMask::changeRect(const QRect &rect, PositionToFilthy pos) const
{
    /**
     * FIXME: This check of the emptiness should be done
     * on the higher/lower level
     */
    if(rect.isEmpty()) return rect;

    QRect filteredRect = rect;

    KisSafeFilterConfigurationSP filterConfig = filter();
    if (filterConfig) {
        KisNodeSP parent = this->parent();
        const int lod = parent && parent->projection() ?
            parent->projection()->defaultBounds()->currentLevelOfDetail() : 0;

        KisFilterSP filter = KisFilterRegistry::instance()->value(filterConfig->name());
        filteredRect = filter->changedRect(rect, filterConfig.data(), lod);
    }

    /**
     * We can't paint outside a selection, that is why we call
     * KisMask::changeRect to crop actual change area in the end
     */
    filteredRect = KisMask::changeRect(filteredRect, pos);
    /**
     * FIXME: Think over this solution
     * Union of rects means that changeRect returns NOT the rect
     * changed by this very layer, but an accumulated rect changed
     * by all underlying layers. Just take into account and change
     * documentation accordingly
     */
    return rect | filteredRect;
}

QRect KisFilterMask::needRect(const QRect& rect, PositionToFilthy pos) const
{
    Q_UNUSED(pos);

    /**
     * FIXME: This check of the emptiness should be done
     * on the higher/lower level
     */

    if(rect.isEmpty()) return rect;

    KisSafeFilterConfigurationSP filterConfig = filter();
    if (!filterConfig) return rect;

    KisNodeSP parent = this->parent();
    const int lod = parent && parent->projection() ?
        parent->projection()->defaultBounds()->currentLevelOfDetail() : 0;

    KisFilterSP filter = KisFilterRegistry::instance()->value(filterConfig->name());

    /**
     * If we need some additional pixels even outside of a selection
     * for accurate layer filtering, we'll get them!
     * And no KisMask::needRect will prevent us from doing this! ;)
     * That's why simply we do not call KisMask::needRect here :)
     */
    return filter->neededRect(rect, filterConfig.data(), lod);
}

