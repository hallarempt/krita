/*
 *  Copyright (c) 2006 Cyrille Berger <cberger@cberger.net>
 *  Copyright (c) 2007 Emanuele Tamponi <emanuele@valinor.it>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
*/

#ifndef KOCOLORSPACEABSTRACT_H
#define KOCOLORSPACEABSTRACT_H

#include <QBitArray>
#include <klocalizedstring.h>

#include <KoColorSpace.h>
#include <KoColorProfile.h>
#include "KoColorSpaceConstants.h"
#include <KoColorSpaceMaths.h>
#include <KoColorSpaceRegistry.h>
#include <KoIntegerMaths.h>
#include "KoColorTransformation.h"
#include "KoFallBackColorTransformation.h"
#include "KoLabDarkenColorTransformation.h"
#include "KoMixColorsOpImpl.h"

#include "KoConvolutionOpImpl.h"
#include "KoInvertColorTransformation.h"

/**
 * This in an implementation of KoColorSpace which can be used as a base for colorspaces with as many
 * different channels of the same type.
 *
 * The template parameters must be a class which inherits KoColorSpaceTrait (or a class with the same signature).
 *
 * SOMETYPE is the type of the channel for instance (quint8, quint32...),
 * SOMENBOFCHANNELS is the number of channels including the alpha channel
 * SOMEALPHAPOS is the position of the alpha channel in the pixel (can be equal to -1 if no alpha channel).
 */
template<class _CSTrait>
class KoColorSpaceAbstract : public KoColorSpace
{
public:
    KoColorSpaceAbstract(const QString &id, const QString &name) :
        KoColorSpace(id, name, new KoMixColorsOpImpl< _CSTrait>(), new KoConvolutionOpImpl< _CSTrait>()) {
    }

    virtual quint32 colorChannelCount() const {
        if (_CSTrait::alpha_pos == -1)
            return _CSTrait::channels_nb;
        else
            return _CSTrait::channels_nb - 1;
    }

    virtual quint32 channelCount() const {
        return _CSTrait::channels_nb;
    }

    virtual quint32 pixelSize() const {
        return _CSTrait::pixelSize;
    }

    virtual QString channelValueText(const quint8 *pixel, quint32 channelIndex) const {
        return _CSTrait::channelValueText(pixel, channelIndex);
    }

    virtual QString normalisedChannelValueText(const quint8 *pixel, quint32 channelIndex) const {
        return _CSTrait::normalisedChannelValueText(pixel, channelIndex);
    }

    virtual void normalisedChannelsValue(const quint8 *pixel, QVector<float> &channels) const {
        return _CSTrait::normalisedChannelsValue(pixel, channels);
    }

    virtual void fromNormalisedChannelsValue(quint8 *pixel, const QVector<float> &values) const {
        return _CSTrait::fromNormalisedChannelsValue(pixel, values);
    }

    virtual quint8 scaleToU8(const quint8 * srcPixel, qint32 channelIndex) const {
        typename _CSTrait::channels_type c = _CSTrait::nativeArray(srcPixel)[channelIndex];
        return KoColorSpaceMaths<typename _CSTrait::channels_type, quint8>::scaleToA(c);
    }

    virtual void singleChannelPixel(quint8 *dstPixel, const quint8 *srcPixel, quint32 channelIndex) const {
        _CSTrait::singleChannelPixel(dstPixel, srcPixel, channelIndex);
    }

    virtual quint8 opacityU8(const quint8 * U8_pixel) const {
        return _CSTrait::opacityU8(U8_pixel);
    }

    virtual qreal opacityF(const quint8 * U8_pixel) const {
        return _CSTrait::opacityF(U8_pixel);
    }

    virtual void setOpacity(quint8 * pixels, quint8 alpha, qint32 nPixels) const {
        _CSTrait::setOpacity(pixels, alpha, nPixels);
    }

    virtual void setOpacity(quint8 * pixels, qreal alpha, qint32 nPixels) const {
        _CSTrait::setOpacity(pixels, alpha, nPixels);
    }

    virtual void multiplyAlpha(quint8 * pixels, quint8 alpha, qint32 nPixels) const {
        _CSTrait::multiplyAlpha(pixels, alpha, nPixels);
    }

    virtual void applyAlphaU8Mask(quint8 * pixels, const quint8 * alpha, qint32 nPixels) const {
        _CSTrait::applyAlphaU8Mask(pixels, alpha, nPixels);
    }

    virtual void applyInverseAlphaU8Mask(quint8 * pixels, const quint8 * alpha, qint32 nPixels) const {
        _CSTrait::applyInverseAlphaU8Mask(pixels, alpha, nPixels);
    }

    virtual void applyAlphaNormedFloatMask(quint8 * pixels, const float * alpha, qint32 nPixels) const {
        _CSTrait::applyAlphaNormedFloatMask(pixels, alpha, nPixels);
    }

    virtual void applyInverseNormedFloatMask(quint8 * pixels, const float * alpha, qint32 nPixels) const {
        _CSTrait::applyInverseAlphaNormedFloatMask(pixels, alpha, nPixels);
    }

    virtual quint8 intensity8(const quint8 * src) const {
        QColor c;
        const_cast<KoColorSpaceAbstract<_CSTrait> *>(this)->toQColor(src, &c);
        return static_cast<quint8>((c.red() * 0.30 + c.green() * 0.59 + c.blue() * 0.11) + 0.5);
    }

    virtual KoColorTransformation* createInvertTransformation() const {
        return new KoInvertColorTransformation(this);
    }

    virtual KoColorTransformation *createDarkenAdjustment(qint32 shade, bool compensate, qreal compensation) const {
        return new KoFallBackColorTransformation(this, KoColorSpaceRegistry::instance()->lab16(""), new KoLabDarkenColorTransformation<quint16>(shade, compensate, compensation, KoColorSpaceRegistry::instance()->lab16("")));
    }

    virtual bool convertPixelsTo(const quint8 *src,
                                 quint8 *dst, const KoColorSpace *dstColorSpace,
                                 quint32 numPixels,
                                 KoColorConversionTransformation::Intent renderingIntent,
                                 KoColorConversionTransformation::ConversionFlags conversionFlags) const
    {
        
        // check whether we have the same profile and color model, but only a different bit
        // depth; in that case we don't convert as such, but scale
        bool scaleOnly = false;

        // Note: getting the id() is really, really expensive, so only do that if
        // we are sure there is a difference between the colorspaces
        if (!(*this == *dstColorSpace)) {
            scaleOnly = dstColorSpace->colorModelId().id() == colorModelId().id() &&
                         dstColorSpace->colorDepthId().id() != colorDepthId().id() &&
                         dstColorSpace->profile()->name()   == profile()->name();
        }
        
        if (scaleOnly && dynamic_cast<const KoColorSpaceAbstract*>(dstColorSpace)) {
            typedef typename _CSTrait::channels_type channels_type;
            
            switch(dstColorSpace->channels()[0]->channelValueType())
            {
            case KoChannelInfo::UINT8:
                scalePixels<_CSTrait::pixelSize, 1, channels_type, quint8>(src, dst, numPixels);
                return true;
//             case KoChannelInfo::INT8:
//                 scalePixels<_CSTrait::pixelSize, 1, channels_type, qint8>(src, dst, numPixels);
//                 return true;
            case KoChannelInfo::UINT16:
                scalePixels<_CSTrait::pixelSize, 2, channels_type, quint16>(src, dst, numPixels);
                return true;
            case KoChannelInfo::INT16:
                scalePixels<_CSTrait::pixelSize, 2, channels_type, qint16>(src, dst, numPixels);
                return true;
            case KoChannelInfo::UINT32:
                scalePixels<_CSTrait::pixelSize, 4, channels_type, quint32>(src, dst, numPixels);
                return true;
            default:
                break;
            }
        }
        
        return KoColorSpace::convertPixelsTo(src, dst, dstColorSpace, numPixels, renderingIntent, conversionFlags);
    }
    
private:
    template<int srcPixelSize, int dstChannelSize, class TSrcChannel, class TDstChannel>
    void scalePixels(const quint8* src, quint8* dst, quint32 numPixels) const {
        qint32 dstPixelSize = dstChannelSize * _CSTrait::channels_nb;
        
        for(quint32 i=0; i<numPixels; ++i) {
            const TSrcChannel* srcPixel = reinterpret_cast<const TSrcChannel*>(src + i * srcPixelSize);
            TDstChannel*       dstPixel = reinterpret_cast<TDstChannel*>(dst + i * dstPixelSize);
            
            for(quint32 c=0; c<_CSTrait::channels_nb; ++c)
                dstPixel[c] = Arithmetic::scale<TDstChannel>(srcPixel[c]);
        }
    }
};

#endif // KOCOLORSPACEABSTRACT_H
