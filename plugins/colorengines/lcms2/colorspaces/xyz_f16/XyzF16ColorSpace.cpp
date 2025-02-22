/*
 *  Copyright (c) 2007 Cyrille Berger (cberger@cberger.net)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
*/

#include "XyzF16ColorSpace.h"
#include <QDomElement>

#include <QDebug>
#include <klocalizedstring.h>

#include "compositeops/KoCompositeOps.h"

XyzF16ColorSpace::XyzF16ColorSpace(const QString &name, KoColorProfile *p) :
    LcmsColorSpace<KoXyzF16Traits>(colorSpaceId(), name, TYPE_XYZA_HALF_FLT, cmsSigXYZData, p)
{
    addChannel(new KoChannelInfo(i18n("X"),     KoXyzF16Traits::x_pos     * sizeof(half), KoXyzF16Traits::x_pos,     KoChannelInfo::COLOR, KoChannelInfo::FLOAT16, 2, Qt::cyan));
    addChannel(new KoChannelInfo(i18n("Y"),     KoXyzF16Traits::y_pos     * sizeof(half), KoXyzF16Traits::y_pos,     KoChannelInfo::COLOR, KoChannelInfo::FLOAT16, 2, Qt::magenta));
    addChannel(new KoChannelInfo(i18n("Z"),     KoXyzF16Traits::z_pos     * sizeof(half), KoXyzF16Traits::z_pos,     KoChannelInfo::COLOR, KoChannelInfo::FLOAT16, 2, Qt::yellow));
    addChannel(new KoChannelInfo(i18n("Alpha"), KoXyzF16Traits::alpha_pos * sizeof(half), KoXyzF16Traits::alpha_pos, KoChannelInfo::ALPHA, KoChannelInfo::FLOAT16, 2));

    init();

    addStandardCompositeOps<KoXyzF16Traits>(this);
}

bool XyzF16ColorSpace::willDegrade(ColorSpaceIndependence independence) const
{
    if (independence == TO_RGBA16) {
        return true;
    } else {
        return false;
    }
}

KoColorSpace *XyzF16ColorSpace::clone() const
{
    return new XyzF16ColorSpace(name(), profile()->clone());
}

void XyzF16ColorSpace::colorToXML(const quint8 *pixel, QDomDocument &doc, QDomElement &colorElt) const
{
    const KoXyzF16Traits::Pixel *p = reinterpret_cast<const KoXyzF16Traits::Pixel *>(pixel);
    QDomElement labElt = doc.createElement("XYZ");
    labElt.setAttribute("x", KoColorSpaceMaths< KoXyzF16Traits::channels_type, qreal>::scaleToA(p->x));
    labElt.setAttribute("y", KoColorSpaceMaths< KoXyzF16Traits::channels_type, qreal>::scaleToA(p->y));
    labElt.setAttribute("z", KoColorSpaceMaths< KoXyzF16Traits::channels_type, qreal>::scaleToA(p->z));
    labElt.setAttribute("space", profile()->name());
    colorElt.appendChild(labElt);
}

void XyzF16ColorSpace::colorFromXML(quint8 *pixel, const QDomElement &elt) const
{
    KoXyzF16Traits::Pixel *p = reinterpret_cast<KoXyzF16Traits::Pixel *>(pixel);
    p->x = KoColorSpaceMaths< qreal, KoXyzF16Traits::channels_type >::scaleToA(elt.attribute("x").toDouble());
    p->y = KoColorSpaceMaths< qreal, KoXyzF16Traits::channels_type >::scaleToA(elt.attribute("y").toDouble());
    p->z = KoColorSpaceMaths< qreal, KoXyzF16Traits::channels_type >::scaleToA(elt.attribute("z").toDouble());
    p->alpha = 1.0;
}

