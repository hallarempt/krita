/*
 *  Copyright (c) 2006-2007, 2009 Cyrille Berger <cberger@cberger.net>
 *
 *  This library is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation; version 2.1 of the License.
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

#include "kis_color_source.h"
#include <kis_paint_device.h>

#include <KoAbstractGradient.h>
#include <KoColorSpaceRegistry.h>
#include <KoColorTransformation.h>
#include <KoMixColorsOp.h>
#include <kis_datamanager.h>
#include <kis_fill_painter.h>
#include "kis_iterator_ng.h"
#include "kis_selection.h"
#include "kis_random_source.h"
#include "kis_paint_information.h"

#include <random>


KisColorSource::~KisColorSource() { }

const KoColor black;

const KoColor& KisColorSource::uniformColor() const
{
    qFatal("Not an uniform color.");
    return black;
}


KisUniformColorSource::KisUniformColorSource() : m_color(0), m_cachedColor(0)
{
}

KisUniformColorSource::~KisUniformColorSource()
{
    delete m_color;
    delete m_cachedColor;
}

void KisUniformColorSource::rotate(double)
{}

void KisUniformColorSource::resize(double , double)
{
    // Do nothing as plain color does not have size
}

void KisUniformColorSource::colorize(KisPaintDeviceSP dev, const QRect& size, const QPoint&) const
{
    Q_UNUSED(size);
    if (!m_cachedColor || !(*dev->colorSpace() == *m_cachedColor->colorSpace())) {
        delete m_cachedColor;
        m_cachedColor = new KoColor(dev->colorSpace());
        m_cachedColor->fromKoColor(*m_color);
    }

    dev->dataManager()->setDefaultPixel(m_cachedColor->data());
    dev->clear();
}

const KoColor& KisUniformColorSource::uniformColor() const
{
    return *m_color;
}

void KisUniformColorSource::applyColorTransformation(const KoColorTransformation* transfo)
{
    transfo->transform(m_color->data(), m_color->data(), 1);
}

const KoColorSpace* KisUniformColorSource::colorSpace() const
{
    return m_color->colorSpace();
}

bool KisUniformColorSource::isUniformColor() const
{
    return true;
}

//-------------------------------------------------//
//---------------- KisPlainColorSource ---------------//
//-------------------------------------------------//

KisPlainColorSource::KisPlainColorSource(const KoColor& backGroundColor, const KoColor& foreGroundColor) : m_backGroundColor(backGroundColor), m_foreGroundColor(foreGroundColor), m_cachedBackGroundColor(0)
{
}

KisPlainColorSource::~KisPlainColorSource()
{
    delete m_cachedBackGroundColor;
}

void KisPlainColorSource::selectColor(double mix, const KisPaintInformation &pi)
{
    Q_UNUSED(pi);

    if (!m_color || !(*m_color->colorSpace() == *m_foreGroundColor.colorSpace())) {
        delete m_color;
        m_color = new KoColor(m_foreGroundColor.colorSpace());
        delete m_cachedBackGroundColor;
        m_cachedBackGroundColor = new KoColor(m_foreGroundColor.colorSpace());
        m_cachedBackGroundColor->fromKoColor(m_backGroundColor);
    }

    const quint8 * colors[2];
    colors[0] = m_cachedBackGroundColor->data();
    colors[1] = m_foreGroundColor.data();
    // equally distribute mix factor over [0..255]
    // mix * 256 ensures that, with exception of mix==1.0, which gets special handling
    const int weight = (mix == 1.0) ? 255 : (int)(mix * 256);
    const qint16 weights[2] = { (qint16)(255 - weight), (qint16)weight };

    m_color->colorSpace()->mixColorsOp()->mixColors(colors, weights, 2, m_color->data());
}

//-------------------------------------------------//
//--------------- KisGradientColorSource -------------//
//-------------------------------------------------//

KisGradientColorSource::KisGradientColorSource(const KoAbstractGradient* gradient, const KoColorSpace* workingCS)
    : m_gradient(gradient)
{
    m_color = new KoColor(workingCS);
    Q_ASSERT(gradient);
}

KisGradientColorSource::~KisGradientColorSource()
{
}

void KisGradientColorSource::selectColor(double mix, const KisPaintInformation &pi)
{
    Q_UNUSED(pi);
    m_gradient->colorAt(*m_color, mix);
}

//-------------------------------------------------//
//--------------- KisGradientColorSource -------------//
//-------------------------------------------------//

KisUniformRandomColorSource::KisUniformRandomColorSource()
{
    m_color = new KoColor();
}

KisUniformRandomColorSource::~KisUniformRandomColorSource()
{
}

void KisUniformRandomColorSource::selectColor(double mix, const KisPaintInformation &pi)
{
    Q_UNUSED(pi);
    Q_UNUSED(mix);

    KisRandomSourceSP source = pi.randomSource();
    m_color->fromQColor(QColor((int)source->generate(0, 255),
                               (int)source->generate(0, 255),
                               (int)source->generate(0, 255)));
}


//------------------------------------------------------//
//--------------- KisTotalRandomColorSource ---------------//
//------------------------------------------------------//

KisTotalRandomColorSource::KisTotalRandomColorSource() : m_colorSpace(KoColorSpaceRegistry::instance()->rgb8())
{
}

KisTotalRandomColorSource::~KisTotalRandomColorSource()
{
}

void KisTotalRandomColorSource::selectColor(double mix, const KisPaintInformation &pi)
{
    Q_UNUSED(mix);
    Q_UNUSED(pi);
}

void KisTotalRandomColorSource::applyColorTransformation(const KoColorTransformation*) {}
const KoColorSpace* KisTotalRandomColorSource::colorSpace() const
{
    return m_colorSpace;
}

void KisTotalRandomColorSource::colorize(KisPaintDeviceSP dev, const QRect& rect, const QPoint&) const
{
    KoColor kc(dev->colorSpace());

    QColor qc;

    std::random_device rand_dev;
    std::default_random_engine rand_engine{rand_dev()};
    std::uniform_int_distribution<> rand_distr(0, 255);

    int pixelSize = dev->colorSpace()->pixelSize();

    KisHLineIteratorSP it = dev->createHLineIteratorNG(rect.x(), rect.y(), rect.width());
    for (int y = 0; y < rect.height(); y++) {
        do {
            qc.setRgb(rand_distr(rand_engine), rand_distr(rand_engine), rand_distr(rand_engine));
            kc.fromQColor(qc);
            memcpy(it->rawData(), kc.data(), pixelSize);
        } while (it->nextPixel());
        it->nextRow();
    }

}

bool KisTotalRandomColorSource::isUniformColor() const
{
    return false;
}

void KisTotalRandomColorSource::rotate(double) {}
void KisTotalRandomColorSource::resize(double , double) {}



KoPatternColorSource::KoPatternColorSource(KisPaintDeviceSP _pattern, int _width, int _height, bool _locked)
    : m_device(_pattern)
    , m_bounds(QRect(0, 0, _width, _height))
    , m_locked(_locked)
{
}

KoPatternColorSource::~KoPatternColorSource()
{
}

void KoPatternColorSource::selectColor(double mix, const KisPaintInformation &pi)
{
    Q_UNUSED(mix);
    Q_UNUSED(pi);
}

void KoPatternColorSource::applyColorTransformation(const KoColorTransformation* transfo)
{
    Q_UNUSED(transfo);
}

const KoColorSpace* KoPatternColorSource::colorSpace() const
{
    return m_device->colorSpace();
}

void KoPatternColorSource::colorize(KisPaintDeviceSP device, const QRect& rect, const QPoint& offset) const
{
    KisFillPainter painter(device);
    if (m_locked) {
        painter.fillRect(rect.x(), rect.y(), rect.width(), rect.height(), m_device, m_bounds);
    }
    else {
        int x = offset.x() % m_bounds.width();
        int y = offset.y() % m_bounds.height();

        // Change the position, because the pattern is always applied starting
        // from (0,0) in the paint device reference
        device->setX(x);
        device->setY(y);
        painter.fillRect(rect.x() + x, rect.y() + y, rect.width(), rect.height(), m_device, m_bounds);
        device->setX(0);
        device->setY(0);
    }
}

void KoPatternColorSource::rotate(double r)
{
    Q_UNUSED(r);
}

void KoPatternColorSource::resize(double xs, double ys)
{
    Q_UNUSED(xs);
    Q_UNUSED(ys);
}

bool KoPatternColorSource::isUniformColor() const
{
    return false;
}

