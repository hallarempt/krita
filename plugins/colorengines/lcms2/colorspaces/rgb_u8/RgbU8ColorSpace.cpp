/*
 *  Copyright (c) 2002 Patrick Julien  <freak@codepimps.org>
 *  Copyright (c) 2004 Boudewijn Rempt <boud@valdyas.org>
 *
 *  This library is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation; either version 2.1 of the License, or
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
#include "RgbU8ColorSpace.h"

#include <QColor>
#include <QDomElement>
#include <QImage>

#include <klocalizedstring.h>

#include <KoIntegerMaths.h>
#include <KoColorSpaceRegistry.h>
#include "compositeops/KoCompositeOps.h"
#include "compositeops/RgbCompositeOps.h"

#define downscale(quantum)  (quantum) //((unsigned char) ((quantum)/257UL))
#define upscale(value)  (value) // ((quint8) (257UL*(value)))

class KoRgbU8InvertColorTransformation : public KoColorTransformation
{

public:

    KoRgbU8InvertColorTransformation(const KoColorSpace *cs) : m_colorSpace(cs), m_psize(cs->pixelSize())
    {
    }

    virtual void transform(const quint8 *src, quint8 *dst, qint32 nPixels) const
    {
        while (nPixels--) {
            dst[0] = KoColorSpaceMathsTraits<quint8>::max - src[0];
            dst[1] = KoColorSpaceMathsTraits<quint8>::max - src[1];
            dst[2] = KoColorSpaceMathsTraits<quint8>::max - src[2];
            dst[3] = src[3];

            src += m_psize;
            dst += m_psize;
        }

    }

private:

    const KoColorSpace *m_colorSpace;
    quint32 m_psize;
};

RgbU8ColorSpace::RgbU8ColorSpace(const QString &name, KoColorProfile *p) :
    LcmsColorSpace<KoBgrU8Traits>(colorSpaceId(), name, TYPE_BGRA_8, cmsSigRgbData, p)
{
    addChannel(new KoChannelInfo(i18n("Blue"), 0, 2, KoChannelInfo::COLOR, KoChannelInfo::UINT8, 1, QColor(0, 0, 255)));
    addChannel(new KoChannelInfo(i18n("Green"), 1, 1, KoChannelInfo::COLOR, KoChannelInfo::UINT8, 1, QColor(0, 255, 0)));
    addChannel(new KoChannelInfo(i18n("Red"), 2, 0, KoChannelInfo::COLOR, KoChannelInfo::UINT8, 1, QColor(255, 0, 0)));
    addChannel(new KoChannelInfo(i18n("Alpha"), 3, 3, KoChannelInfo::ALPHA, KoChannelInfo::UINT8));

    init();

    addStandardCompositeOps<KoBgrU8Traits>(this);

    addCompositeOp(new RgbCompositeOpIn<KoBgrU8Traits>(this));
    addCompositeOp(new RgbCompositeOpOut<KoBgrU8Traits>(this));
    addCompositeOp(new RgbCompositeOpBumpmap<KoBgrU8Traits>(this));
}

KoColorTransformation *RgbU8ColorSpace::createInvertTransformation() const
{
    return new KoRgbU8InvertColorTransformation(this);
}

KoColorSpace *RgbU8ColorSpace::clone() const
{
    return new RgbU8ColorSpace(name(), profile()->clone());
}

void RgbU8ColorSpace::colorToXML(const quint8 *pixel, QDomDocument &doc, QDomElement &colorElt) const
{
    const KoBgrU8Traits::Pixel *p = reinterpret_cast<const KoBgrU8Traits::Pixel *>(pixel);
    QDomElement labElt = doc.createElement("RGB");
    labElt.setAttribute("r", KoColorSpaceMaths< KoBgrU8Traits::channels_type, qreal>::scaleToA(p->red));
    labElt.setAttribute("g", KoColorSpaceMaths< KoBgrU8Traits::channels_type, qreal>::scaleToA(p->green));
    labElt.setAttribute("b", KoColorSpaceMaths< KoBgrU8Traits::channels_type, qreal>::scaleToA(p->blue));
    labElt.setAttribute("space", profile()->name());
    colorElt.appendChild(labElt);
}

void RgbU8ColorSpace::colorFromXML(quint8 *pixel, const QDomElement &elt) const
{
    KoBgrU8Traits::Pixel *p = reinterpret_cast<KoBgrU8Traits::Pixel *>(pixel);
    p->red = KoColorSpaceMaths< qreal, KoBgrU8Traits::channels_type >::scaleToA(elt.attribute("r").toDouble());
    p->green = KoColorSpaceMaths< qreal, KoBgrU8Traits::channels_type >::scaleToA(elt.attribute("g").toDouble());
    p->blue = KoColorSpaceMaths< qreal, KoBgrU8Traits::channels_type >::scaleToA(elt.attribute("b").toDouble());
    p->alpha = KoColorSpaceMathsTraits<quint8>::max;
}

quint8 RgbU8ColorSpace::intensity8(const quint8 *src) const
{
    const KoBgrU8Traits::Pixel *p = reinterpret_cast<const KoBgrU8Traits::Pixel *>(src);
    return (quint8)(p->red * 0.30 + p->green * 0.59 + p->blue * 0.11);
}
