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

#ifndef __KIS_MULTIPLE_PROJECTION_H
#define __KIS_MULTIPLE_PROJECTION_H

#include <QScopedPointer>
#include "kis_types.h"
#include "kritaimage_export.h"


class KRITAIMAGE_EXPORT KisMultipleProjection
{
public:
    KisMultipleProjection();
    ~KisMultipleProjection();

    static QString defaultProjectionId();

    KisPaintDeviceSP getProjection(const QString &id, const QString &compositeOpId, KisPaintDeviceSP prototype);
    void freeProjection(const QString &id);
    void freeAllProjections();

    void clear(const QRect &rc);

    void apply(KisPaintDeviceSP dstDevice, const QRect &rect);

    void syncLodCache();

private:
    struct Private;
    const QScopedPointer<Private> m_d;
};

#endif /* __KIS_MULTIPLE_PROJECTION_H */
