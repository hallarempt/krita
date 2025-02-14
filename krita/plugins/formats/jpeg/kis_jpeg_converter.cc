/*
 *  Copyright (c) 2005 Cyrille Berger <cberger@cberger.net>
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */
#include "kis_jpeg_converter.h"

#include <stdio.h>
#include <stdint.h>

#include <KoConfig.h>
#ifdef HAVE_LCMS2
#   include <lcms2.h>
#else
#   include <lcms.h>
#endif

extern "C" {
#include <iccjpeg.h>
}

#include <exiv2/jpgimage.hpp>

#include <QFile>
#include <QBuffer>
#include <QApplication>

#include <QMessageBox>

#include <klocalizedstring.h>
#include <QUrl>

#include <KoDocumentInfo.h>
#include <KoColorSpace.h>
#include <KoColorSpaceRegistry.h>
#include <KoColorProfile.h>
#include <KoColor.h>
#include <KoUnit.h>
#include "KoColorModelStandardIds.h"

#include <kis_painter.h>
#include <KisDocument.h>
#include <kis_image.h>
#include <kis_paint_layer.h>
#include <kis_transaction.h>
#include <kis_group_layer.h>
#include <kis_meta_data_entry.h>
#include <kis_meta_data_value.h>
#include <kis_meta_data_store.h>
#include <kis_meta_data_io_backend.h>
#include <kis_paint_device.h>
#include <kis_transform_worker.h>
#include <kis_jpeg_source.h>
#include <kis_jpeg_destination.h>
#include "kis_iterator_ng.h"

#include <KoColorModelStandardIds.h>

#define ICC_MARKER  (JPEG_APP0 + 2) /* JPEG marker code for ICC */
#define ICC_OVERHEAD_LEN  14    /* size of non-profile data in APP2 */
#define MAX_BYTES_IN_MARKER  65533  /* maximum data len of a JPEG marker */
#define MAX_DATA_BYTES_IN_MARKER  (MAX_BYTES_IN_MARKER - ICC_OVERHEAD_LEN)

const char photoshopMarker[] = "Photoshop 3.0\0";
//const char photoshopBimId_[] = "8BIM";
const uint16_t photoshopIptc = 0x0404;
const char xmpMarker[] = "http://ns.adobe.com/xap/1.0/\0";
const QByteArray photoshopIptc_((char*)&photoshopIptc, 2);

namespace
{

J_COLOR_SPACE getColorTypeforColorSpace(const KoColorSpace * cs)
{
    if (KoID(cs->id()) == KoID("GRAYA") || cs->id() == "GRAYAU16" || cs->id() == "GRAYA16") {
        return JCS_GRAYSCALE;
    }
    if (KoID(cs->id()) == KoID("RGBA") || KoID(cs->id()) == KoID("RGBA16")) {
        return JCS_RGB;
    }
    if (KoID(cs->id()) == KoID("CMYK") || KoID(cs->id()) == KoID("CMYKAU16")) {
        return JCS_CMYK;
    }
    return JCS_UNKNOWN;
}

QString getColorSpaceModelForColorType(J_COLOR_SPACE color_type)
{
    dbgFile << "color_type =" << color_type;
    if (color_type == JCS_GRAYSCALE) {
        return GrayAColorModelID.id();
    } else if (color_type == JCS_RGB) {
        return RGBAColorModelID.id();
    } else if (color_type == JCS_CMYK) {
        return CMYKAColorModelID.id();
    }
    return "";
}

}

KisJPEGConverter::KisJPEGConverter(KisDocument *doc, bool batchMode)
    : m_doc(doc)
    , m_stop(false)
    , m_batchMode(batchMode)
{
}

KisJPEGConverter::~KisJPEGConverter()
{
}

KisImageBuilder_Result KisJPEGConverter::decode(const QUrl &uri)
{
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&cinfo);

    Q_ASSERT(uri.isLocalFile());

    // open the file
    QFile file(uri.toLocalFile());
    if (!file.exists()) {
        return (KisImageBuilder_RESULT_NOT_EXIST);
    }
    if (!file.open(QIODevice::ReadOnly)) {
        return (KisImageBuilder_RESULT_BAD_FETCH);
    }

    KisJPEGSource::setSource(&cinfo, &file);

    jpeg_save_markers(&cinfo, JPEG_COM, 0xFFFF);
    /* Save APP0..APP15 markers */
    for (int m = 0; m < 16; m++)
        jpeg_save_markers(&cinfo, JPEG_APP0 + m, 0xFFFF);


//     setup_read_icc_profile(&cinfo);
    // read header
    jpeg_read_header(&cinfo, (boolean)true);

    // start reading
    jpeg_start_decompress(&cinfo);

    // Get the colorspace
    QString modelId = getColorSpaceModelForColorType(cinfo.out_color_space);
    if (modelId.isEmpty()) {
        dbgFile << "unsupported colorspace :" << cinfo.out_color_space;
        jpeg_destroy_decompress(&cinfo);
        return KisImageBuilder_RESULT_UNSUPPORTED_COLORSPACE;
    }
    uchar* profile_data;
    uint profile_len;
    const KoColorProfile* profile = 0;
    QByteArray profile_rawdata;
    if (read_icc_profile(&cinfo, &profile_data, &profile_len)) {
        profile_rawdata.resize(profile_len);
        memcpy(profile_rawdata.data(), profile_data, profile_len);
        cmsHPROFILE hProfile = cmsOpenProfileFromMem(profile_data, profile_len);

        if (hProfile != (cmsHPROFILE) NULL) {
            profile = KoColorSpaceRegistry::instance()->createColorProfile(modelId, Integer8BitsColorDepthID.id(), profile_rawdata);
            Q_CHECK_PTR(profile);
            dbgFile <<"profile name:" << profile->name() <<" product information:" << profile->info();
            if (!profile->isSuitableForOutput()) {
                dbgFile << "the profile is not suitable for output and therefore cannot be used in krita, we need to convert the image to a standard profile"; // TODO: in ko2 popup a selection menu to inform the user
            }
        }
    }

    // Check that the profile is used by the color space
    if (profile && !KoColorSpaceRegistry::instance()->colorSpaceFactory(
        KoColorSpaceRegistry::instance()->colorSpaceId(
      modelId, Integer8BitsColorDepthID.id()))->profileIsCompatible(profile)) {
        warnFile << "The profile " << profile->name() << " is not compatible with the color space model " << modelId;
        profile = 0;
    }

    // Retrieve a pointer to the colorspace
    const KoColorSpace* cs;
    if (profile && profile->isSuitableForOutput()) {
        dbgFile << "image has embedded profile:" << profile -> name() << "";
        cs = KoColorSpaceRegistry::instance()->colorSpace(modelId, Integer8BitsColorDepthID.id(), profile);
    } else
        cs = KoColorSpaceRegistry::instance()->colorSpace(modelId, Integer8BitsColorDepthID.id(), "");

    if (cs == 0) {
        dbgFile << "unknown colorspace";
        jpeg_destroy_decompress(&cinfo);
        return KisImageBuilder_RESULT_UNSUPPORTED_COLORSPACE;
    }
    // TODO fixit
    // Create the cmsTransform if needed

    KoColorTransformation* transform = 0;
    if (profile && !profile->isSuitableForOutput()) {
        transform = KoColorSpaceRegistry::instance()->colorSpace(modelId, Integer8BitsColorDepthID.id(), profile)->createColorConverter(cs, KoColorConversionTransformation::internalRenderingIntent(), KoColorConversionTransformation::internalConversionFlags());
    }
    // Apparently an invalid transform was created from the profile. See bug https://bugs.kde.org/show_bug.cgi?id=255451.
    // After 2.3: warn the user!
    if (transform && !transform->isValid()) {
        delete transform;
        transform = 0;
    }

    // Creating the KisImageWSP
    if (! m_image) {
        m_image = new KisImage(m_doc->createUndoStore(),  cinfo.image_width,  cinfo.image_height, cs, "built image");
        Q_CHECK_PTR(m_image);
    }

    // Set resolution
    double xres = 72, yres = 72;
    if (cinfo.density_unit == 1) {
        xres = cinfo.X_density;
        yres = cinfo.Y_density;
    } else if (cinfo.density_unit == 2) {
        xres = cinfo.X_density * 2.54;
        yres = cinfo.Y_density * 2.54;
    }
    if (xres < 72) {
        xres = 72;
    }
    if (yres < 72) {
        yres = 72;
    }
    m_image->setResolution(POINT_TO_INCH(xres), POINT_TO_INCH(yres));   // It is the "invert" macro because we convert from pointer-per-inchs to points

    // Create layer
    KisPaintLayerSP layer = KisPaintLayerSP(new KisPaintLayer(m_image.data(), m_image -> nextLayerName(), quint8_MAX));

    // Read data
    JSAMPROW row_pointer = new JSAMPLE[cinfo.image_width*cinfo.num_components];

    for (; cinfo.output_scanline < cinfo.image_height;) {
        KisHLineIteratorSP it = layer->paintDevice()->createHLineIteratorNG(0, cinfo.output_scanline, cinfo.image_width);
        jpeg_read_scanlines(&cinfo, &row_pointer, 1);
        quint8 *src = row_pointer;
        switch (cinfo.out_color_space) {
        case JCS_GRAYSCALE:
            do {
                quint8 *d = it->rawData();
                d[0] = *(src++);
                if (transform) transform->transform(d, d, 1);
                d[1] = quint8_MAX;

            } while (it->nextPixel());
            break;
        case JCS_RGB:
            do {
                quint8 *d = it->rawData();
                d[2] = *(src++);
                d[1] = *(src++);
                d[0] = *(src++);
                if (transform) transform->transform(d, d, 1);
                d[3] = quint8_MAX;

            } while (it->nextPixel());
            break;
        case JCS_CMYK:
            do {
                quint8 *d = it->rawData();
                d[0] = quint8_MAX - *(src++);
                d[1] = quint8_MAX - *(src++);
                d[2] = quint8_MAX - *(src++);
                d[3] = quint8_MAX - *(src++);
                if (transform) transform->transform(d, d, 1);
                d[4] = quint8_MAX;

            } while (it->nextPixel());
            break;
        default:
            return KisImageBuilder_RESULT_UNSUPPORTED;
        }
    }

    m_image->addNode(KisNodeSP(layer.data()), m_image->rootLayer().data());

    // Read exif information

    dbgFile << "Looking for exif information";

    for (jpeg_saved_marker_ptr marker = cinfo.marker_list; marker != NULL; marker = marker->next) {
        dbgFile << "Marker is" << marker->marker;
        if (marker->marker != (JOCTET)(JPEG_APP0 + 1)
                || marker->data_length < 14) {
            continue; /* Exif data is in an APP1 marker of at least 14 octets */
        }

        if (GETJOCTET(marker->data[0]) != (JOCTET) 0x45 ||
                GETJOCTET(marker->data[1]) != (JOCTET) 0x78 ||
                GETJOCTET(marker->data[2]) != (JOCTET) 0x69 ||
                GETJOCTET(marker->data[3]) != (JOCTET) 0x66 ||
                GETJOCTET(marker->data[4]) != (JOCTET) 0x00 ||
                GETJOCTET(marker->data[5]) != (JOCTET) 0x00)
            continue; /* no Exif header */
        dbgFile << "Found exif information of length :" << marker->data_length;
        KisMetaData::IOBackend* exifIO = KisMetaData::IOBackendRegistry::instance()->value("exif");
        Q_ASSERT(exifIO);
        QByteArray byteArray((const char*)marker->data + 6, marker->data_length - 6);
        QBuffer buf(&byteArray);
        exifIO->loadFrom(layer->metaData(), &buf);
        // Interpret orientation tag
        if (layer->metaData()->containsEntry("http://ns.adobe.com/tiff/1.0/", "Orientation")) {
            KisMetaData::Entry& entry = layer->metaData()->getEntry("http://ns.adobe.com/tiff/1.0/", "Orientation");
            if (entry.value().type() == KisMetaData::Value::Variant) {
                switch (entry.value().asVariant().toInt()) {
                case 2:
                    KisTransformWorker::mirrorY(layer->paintDevice());
                    break;
                case 3:
                    image()->rotateImage(M_PI);
                    break;
                case 4:
                    KisTransformWorker::mirrorX(layer->paintDevice());
                    break;
                case 5:
                    image()->rotateImage(M_PI / 2);
                    KisTransformWorker::mirrorY(layer->paintDevice());
                    break;
                case 6:
                    image()->rotateImage(M_PI / 2);
                    break;
                case 7:
                    image()->rotateImage(M_PI / 2);
                    KisTransformWorker::mirrorX(layer->paintDevice());
                    break;
                case 8:
                    image()->rotateImage(-M_PI / 2 + M_PI*2);
                    break;
                default:
                    break;
                }
            }
            entry.value().setVariant(1);
        }
        break;
    }

    dbgFile << "Looking for IPTC information";

    for (jpeg_saved_marker_ptr marker = cinfo.marker_list; marker != NULL; marker = marker->next) {
        dbgFile << "Marker is" << marker->marker;
        if (marker->marker != (JOCTET)(JPEG_APP0 + 13) ||  marker->data_length < 14) {
            continue; /* IPTC data is in an APP13 marker of at least 16 octets */
        }
        if (memcmp(marker->data, photoshopMarker, 14) != 0) {
            for (int i = 0; i < 14; i++) {
                dbgFile << (int)(*(marker->data + i)) << "" << (int)(photoshopMarker[i]);
            }
            dbgFile << "No photoshop marker";
            continue; /* No IPTC Header */
        }

        dbgFile << "Found Photoshop information of length :" << marker->data_length;
        KisMetaData::IOBackend* iptcIO = KisMetaData::IOBackendRegistry::instance()->value("iptc");
        Q_ASSERT(iptcIO);
        const Exiv2::byte *record = 0;
        uint32_t sizeIptc = 0;
        uint32_t sizeHdr = 0;
        // Find actual Iptc data within the APP13 segment
        if (!Exiv2::Photoshop::locateIptcIrb((Exiv2::byte*)(marker->data + 14),
                                             marker->data_length - 14, &record, &sizeHdr, &sizeIptc)) {
            if (sizeIptc) {
                // Decode the IPTC data
                QByteArray byteArray((const char*)(record + sizeHdr), sizeIptc);
                iptcIO->loadFrom(layer->metaData(), new QBuffer(&byteArray));
            } else {
                dbgFile << "IPTC Not found in Photoshop marker";
            }
        }
        break;
    }

    dbgFile << "Looking for XMP information";

    for (jpeg_saved_marker_ptr marker = cinfo.marker_list; marker != NULL; marker = marker->next) {
        dbgFile << "Marker is" << marker->marker;
        if (marker->marker != (JOCTET)(JPEG_APP0 + 1) || marker->data_length < 31) {
            continue; /* XMP data is in an APP1 marker of at least 31 octets */
        }
        if (memcmp(marker->data, xmpMarker, 29) != 0) {
            dbgFile << "Not XMP marker";
            continue; /* No xmp Header */
        }
        dbgFile << "Found XMP Marker of length " << marker->data_length;
        QByteArray byteArray((const char*)marker->data + 29, marker->data_length - 29);
        KisMetaData::IOBackend* xmpIO = KisMetaData::IOBackendRegistry::instance()->value("xmp");
        Q_ASSERT(xmpIO);
        xmpIO->loadFrom(layer->metaData(), new QBuffer(&byteArray));
        break;
    }

    // Dump loaded metadata
    layer->metaData()->debugDump();

    // Check whether the metadata has resolution info, too...
    if (cinfo.density_unit == 0 && layer->metaData()->containsEntry("tiff:XResolution") && layer->metaData()->containsEntry("tiff:YResolution")) {
        double xres = layer->metaData()->getEntry("tiff:XResolution").value().asDouble();
        double yres = layer->metaData()->getEntry("tiff:YResolution").value().asDouble();
        if (xres != 0 && yres != 0) {
            m_image->setResolution(POINT_TO_INCH(xres), POINT_TO_INCH(yres));   // It is the "invert" macro because we convert from pointer-per-inchs to points
        }
    }

    // Finish decompression
    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    delete [] row_pointer;
    return KisImageBuilder_RESULT_OK;
}



KisImageBuilder_Result KisJPEGConverter::buildImage(const QUrl &uri)
{
    if (uri.isEmpty())
        return KisImageBuilder_RESULT_NO_URI;


    if (!uri.isLocalFile()) {
        return KisImageBuilder_RESULT_NOT_EXIST;
    }
    return decode(uri);

}


KisImageWSP KisJPEGConverter::image()
{
    return m_image;
}


KisImageBuilder_Result KisJPEGConverter::buildFile(const QUrl &uri, KisPaintLayerSP layer, vKisAnnotationSP_it /*annotationsStart*/, vKisAnnotationSP_it /*annotationsEnd*/, KisJPEGOptions options, KisMetaData::Store* metaData)
{
    if (!layer)
        return KisImageBuilder_RESULT_INVALID_ARG;

    KisImageWSP image = KisImageWSP(layer->image());
    if (!image)
        return KisImageBuilder_RESULT_EMPTY;

    if (uri.isEmpty())
        return KisImageBuilder_RESULT_NO_URI;

    if (!uri.isLocalFile())
        return KisImageBuilder_RESULT_NOT_LOCAL;

    const KoColorSpace * cs = layer->colorSpace();
    J_COLOR_SPACE color_type = getColorTypeforColorSpace(cs);

    if (!m_batchMode && cs->colorDepthId() != Integer8BitsColorDepthID) {
        QMessageBox::information(0, i18nc("@title:window", "Krita"), i18n("Warning: JPEG only supports 8 bits per channel. Your image uses: %1. Krita will save your image as 8 bits per channel.", cs->name()));
    }

    if (color_type == JCS_UNKNOWN) {
        if (!m_batchMode) {
            QMessageBox::information(0, i18nc("@title:window", "Krita"), i18n("Cannot export images in %1.\nWill save as RGB.", cs->name()));
        }
        KUndo2Command *tmp = layer->paintDevice()->convertTo(KoColorSpaceRegistry::instance()->rgb8(), KoColorConversionTransformation::internalRenderingIntent(), KoColorConversionTransformation::internalConversionFlags());
        delete tmp;
        cs = KoColorSpaceRegistry::instance()->rgb8();
        color_type = JCS_RGB;
    }

    if (options.forceSRGB) {
        const KoColorSpace* dst = KoColorSpaceRegistry::instance()->colorSpace(RGBAColorModelID.id(), layer->colorSpace()->colorDepthId().id(), "sRGB built-in - (lcms internal)");
        KUndo2Command *tmp = layer->paintDevice()->convertTo(dst);
        delete tmp;
        cs = dst;
        color_type = JCS_RGB;
    }


    // Open file for writing
    QFile file(uri.toLocalFile());
    if (!file.open(QIODevice::WriteOnly)) {
        return (KisImageBuilder_RESULT_FAILURE);
    }

    uint height = image->height();
    uint width = image->width();
    // Initialize structure
    struct jpeg_compress_struct cinfo;
    // Initialize error output
    struct jpeg_error_mgr jerr;
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);
    // Initialize output stream
    KisJPEGDestination::setDestination(&cinfo, &file);

    cinfo.image_width = width;  // image width and height, in pixels
    cinfo.image_height = height;
    cinfo.input_components = cs->colorChannelCount(); // number of color channels per pixel */
    cinfo.in_color_space = color_type;   // colorspace of input image

    // Set default compression parameters
    jpeg_set_defaults(&cinfo);
    // Customize them
    jpeg_set_quality(&cinfo, options.quality, (boolean)options.baseLineJPEG);

    if (options.progressive) {
        jpeg_simple_progression(&cinfo);
    }
    // Optimize ?
    cinfo.optimize_coding = (boolean)options.optimize;

    // Smoothing
    cinfo.smoothing_factor = (boolean)options.smooth;

    // Subsampling
    switch (options.subsampling) {
    default:
    case 0: {
        cinfo.comp_info[0].h_samp_factor = 2;
        cinfo.comp_info[0].v_samp_factor = 2;
        cinfo.comp_info[1].h_samp_factor = 1;
        cinfo.comp_info[1].v_samp_factor = 1;
        cinfo.comp_info[2].h_samp_factor = 1;
        cinfo.comp_info[2].v_samp_factor = 1;

    }
    break;
    case 1: {
        cinfo.comp_info[0].h_samp_factor = 2;
        cinfo.comp_info[0].v_samp_factor = 1;
        cinfo.comp_info[1].h_samp_factor = 1;
        cinfo.comp_info[1].v_samp_factor = 1;
        cinfo.comp_info[2].h_samp_factor = 1;
        cinfo.comp_info[2].v_samp_factor = 1;
    }
    break;
    case 2: {
        cinfo.comp_info[0].h_samp_factor = 1;
        cinfo.comp_info[0].v_samp_factor = 2;
        cinfo.comp_info[1].h_samp_factor = 1;
        cinfo.comp_info[1].v_samp_factor = 1;
        cinfo.comp_info[2].h_samp_factor = 1;
        cinfo.comp_info[2].v_samp_factor = 1;
    }
    break;
    case 3: {
        cinfo.comp_info[0].h_samp_factor = 1;
        cinfo.comp_info[0].v_samp_factor = 1;
        cinfo.comp_info[1].h_samp_factor = 1;
        cinfo.comp_info[1].v_samp_factor = 1;
        cinfo.comp_info[2].h_samp_factor = 1;
        cinfo.comp_info[2].v_samp_factor = 1;
    }
    break;
    }

    // Save resolution
    cinfo.X_density = INCH_TO_POINT(image->xRes()); // It is the "invert" macro because we convert from pointer-per-inchs to points
    cinfo.Y_density = INCH_TO_POINT(image->yRes()); // It is the "invert" macro because we convert from pointer-per-inchs to points
    cinfo.density_unit = 1;
    cinfo.write_JFIF_header = (boolean)true;

    // Start compression
    jpeg_start_compress(&cinfo, (boolean)true);
    // Save exif and iptc information if any available

    if (metaData && !metaData->empty()) {
        metaData->applyFilters(options.filters);
        // Save EXIF
        if (options.exif) {
            dbgFile << "Trying to save exif information";

            KisMetaData::IOBackend* exifIO = KisMetaData::IOBackendRegistry::instance()->value("exif");
            Q_ASSERT(exifIO);

            QBuffer buffer;
            exifIO->saveTo(metaData, &buffer, KisMetaData::IOBackend::JpegHeader);

            dbgFile << "Exif information size is" << buffer.data().size();
            QByteArray data = buffer.data();
            if (data.size() < MAX_DATA_BYTES_IN_MARKER) {
                jpeg_write_marker(&cinfo, JPEG_APP0 + 1, (const JOCTET*)data.data(), data.size());
            } else {
                dbgFile << "EXIF information could not be saved."; // TODO: warn the user ?
            }
        }
        // Save IPTC
        if (options.iptc) {
            dbgFile << "Trying to save exif information";
            KisMetaData::IOBackend* iptcIO = KisMetaData::IOBackendRegistry::instance()->value("iptc");
            Q_ASSERT(iptcIO);

            QBuffer buffer;
            iptcIO->saveTo(metaData, &buffer, KisMetaData::IOBackend::JpegHeader);

            dbgFile << "IPTC information size is" << buffer.data().size();
            QByteArray data = buffer.data();
            if (data.size() < MAX_DATA_BYTES_IN_MARKER) {
                jpeg_write_marker(&cinfo, JPEG_APP0 + 13, (const JOCTET*)data.data(), data.size());
            } else {
                dbgFile << "IPTC information could not be saved."; // TODO: warn the user ?
            }
        }
        // Save XMP
        if (options.xmp) {
            dbgFile << "Trying to save XMP information";
            KisMetaData::IOBackend* xmpIO = KisMetaData::IOBackendRegistry::instance()->value("xmp");
            Q_ASSERT(xmpIO);

            QBuffer buffer;
            xmpIO->saveTo(metaData, &buffer, KisMetaData::IOBackend::JpegHeader);

            dbgFile << "XMP information size is" << buffer.data().size();
            QByteArray data = buffer.data();
            if (data.size() < MAX_DATA_BYTES_IN_MARKER) {
                jpeg_write_marker(&cinfo, JPEG_APP0 + 14, (const JOCTET*)data.data(), data.size());
            } else {
                dbgFile << "XMP information could not be saved."; // TODO: warn the user ?
            }
        }
    }


    KisPaintDeviceSP dev = new KisPaintDevice(layer->colorSpace());
    KoColor c(options.transparencyFillColor, layer->colorSpace());
    dev->fill(QRect(0, 0, width, height), c);
    KisPainter gc(dev);
    gc.bitBlt(QPoint(0, 0), layer->paintDevice(), QRect(0, 0, width, height));
    gc.end();


    if (options.saveProfile) {
        const KoColorProfile* colorProfile = layer->colorSpace()->profile();
        QByteArray colorProfileData = colorProfile->rawData();
        write_icc_profile(& cinfo, (uchar*) colorProfileData.data(), colorProfileData.size());
    }

    // Write data information

    JSAMPROW row_pointer = new JSAMPLE[width*cinfo.input_components];
    int color_nb_bits = 8 * layer->paintDevice()->pixelSize() / layer->paintDevice()->channelCount();

    for (; cinfo.next_scanline < height;) {
        KisHLineConstIteratorSP it = dev->createHLineConstIteratorNG(0, cinfo.next_scanline, width);
        quint8 *dst = row_pointer;
        switch (color_type) {
        case JCS_GRAYSCALE:
            if (color_nb_bits == 16) {
                do {
                    //const quint16 *d = reinterpret_cast<const quint16 *>(it->oldRawData());
                    const quint8 *d = it->oldRawData();
                    *(dst++) = cs->scaleToU8(d, 0);//d[0] / quint8_MAX;

                } while (it->nextPixel());
            } else {
                do {
                    const quint8 *d = it->oldRawData();
                    *(dst++) = d[0];

                } while (it->nextPixel());
            }
            break;
        case JCS_RGB:
            if (color_nb_bits == 16) {
                do {
                    //const quint16 *d = reinterpret_cast<const quint16 *>(it->oldRawData());
                    const quint8 *d = it->oldRawData();
                    *(dst++) = cs->scaleToU8(d, 2); //d[2] / quint8_MAX;
                    *(dst++) = cs->scaleToU8(d, 1); //d[1] / quint8_MAX;
                    *(dst++) = cs->scaleToU8(d, 0); //d[0] / quint8_MAX;

                } while (it->nextPixel());
            } else {
                do {
                    const quint8 *d = it->oldRawData();
                    *(dst++) = d[2];
                    *(dst++) = d[1];
                    *(dst++) = d[0];

                } while (it->nextPixel());
            }
            break;
        case JCS_CMYK:
            if (color_nb_bits == 16) {
                do {
                    //const quint16 *d = reinterpret_cast<const quint16 *>(it->oldRawData());
                    const quint8 *d = it->oldRawData();
                    *(dst++) = quint8_MAX - cs->scaleToU8(d, 0);//quint8_MAX - d[0] / quint8_MAX;
                    *(dst++) = quint8_MAX - cs->scaleToU8(d, 1);//quint8_MAX - d[1] / quint8_MAX;
                    *(dst++) = quint8_MAX - cs->scaleToU8(d, 2);//quint8_MAX - d[2] / quint8_MAX;
                    *(dst++) = quint8_MAX - cs->scaleToU8(d, 3);//quint8_MAX - d[3] / quint8_MAX;

                } while (it->nextPixel());
            } else {
                do {
                    const quint8 *d = it->oldRawData();
                    *(dst++) = quint8_MAX - d[0];
                    *(dst++) = quint8_MAX - d[1];
                    *(dst++) = quint8_MAX - d[2];
                    *(dst++) = quint8_MAX - d[3];

                } while (it->nextPixel());
            }
            break;
        default:
            delete [] row_pointer;
            jpeg_destroy_compress(&cinfo);
            return KisImageBuilder_RESULT_UNSUPPORTED;
        }
        jpeg_write_scanlines(&cinfo, &row_pointer, 1);
    }


    // Writing is over
    jpeg_finish_compress(&cinfo);
    file.close();

    delete [] row_pointer;
    // Free memory
    jpeg_destroy_compress(&cinfo);

    return KisImageBuilder_RESULT_OK;
}


void KisJPEGConverter::cancel()
{
    m_stop = true;
}


