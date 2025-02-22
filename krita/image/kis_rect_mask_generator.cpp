/*
 *  Copyright (c) 2004,2007,2008,2009.2010 Cyrille Berger <cberger@cberger.net>
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

#include <KoVcMultiArchBuildSupport.h> //MSVC requires that Vc come first
#include <cmath>

#include <QDomDocument>

#include "kis_fast_math.h"

#include "kis_rect_mask_generator.h"
#include "kis_base_mask_generator.h"

#ifdef Q_OS_WIN
#include <float.h>
#ifndef __MINGW32__
#define isnan _isnan
#endif
#endif

struct Q_DECL_HIDDEN KisRectangleMaskGenerator::Private {
    double m_c;
    qreal xcoeff;
    qreal ycoeff;
    qreal xfadecoeff;
    qreal yfadecoeff;
    qreal transformedFadeX;
    qreal transformedFadeY;
};

KisRectangleMaskGenerator::KisRectangleMaskGenerator(qreal radius, qreal ratio, qreal fh, qreal fv, int spikes, bool antialiasEdges)
    : KisMaskGenerator(radius, ratio, fh, fv, spikes, antialiasEdges, RECTANGLE, DefaultId), d(new Private)
{
    if (fv == 0 && fh == 0) {
        d->m_c = 0;
    } else {
        d->m_c = (fv / fh);
#ifdef Q_CC_MSVC
        Q_ASSERT(!isnan(d->m_c));
#else
        Q_ASSERT(!std::isnan(d->m_c));
#endif

    }

    setScale(1.0, 1.0);
}

void KisRectangleMaskGenerator::setScale(qreal scaleX, qreal scaleY)
{
    KisMaskGenerator::setScale(scaleX, scaleY);

    d->xcoeff = 2.0 / effectiveSrcWidth();
    d->ycoeff = 2.0 / effectiveSrcHeight();
    d->xfadecoeff = (horizontalFade() == 0) ? 1 : (2.0 / (horizontalFade() * effectiveSrcWidth()));
    d->yfadecoeff = (verticalFade() == 0)   ? 1 : (2.0 / (verticalFade() * effectiveSrcHeight()));
    setSoftness(this->softness());
}

void KisRectangleMaskGenerator::setSoftness(qreal softness)
{
    KisMaskGenerator::setSoftness(softness);
    qreal safeSoftnessCoeff = qreal(1.0) / qMax(qreal(0.01), softness);

    d->transformedFadeX = d->xfadecoeff * safeSoftnessCoeff;
    d->transformedFadeY = d->yfadecoeff * safeSoftnessCoeff;
}

KisRectangleMaskGenerator::~KisRectangleMaskGenerator()
{
    delete d;
}

bool KisRectangleMaskGenerator::shouldSupersample() const
{
    return effectiveSrcWidth() < 10 || effectiveSrcHeight() < 10;
}

quint8 KisRectangleMaskGenerator::valueAt(qreal x, qreal y) const
{
    if (isEmpty()) return 255;
    qreal xr = qAbs(x /*- m_xcenter*/);
    qreal yr = qAbs(y /*- m_ycenter*/);
    fixRotation(xr, yr);

    xr = qAbs(xr);
    yr = qAbs(yr);

    qreal nxr = xr * d->xcoeff;
    qreal nyr = yr * d->ycoeff;

    if (nxr > 1.0 || nyr > 1.0) return 255;

    if (antialiasEdges()) {
        xr += 1.0;
        yr += 1.0;
    }

    qreal fxr = xr * d->transformedFadeX;
    qreal fyr = yr * d->transformedFadeY;

    if (fxr > 1.0 && (fxr > fyr || fyr < 1.0)) {
        return 255 * nxr * (fxr - 1.0) / (fxr - nxr);
    }

    if (fyr > 1.0 && (fyr > fxr || fxr < 1.0)) {
        return 255 * nyr * (fyr - 1.0) / (fyr - nyr);
    }

    return 0;
}

