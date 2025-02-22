/*
 *  Copyright (c) 2006 Cyrille Berger <cberger@cberger.net>
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

#ifndef KORGBU16COLORSPACE_H_
#define KORGBU16COLORSPACE_H_

#include "LcmsColorSpace.h"
#include "KoColorSpaceTraits.h"
#include "KoColorModelStandardIds.h"

class RgbU16ColorSpace : public LcmsColorSpace<KoBgrU16Traits>
{
public:
    RgbU16ColorSpace(const QString &name, KoColorProfile *p);

    virtual bool willDegrade(ColorSpaceIndependence independence) const;

    virtual KoID colorModelId() const
    {
        return RGBAColorModelID;
    }

    virtual KoID colorDepthId() const
    {
        return Integer16BitsColorDepthID;
    }

    virtual KoColorSpace *clone() const;

    virtual void colorToXML(const quint8 *pixel, QDomDocument &doc, QDomElement &colorElt) const;

    virtual void colorFromXML(quint8 *pixel, const QDomElement &elt) const;

    static QString colorSpaceId()
    {
        return QString("RGBA16");
    }

};

class RgbU16ColorSpaceFactory : public LcmsColorSpaceFactory
{
public:
    RgbU16ColorSpaceFactory() : LcmsColorSpaceFactory(TYPE_BGRA_16, cmsSigRgbData)
    {
    }
    virtual QString id() const
    {
        return RgbU16ColorSpace::colorSpaceId();
    }
    virtual QString name() const
    {
        return i18n("RGB (16-bit integer/channel)");
    }

    virtual bool userVisible() const
    {
        return true;
    }
    virtual KoID colorModelId() const
    {
        return RGBAColorModelID;
    }
    virtual KoID colorDepthId() const
    {
        return Integer16BitsColorDepthID;
    }
    virtual int referenceDepth() const
    {
        return 16;
    }

    virtual KoColorSpace *createColorSpace(const KoColorProfile *p) const
    {
        return new RgbU16ColorSpace(name(), p->clone());
    }

    virtual QString defaultProfile() const
    {
        return "sRGB-elle-V2-g10.icc";//this is a linear space, because 16bit is enough to only enjoy advantages of linear space
    }
};

#endif
