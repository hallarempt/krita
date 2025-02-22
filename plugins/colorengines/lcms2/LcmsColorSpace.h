/*
 *  Copyright (c) 2002 Patrick Julien  <freak@codepimps.org>
 *  Copyright (c) 2005-2006 C. Boemann <cbo@boemann.dk>
 *  Copyright (c) 2004,2006-2007 Cyrille Berger <cberger@cberger.net>
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

#ifndef KOLCMSCOLORSPACE_H_
#define KOLCMSCOLORSPACE_H_

#include <colorprofiles/LcmsColorProfileContainer.h>
#include <KoColorSpaceAbstract.h>
#include <KoColorSpaceRegistry.h>

class KoLcmsInfo
{
    struct Private {
        cmsUInt32Number cmType;  // The colorspace type as defined by littlecms
        cmsColorSpaceSignature colorSpaceSignature; // The colorspace signature as defined in icm/icc files
    };

public:

    KoLcmsInfo(cmsUInt32Number cmType, cmsColorSpaceSignature colorSpaceSignature)
        : d(new Private)
    {
        d->cmType = cmType;
        d->colorSpaceSignature = colorSpaceSignature;
    }

    virtual ~KoLcmsInfo()
    {
        delete d;
    }

    virtual quint32 colorSpaceType() const
    {
        return d->cmType;
    }

    virtual cmsColorSpaceSignature colorSpaceSignature() const
    {
        return d->colorSpaceSignature;
    }

private:
    Private *const d;
};

struct KoLcmsDefaultTransformations {
    cmsHTRANSFORM toRGB;
    cmsHTRANSFORM fromRGB;
    static cmsHPROFILE s_RGBProfile;
    static QMap< QString, QMap< LcmsColorProfileContainer *, KoLcmsDefaultTransformations * > > s_transformations;
};

/**
 * This is the base class for all colorspaces that are based on the lcms library, for instance
 * RGB 8bits and 16bits, CMYK 8bits and 16bits, LAB...
 */
template<class _CSTraits>
class LcmsColorSpace : public KoColorSpaceAbstract<_CSTraits>, public KoLcmsInfo
{
    struct KoLcmsColorTransformation : public KoColorTransformation {

        KoLcmsColorTransformation(const KoColorSpace *colorSpace)
            : KoColorTransformation()
            , m_colorSpace(colorSpace)
        {
            csProfile = 0;
            cmstransform = 0;
            cmsAlphaTransform = 0;
            profiles[0] = 0;
            profiles[1] = 0;
            profiles[2] = 0;
        }

        ~KoLcmsColorTransformation()
        {

            if (cmstransform) {
                cmsDeleteTransform(cmstransform);
            }
            if (profiles[0] && profiles[0] != csProfile) {
                cmsCloseProfile(profiles[0]);
            }
            if (profiles[1] && profiles[1] != csProfile) {
                cmsCloseProfile(profiles[1]);
            }
            if (profiles[2] && profiles[2] != csProfile) {
                cmsCloseProfile(profiles[2]);
            }
        }

        virtual void transform(const quint8 *src, quint8 *dst, qint32 nPixels) const
        {
            cmsDoTransform(cmstransform, const_cast<quint8 *>(src), dst, nPixels);

            qint32 numPixels = nPixels;
            qint32 pixelSize = m_colorSpace->pixelSize();
            int index = 0;

            if (cmsAlphaTransform) {
                qreal *alpha = new qreal[nPixels];
                qreal *dstalpha = new qreal[nPixels];

                while (index < nPixels) {
                    alpha[index] = m_colorSpace->opacityF(src);
                    src += pixelSize;
                    index++;
                }

                cmsDoTransform(cmsAlphaTransform, const_cast<qreal *>(alpha), static_cast<qreal *>(dstalpha), nPixels);
                for (int i = 0; i < numPixels; i++) {
                    m_colorSpace->setOpacity(dst, dstalpha[i], 1);
                    dst += pixelSize;
                }

                delete [] alpha;
                delete [] dstalpha;
            } else {
                while (numPixels > 0) {
                    qreal alpha = m_colorSpace->opacityF(src);
                    m_colorSpace->setOpacity(dst, alpha, 1);
                    src += pixelSize;
                    dst += pixelSize;
                    numPixels--;
                }
            }
        }

        const KoColorSpace *m_colorSpace;
        cmsHPROFILE csProfile;
        cmsHPROFILE profiles[3];
        cmsHTRANSFORM cmstransform;
        cmsHTRANSFORM cmsAlphaTransform;
    };

    struct Private {
        mutable quint8 *qcolordata; // A small buffer for conversion from and to qcolor.
        KoLcmsDefaultTransformations *defaultTransformations;

        mutable cmsHPROFILE   lastRGBProfile;  // Last used profile to transform to/from RGB
        mutable cmsHTRANSFORM lastToRGB;       // Last used transform to transform to RGB
        mutable cmsHTRANSFORM lastFromRGB;     // Last used transform to transform from RGB
        LcmsColorProfileContainer *profile;
        KoColorProfile *colorProfile;
    };

protected:

    LcmsColorSpace(const QString &id,
                   const QString &name,
                   cmsUInt32Number cmType,
                   cmsColorSpaceSignature colorSpaceSignature,
                   KoColorProfile *p)
        : KoColorSpaceAbstract<_CSTraits>(id, name)
        , KoLcmsInfo(cmType, colorSpaceSignature)
        , d(new Private())
    {
        Q_ASSERT(p); // No profile means the lcms color space can't work
        Q_ASSERT(profileIsCompatible(p));
        d->profile = asLcmsProfile(p);
        Q_ASSERT(d->profile);
        d->colorProfile = p;
        d->qcolordata = 0;
        d->lastRGBProfile = 0;
        d->lastToRGB = 0;
        d->lastFromRGB = 0;
        d->defaultTransformations = 0;
    }

    virtual ~LcmsColorSpace()
    {
        delete d->colorProfile;
        delete[] d->qcolordata;
        delete d;
    }

    void init()
    {
        // Default pixel buffer for QColor conversion
        d->qcolordata = new quint8[3];
        Q_CHECK_PTR(d->qcolordata);

        Q_ASSERT(d->profile);

        if (KoLcmsDefaultTransformations::s_RGBProfile == 0) {
            KoLcmsDefaultTransformations::s_RGBProfile = cmsCreate_sRGBProfile();
        }
        d->defaultTransformations = KoLcmsDefaultTransformations::s_transformations[this->id()][ d->profile];
        if (!d->defaultTransformations) {
            d->defaultTransformations = new KoLcmsDefaultTransformations;
            d->defaultTransformations->fromRGB = cmsCreateTransform(KoLcmsDefaultTransformations::s_RGBProfile,
                                                 TYPE_BGR_8,
                                                 d->profile->lcmsProfile(),
                                                 this->colorSpaceType(),
                                                 KoColorConversionTransformation::internalRenderingIntent(),
                                                 KoColorConversionTransformation::internalConversionFlags());
            Q_ASSERT(d->defaultTransformations->fromRGB);
            d->defaultTransformations->toRGB = cmsCreateTransform(d->profile->lcmsProfile(),
                                               this->colorSpaceType(),
                                               KoLcmsDefaultTransformations::s_RGBProfile,
                                               TYPE_BGR_8,
                                               KoColorConversionTransformation::internalRenderingIntent(),
                                               KoColorConversionTransformation::internalConversionFlags());
            Q_ASSERT(d->defaultTransformations->toRGB);
            KoLcmsDefaultTransformations::s_transformations[ this->id()][ d->profile ] = d->defaultTransformations;
        }
    }

public:

    virtual bool hasHighDynamicRange() const
    {
        return false;
    }

    virtual const KoColorProfile *profile() const
    {
        return d->colorProfile;
    }

    virtual bool profileIsCompatible(const KoColorProfile *profile) const
    {
        const IccColorProfile *p = dynamic_cast<const IccColorProfile *>(profile);
        return (p && p->asLcms()->colorSpaceSignature() == colorSpaceSignature());
    }

    virtual void fromQColor(const QColor &color, quint8 *dst, const KoColorProfile *koprofile = 0) const
    {
        d->qcolordata[2] = color.red();
        d->qcolordata[1] = color.green();
        d->qcolordata[0] = color.blue();

        LcmsColorProfileContainer *profile = asLcmsProfile(koprofile);
        if (profile == 0) {
            // Default sRGB
            Q_ASSERT(d->defaultTransformations && d->defaultTransformations->fromRGB);

            cmsDoTransform(d->defaultTransformations->fromRGB, d->qcolordata, dst, 1);
        } else {
            if (d->lastFromRGB == 0 || (d->lastFromRGB != 0 && d->lastRGBProfile != profile->lcmsProfile())) {
                d->lastFromRGB = cmsCreateTransform(profile->lcmsProfile(),
                                                    TYPE_BGR_8,
                                                    d->profile->lcmsProfile(),
                                                    this->colorSpaceType(),
                                                    KoColorConversionTransformation::internalRenderingIntent(),
                                                    KoColorConversionTransformation::internalConversionFlags());
                d->lastRGBProfile = profile->lcmsProfile();

            }
            cmsDoTransform(d->lastFromRGB, d->qcolordata, dst, 1);
        }

        this->setOpacity(dst, (quint8)(color.alpha()), 1);
    }

    virtual void toQColor(const quint8 *src, QColor *c, const KoColorProfile *koprofile = 0) const
    {
        LcmsColorProfileContainer *profile = asLcmsProfile(koprofile);
        if (profile == 0) {
            // Default sRGB transform
            Q_ASSERT(d->defaultTransformations && d->defaultTransformations->toRGB);
            cmsDoTransform(d->defaultTransformations->toRGB, const_cast <quint8 *>(src), d->qcolordata, 1);
        } else {
            if (d->lastToRGB == 0 || (d->lastToRGB != 0 && d->lastRGBProfile != profile->lcmsProfile())) {
                d->lastToRGB = cmsCreateTransform(d->profile->lcmsProfile(), this->colorSpaceType(),
                                                  profile->lcmsProfile(), TYPE_BGR_8,
                                                  KoColorConversionTransformation::internalRenderingIntent(),
                                                  KoColorConversionTransformation::internalConversionFlags());
                d->lastRGBProfile = profile->lcmsProfile();
            }
            cmsDoTransform(d->lastToRGB, const_cast <quint8 *>(src), d->qcolordata, 1);
        }
        c->setRgb(d->qcolordata[2], d->qcolordata[1], d->qcolordata[0]);
        c->setAlpha(this->opacityU8(src));
    }

    virtual KoColorTransformation *createBrightnessContrastAdjustment(const quint16 *transferValues) const
    {
        if (!d->profile) {
            return 0;
        }

        cmsToneCurve *transferFunctions[3];
        transferFunctions[0] = cmsBuildTabulatedToneCurve16(0, 256, transferValues);
        transferFunctions[1] = cmsBuildGamma(0, 1.0);
        transferFunctions[2] = cmsBuildGamma(0, 1.0);

        KoLcmsColorTransformation *adj = new KoLcmsColorTransformation(this);
        adj->profiles[1] = cmsCreateLinearizationDeviceLink(cmsSigLabData, transferFunctions);
        cmsSetDeviceClass(adj->profiles[1], cmsSigAbstractClass);

        adj->profiles[0] = d->profile->lcmsProfile();
        adj->profiles[2] = d->profile->lcmsProfile();
        adj->cmstransform  = cmsCreateMultiprofileTransform(adj->profiles, 3, this->colorSpaceType(), this->colorSpaceType(),
                             KoColorConversionTransformation::adjustmentRenderingIntent(),
                             KoColorConversionTransformation::adjustmentConversionFlags());
        adj->csProfile = d->profile->lcmsProfile();
        return adj;
    }

    virtual KoColorTransformation *createPerChannelAdjustment(const quint16 *const *transferValues) const
    {
        if (!d->profile) {
            return 0;
        }

        cmsToneCurve **transferFunctions = new cmsToneCurve*[ this->colorChannelCount()];

        for (uint ch = 0; ch < this->colorChannelCount(); ch++) {
            transferFunctions[ch] = transferValues[ch] ?
                                    cmsBuildTabulatedToneCurve16(0, 256, transferValues[ch]) :
                                    cmsBuildGamma(0, 1.0);
        }

        cmsToneCurve **alphaTransferFunctions = new cmsToneCurve*[1];
        alphaTransferFunctions[0] = transferValues[this->colorChannelCount()] ?
                                    cmsBuildTabulatedToneCurve16(0, 256, transferValues[this->colorChannelCount()]) :
                                    cmsBuildGamma(0, 1.0);

        KoLcmsColorTransformation *adj = new KoLcmsColorTransformation(this);
        adj->profiles[0] = cmsCreateLinearizationDeviceLink(this->colorSpaceSignature(), transferFunctions);
        adj->profiles[1] = cmsCreateLinearizationDeviceLink(cmsSigGrayData, alphaTransferFunctions);
        adj->profiles[2] = NULL;
        adj->csProfile = d->profile->lcmsProfile();
        adj->cmstransform  = cmsCreateTransform(adj->profiles[0], this->colorSpaceType(), NULL, this->colorSpaceType(),
                                                KoColorConversionTransformation::adjustmentRenderingIntent(),
                                                KoColorConversionTransformation::adjustmentConversionFlags());

        adj->cmsAlphaTransform  = cmsCreateTransform(adj->profiles[1], TYPE_GRAY_DBL, NULL, TYPE_GRAY_DBL,
                                  KoColorConversionTransformation::adjustmentRenderingIntent(),
                                  KoColorConversionTransformation::adjustmentConversionFlags());

        delete [] transferFunctions;
        delete [] alphaTransferFunctions;
        return adj;
    }

    virtual quint8 difference(const quint8 *src1, const quint8 *src2) const
    {
        quint8 lab1[8], lab2[8];
        cmsCIELab labF1, labF2;

        if (this->opacityU8(src1) == OPACITY_TRANSPARENT_U8
                || this->opacityU8(src2) == OPACITY_TRANSPARENT_U8) {
            return (this->opacityU8(src1) == this->opacityU8(src2) ? 0 : 255);
        }
        Q_ASSERT(this->toLabA16Converter());
        this->toLabA16Converter()->transform(src1, lab1, 1);
        this->toLabA16Converter()->transform(src2, lab2, 1);
        cmsLabEncoded2Float(&labF1, (cmsUInt16Number *)lab1);
        cmsLabEncoded2Float(&labF2, (cmsUInt16Number *)lab2);
        qreal diff = cmsDeltaE(&labF1, &labF2);

        if (diff > 255.0) {
            return 255;
        } else {
            return quint8(diff);
        }
    }

    virtual quint8 differenceA(const quint8 *src1, const quint8 *src2) const
    {
        quint8 lab1[8];
        quint8 lab2[8];
        cmsCIELab labF1;
        cmsCIELab labF2;

        if (this->opacityU8(src1) == OPACITY_TRANSPARENT_U8
                || this->opacityU8(src2) == OPACITY_TRANSPARENT_U8) {
            return (this->opacityU8(src1) == this->opacityU8(src2) ? 0 : 255);
        }
        Q_ASSERT(this->toLabA16Converter());
        this->toLabA16Converter()->transform(src1, lab1, 1);
        this->toLabA16Converter()->transform(src2, lab2, 1);
        cmsLabEncoded2Float(&labF1, (cmsUInt16Number *)lab1);
        cmsLabEncoded2Float(&labF2, (cmsUInt16Number *)lab2);

        cmsFloat64Number dL;
        cmsFloat64Number da;
        cmsFloat64Number db;
        cmsFloat64Number dAlpha;

        dL = fabs((qreal)(labF1.L - labF2.L));
        da = fabs((qreal)(labF1.a - labF2.a));
        db = fabs((qreal)(labF1.b - labF2.b));

        static const int LabAAlphaPos = 3;
        static const cmsFloat64Number alphaScale = 100.0 / KoColorSpaceMathsTraits<quint16>::max;
        quint16 alpha1 = reinterpret_cast<quint16 *>(lab1)[LabAAlphaPos];
        quint16 alpha2 = reinterpret_cast<quint16 *>(lab2)[LabAAlphaPos];
        dAlpha = fabs((qreal)(alpha1 - alpha2)) * alphaScale;

        qreal diff = pow(dL * dL + da * da + db * db + dAlpha * dAlpha, 0.5);

        if (diff > 255.0) {
            return 255;
        } else {
            return quint8(diff);
        }
    }

private:

    inline LcmsColorProfileContainer *lcmsProfile() const
    {
        return d->profile;
    }

    inline static LcmsColorProfileContainer *asLcmsProfile(const KoColorProfile *p)
    {
        if (!p) {
            return 0;
        }

        const IccColorProfile *iccp = dynamic_cast<const IccColorProfile *>(p);

        if (!iccp) {
            return 0;
        }

        Q_ASSERT(iccp->asLcms());

        return iccp->asLcms();
    }

    Private *const d;
};

/**
 * Base class for all LCMS based ColorSpace factories.
 */
class LcmsColorSpaceFactory : public KoColorSpaceFactory, private KoLcmsInfo
{
public:
    LcmsColorSpaceFactory(cmsUInt32Number cmType, cmsColorSpaceSignature colorSpaceSignature)
        : KoLcmsInfo(cmType, colorSpaceSignature)
    {
    }

    virtual bool profileIsCompatible(const KoColorProfile *profile) const
    {
        const IccColorProfile *p = dynamic_cast<const IccColorProfile *>(profile);
        return (p && p->asLcms()->colorSpaceSignature() == colorSpaceSignature());
    }

    virtual QString colorSpaceEngine() const
    {
        return "icc";
    }

    virtual bool isHdr() const
    {
        return false;
    }

    virtual QList<KoColorConversionTransformationFactory *> colorConversionLinks() const;
    virtual KoColorProfile *createColorProfile(const QByteArray &rawData) const;
};

#endif
