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

#include "XyzU8ColorSpace.h"
#include <QDomElement>

#include <QDebug>
#include <klocalizedstring.h>

#include "compositeops/KoCompositeOps.h"

XyzU8ColorSpace::XyzU8ColorSpace(const QString &name, KoColorProfile *p)
    : LcmsColorSpace<KoXyzU8Traits>(colorSpaceId(), name, TYPE_XYZA_8, cmsSigXYZData, p)
{
    addChannel(new KoChannelInfo(i18n("X"), KoXyzU8Traits::x_pos * sizeof(quint8), KoXyzU8Traits::x_pos, KoChannelInfo::COLOR, KoChannelInfo::UINT8, sizeof(quint8), Qt::cyan));
    addChannel(new KoChannelInfo(i18n("Y"), KoXyzU8Traits::y_pos * sizeof(quint8), KoXyzU8Traits::y_pos, KoChannelInfo::COLOR, KoChannelInfo::UINT8, sizeof(quint8), Qt::magenta));
    addChannel(new KoChannelInfo(i18n("Z"), KoXyzU8Traits::z_pos * sizeof(quint8), KoXyzU8Traits::z_pos, KoChannelInfo::COLOR, KoChannelInfo::UINT8, sizeof(quint8), Qt::yellow));
    addChannel(new KoChannelInfo(i18n("Alpha"), KoXyzU8Traits::alpha_pos * sizeof(quint8), KoXyzU8Traits::alpha_pos, KoChannelInfo::ALPHA, KoChannelInfo::UINT8, sizeof(quint8)));

    init();

    addStandardCompositeOps<KoXyzU8Traits>(this);
}

bool XyzU8ColorSpace::willDegrade(ColorSpaceIndependence /*independence*/) const
{
    return false;
}

KoColorSpace *XyzU8ColorSpace::clone() const
{
    return new XyzU8ColorSpace(name(), profile()->clone());
}

void XyzU8ColorSpace::colorToXML(const quint8 *pixel, QDomDocument &doc, QDomElement &colorElt) const
{
    const KoXyzU8Traits::Pixel *p = reinterpret_cast<const KoXyzU8Traits::Pixel *>(pixel);
    QDomElement labElt = doc.createElement("XYZ");
    labElt.setAttribute("x", KoColorSpaceMaths< KoXyzU8Traits::channels_type, qreal>::scaleToA(p->x));
    labElt.setAttribute("y", KoColorSpaceMaths< KoXyzU8Traits::channels_type, qreal>::scaleToA(p->y));
    labElt.setAttribute("z", KoColorSpaceMaths< KoXyzU8Traits::channels_type, qreal>::scaleToA(p->z));
    labElt.setAttribute("space", profile()->name());
    colorElt.appendChild(labElt);
}

void XyzU8ColorSpace::colorFromXML(quint8 *pixel, const QDomElement &elt) const
{
    KoXyzU8Traits::Pixel *p = reinterpret_cast<KoXyzU8Traits::Pixel *>(pixel);
    p->x = KoColorSpaceMaths< qreal, KoXyzU8Traits::channels_type >::scaleToA(elt.attribute("x").toDouble());
    p->y = KoColorSpaceMaths< qreal, KoXyzU8Traits::channels_type >::scaleToA(elt.attribute("y").toDouble());
    p->z = KoColorSpaceMaths< qreal, KoXyzU8Traits::channels_type >::scaleToA(elt.attribute("z").toDouble());
    p->alpha = KoColorSpaceMathsTraits<quint8>::max;
}

