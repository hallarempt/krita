/*
 *  Copyright (c) 2004-2006 Cyrille Berger <cberger@cberger.net>
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
#ifndef KIS_GRAY_COLORSPACE_H_
#define KIS_GRAY_COLORSPACE_H_

#include <klocalizedstring.h>
#include <LcmsColorSpace.h>
#include <KoColorSpaceTraits.h>
#include "KoColorModelStandardIds.h"

typedef KoColorSpaceTrait<quint8, 2, 1> GrayAU8Traits;

class GrayAU8ColorSpace : public LcmsColorSpace<GrayAU8Traits>
{
public:

    GrayAU8ColorSpace(const QString &name, KoColorProfile *p);

    virtual bool willDegrade(ColorSpaceIndependence) const
    {
        return false;
    }

    virtual KoID colorModelId() const
    {
        return GrayAColorModelID;
    }

    virtual KoID colorDepthId() const
    {
        return Integer8BitsColorDepthID;
    }

    virtual KoColorSpace *clone() const;

    virtual void colorToXML(const quint8 *pixel, QDomDocument &doc, QDomElement &colorElt) const;

    virtual void colorFromXML(quint8 *pixel, const QDomElement &elt) const;

    static QString colorSpaceId()
    {
        return "GRAYA";
    }

};

class GrayAU8ColorSpaceFactory : public LcmsColorSpaceFactory
{
public:
    GrayAU8ColorSpaceFactory()
        : LcmsColorSpaceFactory(TYPE_GRAYA_8, cmsSigGrayData)
    {
    }

    virtual QString id() const
    {
        return GrayAU8ColorSpace::colorSpaceId();
    }

    virtual QString name() const
    {
        return i18n("Grayscale (8-bit integer/channel)");
    }

    virtual KoID colorModelId() const
    {
        return GrayAColorModelID;
    }

    virtual KoID colorDepthId() const
    {
        return Integer8BitsColorDepthID;
    }

    virtual int referenceDepth() const
    {
        return 8;
    }

    virtual bool userVisible() const
    {
        return true;
    }

    virtual KoColorSpace *createColorSpace(const KoColorProfile *p) const
    {
        return new GrayAU8ColorSpace(name(), p->clone());
    }

    virtual QString defaultProfile() const
    {
        return "gray built-in";
    }
};

#endif // KIS_STRATEGY_COLORSPACE_GRAYSCALE_H_
