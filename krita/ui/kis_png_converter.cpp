/*
 *  Copyright (c) 2005-2007 Cyrille Berger <cberger@cberger.net>
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

#include "kis_png_converter.h"
// A big thank to Glenn Randers-Pehrson for his wonderful
// documentation of libpng available at
// http://www.libpng.org/pub/png/libpng-1.2.5-manual.html

#ifndef PNG_MAX_UINT // Removed in libpng 1.4
#define PNG_MAX_UINT PNG_UINT_31_MAX
#endif

#include <KoConfig.h> // WORDS_BIGENDIAN
#include <KoStore.h>
#include <KoStoreDevice.h>

#include <limits.h>
#include <stdio.h>
#include <zlib.h>

#include <QBuffer>
#include <QFile>
#include <QApplication>

#include <klocalizedstring.h>
#include <QUrl>

#include <KoColorSpace.h>
#include <KoDocumentInfo.h>
#include <KoID.h>
#include <KoColorSpaceRegistry.h>
#include <KoColorProfile.h>
#include <KoColorSpace.h>
#include <KoColor.h>
#include <KoUnit.h>

#include <kis_config.h>
#include <kis_painter.h>
#include <KisDocument.h>
#include <kis_image.h>
#include <kis_iterator_ng.h>
#include <kis_layer.h>
#include <kis_paint_device.h>
#include <kis_transaction.h>
#include <kis_paint_layer.h>
#include <kis_group_layer.h>
#include <kis_meta_data_io_backend.h>
#include <kis_meta_data_store.h>
#include <KoColorModelStandardIds.h>
#include "dialogs/kis_dlg_png_import.h"
#include "kis_clipboard.h"

namespace
{


int getColorTypeforColorSpace(const KoColorSpace * cs , bool alpha)
{

    QString id = cs->id();

    if (id == "GRAYA" || id == "GRAYAU16" || id == "GRAYA16") {
        return alpha ? PNG_COLOR_TYPE_GRAY_ALPHA : PNG_COLOR_TYPE_GRAY;
    }
    if (id == "RGBA" || id == "RGBA16") {
        return alpha ? PNG_COLOR_TYPE_RGB_ALPHA : PNG_COLOR_TYPE_RGB;
    }

    return -1;

}

bool colorSpaceIdSupported(const QString &id)
{
    return id == "RGBA" || id == "RGBA16" ||
        id == "GRAYA" || id == "GRAYAU16" || id == "GRAYA16";
}

QPair<QString, QString> getColorSpaceForColorType(int color_type, int color_nb_bits)
{
    QPair<QString, QString> r;

    if (color_type ==  PNG_COLOR_TYPE_PALETTE) {
        r.first = RGBAColorModelID.id();
        r.second = Integer8BitsColorDepthID.id();
    } else {
        if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {
            r.first = GrayAColorModelID.id();
        } else if (color_type == PNG_COLOR_TYPE_RGB_ALPHA || color_type == PNG_COLOR_TYPE_RGB) {
            r.first = RGBAColorModelID.id();
        }
        if (color_nb_bits == 16) {
            r.second = Integer16BitsColorDepthID.id();
        } else if (color_nb_bits <= 8) {
            r.second = Integer8BitsColorDepthID.id();
        }
    }
    return r;
}


void fillText(png_text* p_text, const char* key, QString& text)
{
    p_text->compression = PNG_TEXT_COMPRESSION_zTXt;
    p_text->key = const_cast<char *>(key);
    char* textc = new char[text.length()+1];
    strcpy(textc, text.toLatin1());
    p_text->text = textc;
    p_text->text_length = text.length() + 1;
}

long formatStringList(char *string, const size_t length, const char *format, va_list operands)
{
    int n = vsnprintf(string, length, format, operands);

    if (n < 0)
        string[length-1] = '\0';

    return((long) n);
}

long formatString(char *string, const size_t length, const char *format, ...)
{
    long n;

    va_list operands;

    va_start(operands, format);
    n = (long) formatStringList(string, length, format, operands);
    va_end(operands);
    return(n);
}

void writeRawProfile(png_struct *ping, png_info *ping_info, QString profile_type, QByteArray profile_data)
{

    png_textp      text;

    png_uint_32    allocated_length, description_length;

    const uchar hex[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

    dbgFile << "Writing Raw profile: type=" << profile_type << ", length=" << profile_data.length() << endl;

    text               = (png_textp) png_malloc(ping, (png_uint_32) sizeof(png_text));
    description_length = profile_type.length();
    allocated_length   = (png_uint_32)(profile_data.length() * 2 + (profile_data.length() >> 5) + 20 + description_length);

    text[0].text   = (png_charp) png_malloc(ping, allocated_length);

    QString key = QLatin1Literal("Raw profile type ") + profile_type.toLatin1();
    QByteArray keyData = key.toLatin1();
    text[0].key = keyData.data();

    uchar* sp = (uchar*)profile_data.data();
    png_charp dp = text[0].text;
    *dp++ = '\n';

    memcpy(dp, profile_type.toLatin1().constData(), profile_type.length());

    dp += description_length;
    *dp++ = '\n';

    formatString(dp, allocated_length - strlen(text[0].text), "%8lu ", profile_data.length());

    dp += 8;

    for (long i = 0; i < (long) profile_data.length(); i++) {
        if (i % 36 == 0)
            *dp++ = '\n';

        *(dp++) = (char) hex[((*sp >> 4) & 0x0f)];
        *(dp++) = (char) hex[((*sp++) & 0x0f)];
    }

    *dp++ = '\n';
    *dp = '\0';
    text[0].text_length = (png_size_t)(dp - text[0].text);
    text[0].compression = -1;

    if (text[0].text_length <= allocated_length)
        png_set_text(ping, ping_info, text, 1);

    png_free(ping, text[0].text);
    png_free(ping, text);
}

QByteArray png_read_raw_profile(png_textp text)
{
    QByteArray profile;

    static const unsigned char unhex[103] = {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 10, 11, 12,
        13, 14, 15
    };

    png_charp sp = text[0].text + 1;
    /* look for newline */
    while (*sp != '\n')
        sp++;
    /* look for length */
    while (*sp == '\0' || *sp == ' ' || *sp == '\n')
        sp++;
    png_uint_32 length = (png_uint_32) atol(sp);
    while (*sp != ' ' && *sp != '\n')
        sp++;
    if (length == 0) {
        return profile;
    }
    profile.resize(length);
    /* copy profile, skipping white space and column 1 "=" signs */
    unsigned char *dp = (unsigned char*)profile.data();
    png_uint_32 nibbles = length * 2;
    for (png_uint_32 i = 0; i < nibbles; i++) {
        while (*sp < '0' || (*sp > '9' && *sp < 'a') || *sp > 'f') {
            if (*sp == '\0') {
                return QByteArray();
            }
            sp++;
        }
        if (i % 2 == 0)
            *dp = (unsigned char)(16 * unhex[(int) *sp++]);
        else
            (*dp++) += unhex[(int) *sp++];
    }
    return profile;
}

void decode_meta_data(png_textp text, KisMetaData::Store* store, QString type, int headerSize)
{
    dbgFile << "Decoding " << type << " " << text[0].key;
    KisMetaData::IOBackend* exifIO = KisMetaData::IOBackendRegistry::instance()->value(type);
    Q_ASSERT(exifIO);

    QByteArray rawProfile = png_read_raw_profile(text);
    if (headerSize > 0) {
        rawProfile.remove(0, headerSize);
    }
    if (rawProfile.size() > 0) {
        QBuffer buffer;
        buffer.setData(rawProfile);
        exifIO->loadFrom(store, &buffer);
    } else {
        dbgFile << "Decoding failed";
    }
}
}

KisPNGConverter::KisPNGConverter(KisDocument *doc, bool batchMode)
{
    //     Q_ASSERT(doc);
    //     Q_ASSERT(adapter);

    m_doc = doc;
    m_stop = false;
    m_max_row = 0;
    m_image = 0;
    m_batchMode = batchMode;
}

KisPNGConverter::~KisPNGConverter()
{
}

class KisPNGReadStream
{
public:
    KisPNGReadStream(quint8* buf,  quint32 depth) : m_posinc(8), m_depth(depth), m_buf(buf) {
    }
    int nextValue() {
        if (m_posinc == 0) {
            m_posinc = 8;
            m_buf++;
        }
        m_posinc -= m_depth;
        return (((*m_buf) >> (m_posinc)) & ((1 << m_depth) - 1));
    }
private:
    quint32 m_posinc, m_depth;
    quint8* m_buf;
};

class KisPNGWriteStream
{
public:
    KisPNGWriteStream(quint8* buf,  quint32 depth) : m_posinc(8), m_depth(depth), m_buf(buf) {
        *m_buf = 0;
    }
    void setNextValue(int v) {
        if (m_posinc == 0) {
            m_posinc = 8;
            m_buf++;
            *m_buf = 0;
        }
        m_posinc -= m_depth;
        *m_buf = (v << m_posinc) | *m_buf;
    }
private:
    quint32 m_posinc, m_depth;
    quint8* m_buf;
};

class KisPNGReaderAbstract
{
public:
    KisPNGReaderAbstract(png_structp _png_ptr, int _width, int _height) : png_ptr(_png_ptr), width(_width), height(_height) {}
    virtual ~KisPNGReaderAbstract() {}
    virtual png_bytep readLine() = 0;
protected:
    png_structp png_ptr;
    int width, height;
};

class KisPNGReaderLineByLine : public KisPNGReaderAbstract
{
public:
    KisPNGReaderLineByLine(png_structp _png_ptr, png_infop info_ptr, int _width, int _height) : KisPNGReaderAbstract(_png_ptr, _width, _height) {
        png_uint_32 rowbytes = png_get_rowbytes(png_ptr, info_ptr);
        row_pointer = new png_byte[rowbytes];
    }
    virtual ~KisPNGReaderLineByLine() {
        delete[] row_pointer;
    }
    virtual png_bytep readLine() {
        png_read_row(png_ptr, row_pointer, NULL);
        return row_pointer;
    }
private:
    png_bytep row_pointer;
};

class KisPNGReaderFullImage : public KisPNGReaderAbstract
{
public:
    KisPNGReaderFullImage(png_structp _png_ptr, png_infop info_ptr, int _width, int _height) : KisPNGReaderAbstract(_png_ptr, _width, _height), y(0) {
        row_pointers = new png_bytep[height];
        png_uint_32 rowbytes = png_get_rowbytes(png_ptr, info_ptr);
        for (int i = 0; i < height; i++) {
            row_pointers[i] = new png_byte[rowbytes];
        }
        png_read_image(png_ptr, row_pointers);
    }
    virtual ~KisPNGReaderFullImage() {
        for (int i = 0; i < height; i++) {
            delete[] row_pointers[i];
        }
        delete[] row_pointers;
    }
    virtual png_bytep readLine() {
        return row_pointers[y++];
    }
private:
    png_bytepp row_pointers;
    int y;
};


static
void _read_fn(png_structp png_ptr, png_bytep data, png_size_t length)
{
    QIODevice *in = (QIODevice *)png_get_io_ptr(png_ptr);

    while (length) {
        int nr = in->read((char*)data, length);
        if (nr <= 0) {
            png_error(png_ptr, "Read Error");
            return;
        }
        length -= nr;
    }
}

static
void _write_fn(png_structp png_ptr, png_bytep data, png_size_t length)
{
    QIODevice* out = (QIODevice*)png_get_io_ptr(png_ptr);

    uint nr = out->write((char*)data, length);
    if (nr != length) {
        png_error(png_ptr, "Write Error");
        return;
    }
}

static
void _flush_fn(png_structp png_ptr)
{
    Q_UNUSED(png_ptr);
}


KisImageBuilder_Result KisPNGConverter::buildImage(QIODevice* iod)
{
    dbgFile << "Start decoding PNG File";
    if (!iod->open(QIODevice::ReadOnly)) {
        dbgFile << "Failed to open PNG File";
        return (KisImageBuilder_RESULT_FAILURE);
    }

    png_byte signature[8];
    iod->peek((char*)signature, 8);

#if PNG_LIBPNG_VER < 10400
    if (!png_check_sig(signature, 8)) {
#else
    if (png_sig_cmp(signature, 0, 8) != 0) {
#endif
        iod->close();
        return (KisImageBuilder_RESULT_BAD_FETCH);
    }

    // Initialize the internal structures
    png_structp png_ptr =  png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
    if (!png_ptr) {
        iod->close();
    }

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
        iod->close();
        return (KisImageBuilder_RESULT_FAILURE);
    }

    png_infop end_info = png_create_info_struct(png_ptr);
    if (!end_info) {
        png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
        iod->close();
        return (KisImageBuilder_RESULT_FAILURE);
    }

    // Catch errors
    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
        iod->close();
        return (KisImageBuilder_RESULT_FAILURE);
    }

    // Initialize the special
    png_set_read_fn(png_ptr, iod, _read_fn);

#if defined(PNG_SKIP_sRGB_CHECK_PROFILE) && defined(PNG_SET_OPTION_SUPPORTED)
    png_set_option(png_ptr, PNG_SKIP_sRGB_CHECK_PROFILE, PNG_OPTION_ON);
#endif

    // read all PNG info up to image data
    png_read_info(png_ptr, info_ptr);


    if (png_get_color_type(png_ptr, info_ptr) == PNG_COLOR_TYPE_GRAY && png_get_bit_depth(png_ptr, info_ptr) < 8) {
        png_set_expand(png_ptr);
    }

    if (png_get_color_type(png_ptr, info_ptr) == PNG_COLOR_TYPE_PALETTE && png_get_bit_depth(png_ptr, info_ptr) < 8) {
        png_set_packing(png_ptr);
    }


    if (png_get_color_type(png_ptr, info_ptr) != PNG_COLOR_TYPE_PALETTE &&
            (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))) {
        png_set_expand(png_ptr);
    }
    png_read_update_info(png_ptr, info_ptr);

    // Read information about the png
    png_uint_32 width, height;
    int color_nb_bits, color_type, interlace_type;
    png_get_IHDR(png_ptr, info_ptr, &width, &height, &color_nb_bits, &color_type, &interlace_type, NULL, NULL);
    dbgFile << "width = " << width << " height = " << height << " color_nb_bits = " << color_nb_bits << " color_type = " << color_type << " interlace_type = " << interlace_type << endl;
    // swap byteorder on little endian machines.
#ifndef WORDS_BIGENDIAN
    if (color_nb_bits > 8)
        png_set_swap(png_ptr);
#endif

    // Determine the colorspace
    QPair<QString, QString> csName = getColorSpaceForColorType(color_type, color_nb_bits);
    if (csName.first.isEmpty()) {
        png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
        iod->close();
        return KisImageBuilder_RESULT_UNSUPPORTED_COLORSPACE;
    }
    bool hasalpha = (color_type == PNG_COLOR_TYPE_RGB_ALPHA || color_type == PNG_COLOR_TYPE_GRAY_ALPHA);

    // Read image profile
    png_charp profile_name;
#if PNG_LIBPNG_VER_MAJOR >= 1 && PNG_LIBPNG_VER_MINOR >= 5
    png_bytep profile_data;
#else
    png_charp profile_data;
#endif
    int compression_type;
    png_uint_32 proflen;


    const KoColorProfile* profile = 0;
    if (png_get_iCCP(png_ptr, info_ptr, &profile_name, &compression_type, &profile_data, &proflen)) {
        QByteArray profile_rawdata;
        // XXX: Hardcoded for icc type -- is that correct for us?
        profile_rawdata.resize(proflen);
        memcpy(profile_rawdata.data(), profile_data, proflen);
        profile = KoColorSpaceRegistry::instance()->createColorProfile(csName.first, csName.second, profile_rawdata);
        Q_CHECK_PTR(profile);
        if (profile) {
            //                 dbgFile << "profile name: " << profile->productName() << " profile description: " << profile->productDescription() << " information sur le produit: " << profile->productInfo();
            if (!profile->isSuitableForOutput()) {
                dbgFile << "the profile is not suitable for output and therefore cannot be used in krita, we need to convert the image to a standard profile"; // TODO: in ko2 popup a selection menu to inform the user
            }
        }
    }
    else {
        dbgFile << "no embedded profile, will use the default profile";
        if (color_nb_bits == 16 && !qAppName().toLower().contains("test") && !m_batchMode) {
            KisConfig cfg;
            quint32 behaviour = cfg.pasteBehaviour();
            if (behaviour == PASTE_ASK) {
                KisDlgPngImport dlg(m_path, csName.first, csName.second);
                QApplication::restoreOverrideCursor();
                dlg.exec();
                if (!dlg.profile().isEmpty()) {

                    QString s = KoColorSpaceRegistry::instance()->colorSpaceId(csName.first, csName.second);

                    const KoColorSpaceFactory * csf = KoColorSpaceRegistry::instance()->colorSpaceFactory(s);
                    if (csf) {
                        QList<const KoColorProfile *>  profileList = KoColorSpaceRegistry::instance()->profilesFor(csf);
                        Q_FOREACH (const KoColorProfile *p, profileList) {
                            if (p->name() == dlg.profile()) {
                                profile = p;
                                break;
                            }
                        }
                    }
                }
                QApplication::setOverrideCursor(Qt::WaitCursor);

            }
        }
        dbgFile << "no embedded profile, will use the default profile";
    }

    // Check that the profile is used by the color space
    if (profile && !KoColorSpaceRegistry::instance()->colorSpaceFactory(
                KoColorSpaceRegistry::instance()->colorSpaceId(
                    csName.first, csName.second))->profileIsCompatible(profile)) {
        warnFile << "The profile " << profile->name() << " is not compatible with the color space model " << csName.first << " " << csName.second;
        profile = 0;
    }

    // Retrieve a pointer to the colorspace
    const KoColorSpace* cs;
    if (profile && profile->isSuitableForOutput()) {
        dbgFile << "image has embedded profile: " << profile->name() << "\n";
        cs = KoColorSpaceRegistry::instance()->colorSpace(csName.first, csName.second, profile);
    }
    else {
        if (csName.first == RGBAColorModelID.id()) {
            cs = KoColorSpaceRegistry::instance()->colorSpace(csName.first, csName.second, "sRGB-elle-V2-srgbtrc.icc");
        }
        else {
            cs = KoColorSpaceRegistry::instance()->colorSpace(csName.first, csName.second, 0);
        }
    }

    if (cs == 0) {
        png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
        return KisImageBuilder_RESULT_UNSUPPORTED_COLORSPACE;
    }
    //TODO: two fixes : one tell the user about the problem and ask for a solution, and two once the kocolorspace include KoColorTransformation, use that instead of hacking a lcms transformation
    // Create the cmsTransform if needed
    KoColorTransformation* transform = 0;
    if (profile && !profile->isSuitableForOutput()) {
        transform = KoColorSpaceRegistry::instance()->colorSpace(csName.first, csName.second, profile)->createColorConverter(cs, KoColorConversionTransformation::internalRenderingIntent(), KoColorConversionTransformation::internalConversionFlags());
    }

    // Creating the KisImageWSP
    if (m_image == 0) {
        m_image = new KisImage(m_doc->createUndoStore(), width, height, cs, "built image");
        Q_CHECK_PTR(m_image);
    }

    // Read resolution
    int unit_type;
    png_uint_32 x_resolution, y_resolution;

    png_get_pHYs(png_ptr, info_ptr, &x_resolution, &y_resolution, &unit_type);
    if (unit_type == PNG_RESOLUTION_METER) {
        m_image->setResolution((double) POINT_TO_CM(x_resolution) / 100.0, (double) POINT_TO_CM(y_resolution) / 100.0); // It is the "invert" macro because we convert from pointer-per-inchs to points
    }

    double coeff = quint8_MAX / (double)(pow((double)2, color_nb_bits) - 1);
    KisPaintLayerSP layer = new KisPaintLayer(m_image.data(), m_image -> nextLayerName(), UCHAR_MAX);

    // Read comments/texts...
    png_text* text_ptr;
    int num_comments;
    png_get_text(png_ptr, info_ptr, &text_ptr, &num_comments);
    if (m_doc) {
        KoDocumentInfo * info = m_doc->documentInfo();
        dbgFile << "There are " << num_comments << " comments in the text";
        for (int i = 0; i < num_comments; i++) {
            QString key = text_ptr[i].key;
            dbgFile << "key is |" << text_ptr[i].key << "| containing " << text_ptr[i].text << " " << (key ==  "Raw profile type exif ");
            if (key == "title") {
                info->setAboutInfo("title", text_ptr[i].text);
            } else if (key == "abstract") {
                info->setAboutInfo("description", text_ptr[i].text);
            } else if (key == "author") {
                info->setAuthorInfo("creator", text_ptr[i].text);
            } else if (key.contains("Raw profile type exif")) {
                decode_meta_data(text_ptr + i, layer->metaData(), "exif", 6);
            } else if (key.contains("Raw profile type iptc")) {
                decode_meta_data(text_ptr + i, layer->metaData(), "iptc", 14);
            } else if (key.contains("Raw profile type xmp")) {
                decode_meta_data(text_ptr + i, layer->metaData(), "xmp", 0);
            } else if (key == "version") {
                m_image->addAnnotation(new KisAnnotation("kpp_version", "version", QByteArray(text_ptr[i].text)));
            } else if (key == "preset") {
                m_image->addAnnotation(new KisAnnotation("kpp_preset", "preset", QByteArray(text_ptr[i].text)));
            }
        }
    }

    // Read image data
    KisPNGReaderAbstract* reader = 0;
    try {
        if (interlace_type == PNG_INTERLACE_ADAM7) {
            reader = new KisPNGReaderFullImage(png_ptr, info_ptr, width, height);
        } else {
            reader = new KisPNGReaderLineByLine(png_ptr, info_ptr, width, height);
        }
    } catch (std::bad_alloc& e) {
        // new png_byte[] may raise such an exception if the image
        // is invalid / to large.
        dbgFile << "bad alloc: " << e.what();
        // Free only the already allocated png_byte instances.
        png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
        return (KisImageBuilder_RESULT_FAILURE);
    }

    // Read the palette if the file is indexed
    png_colorp palette ;
    int num_palette;
    if (color_type == PNG_COLOR_TYPE_PALETTE) {
        png_get_PLTE(png_ptr, info_ptr, &palette, &num_palette);
    }

    // Read the transparency palette
    quint8 palette_alpha[256];
    memset(palette_alpha, 255, 256);
    if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) {
        if (color_type == PNG_COLOR_TYPE_PALETTE) {
            png_bytep alpha_ptr;
            int num_alpha;
            png_get_tRNS(png_ptr, info_ptr, &alpha_ptr, &num_alpha, NULL);
            for (int i = 0; i < num_alpha; ++i) {
                palette_alpha[i] = alpha_ptr[i];
            }
        }
    }

    for (png_uint_32 y = 0; y < height; y++) {
        KisHLineIteratorSP it = layer -> paintDevice() -> createHLineIteratorNG(0, y, width);

        png_bytep row_pointer = reader->readLine();

        switch (color_type) {
        case PNG_COLOR_TYPE_GRAY:
        case PNG_COLOR_TYPE_GRAY_ALPHA:
            if (color_nb_bits == 16) {
                quint16 *src = reinterpret_cast<quint16 *>(row_pointer);
                do {
                    quint16 *d = reinterpret_cast<quint16 *>(it->rawData());
                    d[0] = *(src++);
                    if (transform) transform->transform(reinterpret_cast<quint8*>(d), reinterpret_cast<quint8*>(d), 1);
                    if (hasalpha) {
                        d[1] = *(src++);
                    } else {
                        d[1] = quint16_MAX;
                    }
                } while (it->nextPixel());
            } else  {
                KisPNGReadStream stream(row_pointer, color_nb_bits);
                do {
                    quint8 *d = it->rawData();
                    d[0] = (quint8)(stream.nextValue() * coeff);
                    if (transform) transform->transform(d, d, 1);
                    if (hasalpha) {
                        d[1] = (quint8)(stream.nextValue() * coeff);
                    } else {
                        d[1] = UCHAR_MAX;
                    }
                } while (it->nextPixel());
            }
            // FIXME:should be able to read 1 and 4 bits depth and scale them to 8 bits"
            break;
        case PNG_COLOR_TYPE_RGB:
        case PNG_COLOR_TYPE_RGB_ALPHA:
            if (color_nb_bits == 16) {
                quint16 *src = reinterpret_cast<quint16 *>(row_pointer);
                do {
                    quint16 *d = reinterpret_cast<quint16 *>(it->rawData());
                    d[2] = *(src++);
                    d[1] = *(src++);
                    d[0] = *(src++);
                    if (transform) transform->transform(reinterpret_cast<quint8 *>(d), reinterpret_cast<quint8*>(d), 1);
                    if (hasalpha) d[3] = *(src++);
                    else d[3] = quint16_MAX;
                } while (it->nextPixel());
            } else {
                KisPNGReadStream stream(row_pointer, color_nb_bits);
                do {
                    quint8 *d = it->rawData();
                    d[2] = (quint8)(stream.nextValue() * coeff);
                    d[1] = (quint8)(stream.nextValue() * coeff);
                    d[0] = (quint8)(stream.nextValue() * coeff);
                    if (transform) transform->transform(d, d, 1);
                    if (hasalpha) d[3] = (quint8)(stream.nextValue() * coeff);
                    else d[3] = UCHAR_MAX;
                } while (it->nextPixel());
            }
            break;
        case PNG_COLOR_TYPE_PALETTE: {
            KisPNGReadStream stream(row_pointer, color_nb_bits);
            do {
                quint8 *d = it->rawData();
                quint8 index = stream.nextValue();
                quint8 alpha = palette_alpha[ index ];
                if (alpha == 0) {
                    memset(d, 0, 4);
                } else {
                    png_color c = palette[ index ];
                    d[2] = c.red;
                    d[1] = c.green;
                    d[0] = c.blue;
                    d[3] = alpha;
                }
            } while (it->nextPixel());
        }
            break;
        default:
            return KisImageBuilder_RESULT_UNSUPPORTED;
        }
    }
    m_image->addNode(layer.data(), m_image->rootLayer().data());

    png_read_end(png_ptr, end_info);
    iod->close();

    // Freeing memory
    png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);

    delete reader;
    return KisImageBuilder_RESULT_OK;

}

KisImageBuilder_Result KisPNGConverter::buildImage(const QUrl &uri)
{
    dbgFile << QFile::encodeName(uri.path()) << " " << uri.path() << " " << uri;
    if (uri.isEmpty())
        return KisImageBuilder_RESULT_NO_URI;

    if (!uri.isLocalFile()) {
        return KisImageBuilder_RESULT_NOT_LOCAL;
    }

    m_path = uri.toDisplayString();

    QFile fp(uri.toLocalFile());
    if (fp.exists()) {
        return buildImage(&fp);
    }
    return (KisImageBuilder_RESULT_NOT_EXIST);

}


KisImageWSP KisPNGConverter::image()
{
    return m_image;
}

bool KisPNGConverter::saveDeviceToStore(const QString &filename, const QRect &imageRect, const qreal xRes, const qreal yRes, KisPaintDeviceSP dev, KoStore *store, KisMetaData::Store* metaData)
{
    if (store->open(filename)) {
        KoStoreDevice io(store);
        if (!io.open(QIODevice::WriteOnly)) {
            dbgFile << "Could not open for writing:" << filename;
            return false;
        }
        KisPNGConverter pngconv(0);
        vKisAnnotationSP_it annotIt = 0;
        KisMetaData::Store* metaDataStore = 0;
        if (metaData) {
            metaDataStore = new KisMetaData::Store(*metaData);
        }
        KisPNGOptions options;
        options.compression = 0;
        options.interlace = false;
        options.tryToSaveAsIndexed = false;
        options.alpha = true;
        bool success = pngconv.buildFile(&io, imageRect, xRes, yRes, dev, annotIt, annotIt, options, metaDataStore);
        if (success != KisImageBuilder_RESULT_OK) {
            dbgFile << "Saving PNG failed:" << filename;
            delete metaDataStore;
            return false;
        }
        delete metaDataStore;
        io.close();
        if (!store->close()) {
            return false;
        }
    } else {
        dbgFile << "Opening of data file failed :" << filename;
        return false;
    }
    return true;

}


KisImageBuilder_Result KisPNGConverter::buildFile(const QUrl &uri, const QRect &imageRect, const qreal xRes, const qreal yRes, KisPaintDeviceSP device, vKisAnnotationSP_it annotationsStart, vKisAnnotationSP_it annotationsEnd, KisPNGOptions options, KisMetaData::Store* metaData)
{
    dbgFile << "Start writing PNG File " << uri;
    if (uri.isEmpty())
        return KisImageBuilder_RESULT_NO_URI;

    if (!uri.isLocalFile())
        return KisImageBuilder_RESULT_NOT_LOCAL;
    // Open a QIODevice for writing
    QFile *fp = new QFile(uri.toLocalFile());
    KisImageBuilder_Result result = buildFile(fp, imageRect, xRes, yRes, device, annotationsStart, annotationsEnd, options, metaData);
    delete fp;
    return result;
}

KisImageBuilder_Result KisPNGConverter::buildFile(QIODevice* iodevice, const QRect &imageRect, const qreal xRes, const qreal yRes, KisPaintDeviceSP device, vKisAnnotationSP_it annotationsStart, vKisAnnotationSP_it annotationsEnd, KisPNGOptions options, KisMetaData::Store* metaData)
{
    if (!iodevice->open(QIODevice::WriteOnly)) {
        dbgFile << "Failed to open PNG File for writing";
        return (KisImageBuilder_RESULT_FAILURE);
    }

    if (!device)
        return KisImageBuilder_RESULT_INVALID_ARG;

    if (!options.alpha) {
        KisPaintDeviceSP tmp = new KisPaintDevice(device->colorSpace());
        KoColor c(options.transparencyFillColor, device->colorSpace());
        tmp->fill(imageRect, c);
        KisPainter gc(tmp);
        gc.bitBlt(imageRect.topLeft(), device, imageRect);
        gc.end();
        device = tmp;
    }

    if (options.forceSRGB) {
        const KoColorSpace* cs = KoColorSpaceRegistry::instance()->colorSpace(RGBAColorModelID.id(), device->colorSpace()->colorDepthId().id(), "sRGB built-in - (lcms internal)");
        device = new KisPaintDevice(*device);
        device->convertTo(cs);
    }

    // Initialize structures
    png_structp png_ptr =  png_create_write_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
    if (!png_ptr) {
        return (KisImageBuilder_RESULT_FAILURE);
    }

#if defined(PNG_SKIP_sRGB_CHECK_PROFILE) && defined(PNG_SET_OPTION_SUPPORTED)
    png_set_option(png_ptr, PNG_SKIP_sRGB_CHECK_PROFILE, PNG_OPTION_ON);
#endif


#ifdef PNG_READ_CHECK_FOR_INVALID_INDEX_SUPPORTED
    png_set_check_for_invalid_index(png_ptr, 0);
#endif

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
        return (KisImageBuilder_RESULT_FAILURE);
    }

    // If an error occurs during writing, libpng will jump here
    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_write_struct(&png_ptr, &info_ptr);
        return (KisImageBuilder_RESULT_FAILURE);
    }
    // Initialize the writing
    //     png_init_io(png_ptr, fp);
    // Setup the progress function
    // XXX: Implement progress updating -- png_set_write_status_fn(png_ptr, progress);"
    //     setProgressTotalSteps(100/*height*/);

    /* set the zlib compression level */
    png_set_compression_level(png_ptr, options.compression);

    png_set_write_fn(png_ptr, (void*)iodevice, _write_fn, _flush_fn);

    /* set other zlib parameters */
    png_set_compression_mem_level(png_ptr, 8);
    png_set_compression_strategy(png_ptr, Z_DEFAULT_STRATEGY);
    png_set_compression_window_bits(png_ptr, 15);
    png_set_compression_method(png_ptr, 8);
    png_set_compression_buffer_size(png_ptr, 8192);

    int color_nb_bits = 8 * device->pixelSize() / device->channelCount();
    int color_type = getColorTypeforColorSpace(device->colorSpace(), options.alpha);

    Q_ASSERT(color_type > -1);

    // Try to compute a table of color if the colorspace is RGB8f
    png_colorp palette = 0;
    int num_palette = 0;
    if (!options.alpha && options.tryToSaveAsIndexed && KoID(device->colorSpace()->id()) == KoID("RGBA")) { // png doesn't handle indexed images and alpha, and only have indexed for RGB8
        palette = new png_color[255];

        KisSequentialIterator it(device, imageRect);

        bool toomuchcolor = false;
        do {
            const quint8* c = it.oldRawData();
            bool findit = false;
            for (int i = 0; i < num_palette; i++) {
                if (palette[i].red == c[2] &&
                        palette[i].green == c[1] &&
                        palette[i].blue == c[0]) {
                    findit = true;
                    break;
                }
            }
            if (!findit) {
                if (num_palette == 255) {
                    toomuchcolor = true;
                    break;
                }
                palette[num_palette].red = c[2];
                palette[num_palette].green = c[1];
                palette[num_palette].blue = c[0];
                num_palette++;
            }
        }  while (it.nextPixel());

        if (!toomuchcolor) {
            dbgFile << "Found a palette of " << num_palette << " colors";
            color_type = PNG_COLOR_TYPE_PALETTE;
            if (num_palette <= 2) {
                color_nb_bits = 1;
            } else if (num_palette <= 4) {
                color_nb_bits = 2;
            } else if (num_palette <= 16) {
                color_nb_bits = 4;
            } else {
                color_nb_bits = 8;
            }
        } else {
            delete [] palette;
        }
    }

    int interlacetype = options.interlace ? PNG_INTERLACE_ADAM7 : PNG_INTERLACE_NONE;

    png_set_IHDR(png_ptr, info_ptr,
                 imageRect.width(),
                 imageRect.height(),
                 color_nb_bits,
                 color_type, interlacetype,
                 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

    // set sRGB only if the profile is sRGB  -- http://www.w3.org/TR/PNG/#11sRGB says sRGB and iCCP should not both be present

    bool sRGB = device->colorSpace()->profile()->name().contains(QLatin1String("srgb"), Qt::CaseInsensitive);
    if (!options.saveSRGBProfile && sRGB) {
        png_set_sRGB(png_ptr, info_ptr, PNG_sRGB_INTENT_ABSOLUTE);
    }
    // set the palette
    if (color_type == PNG_COLOR_TYPE_PALETTE) {
        png_set_PLTE(png_ptr, info_ptr, palette, num_palette);
    }
    // Save annotation
    vKisAnnotationSP_it it = annotationsStart;
    while (it != annotationsEnd) {
        if (!(*it) || (*it)->type().isEmpty()) {
            dbgFile << "Warning: empty annotation";
            it++;
            continue;
        }

        dbgFile << "Trying to store annotation of type " << (*it) -> type() << " of size " << (*it) -> annotation() . size();

        if ((*it) -> type().startsWith(QString("krita_attribute:"))) { //
            // Attribute
            // XXX: it should be possible to save krita_attributes in the \"CHUNKs\""
            dbgFile << "cannot save this annotation : " << (*it) -> type();
        } else if ((*it)->type() == "kpp_version" || (*it)->type() == "kpp_preset" ) {
            dbgFile << "Saving preset information " << (*it)->description();
            png_textp      text = (png_textp) png_malloc(png_ptr, (png_uint_32) sizeof(png_text));

            QByteArray keyData = (*it)->description().toLatin1();
            text[0].key = keyData.data();
            text[0].text = (char*)(*it)->annotation().data();
            text[0].text_length = (*it)->annotation().size();
            text[0].compression = -1;

            png_set_text(png_ptr, info_ptr, text, 1);
            png_free(png_ptr, text);
        }
        it++;
    }

    // Save the color profile
    const KoColorProfile* colorProfile = device->colorSpace()->profile();
    QByteArray colorProfileData = colorProfile->rawData();
    if (!sRGB || options.saveSRGBProfile) {
#if PNG_LIBPNG_VER_MAJOR >= 1 && PNG_LIBPNG_VER_MINOR >= 5
        png_set_iCCP(png_ptr, info_ptr, (char*)"icc", PNG_COMPRESSION_TYPE_BASE, (const png_bytep)colorProfileData.constData(), colorProfileData . size());
#else
        png_set_iCCP(png_ptr, info_ptr, (char*)"icc", PNG_COMPRESSION_TYPE_BASE, (char*)colorProfileData.constData(), colorProfileData . size());
#endif
    }

    // read comments from the document information
    if (m_doc) {
        png_text texts[3];
        int nbtexts = 0;
        KoDocumentInfo * info = m_doc->documentInfo();
        QString title = info->aboutInfo("creator");
        if (!title.isEmpty()) {
            fillText(texts + nbtexts, "title", title);
            nbtexts++;
        }
        QString abstract = info->aboutInfo("description");
        if (!abstract.isEmpty()) {
            fillText(texts + nbtexts, "abstract", abstract);
            nbtexts++;
        }
        QString author = info->authorInfo("creator");
        if (!author.isEmpty()) {
            fillText(texts + nbtexts, "author", author);
            nbtexts++;
        }

        png_set_text(png_ptr, info_ptr, texts, nbtexts);
    }

    // Save metadata following imagemagick way

    // Save exif
    if (metaData && !metaData->empty()) {
        if (options.exif) {
            dbgFile << "Trying to save exif information";

            KisMetaData::IOBackend* exifIO = KisMetaData::IOBackendRegistry::instance()->value("exif");
            Q_ASSERT(exifIO);

            QBuffer buffer;
            exifIO->saveTo(metaData, &buffer, KisMetaData::IOBackend::JpegHeader);
            writeRawProfile(png_ptr, info_ptr, "exif", buffer.data());
        }
        // Save IPTC
        if (options.iptc) {
            dbgFile << "Trying to save exif information";
            KisMetaData::IOBackend* iptcIO = KisMetaData::IOBackendRegistry::instance()->value("iptc");
            Q_ASSERT(iptcIO);

            QBuffer buffer;
            iptcIO->saveTo(metaData, &buffer, KisMetaData::IOBackend::JpegHeader);

            dbgFile << "IPTC information size is" << buffer.data().size();
            writeRawProfile(png_ptr, info_ptr, "iptc", buffer.data());
        }
        // Save XMP
        if (options.xmp) {
            dbgFile << "Trying to save XMP information";
            KisMetaData::IOBackend* xmpIO = KisMetaData::IOBackendRegistry::instance()->value("xmp");
            Q_ASSERT(xmpIO);

            QBuffer buffer;
            xmpIO->saveTo(metaData, &buffer, KisMetaData::IOBackend::NoHeader);

            dbgFile << "XMP information size is" << buffer.data().size();
            writeRawProfile(png_ptr, info_ptr, "xmp", buffer.data());
        }
    }
#if 0 // Unimplemented?
    // Save resolution
    int unit_type;
    png_uint_32 x_resolution, y_resolution;
#endif
    png_set_pHYs(png_ptr, info_ptr, CM_TO_POINT(xRes) * 100.0, CM_TO_POINT(yRes) * 100.0, PNG_RESOLUTION_METER); // It is the "invert" macro because we convert from pointer-per-inchs to points

    // Save the information to the file
    png_write_info(png_ptr, info_ptr);
    png_write_flush(png_ptr);

    // swap byteorder on little endian machines.
#ifndef WORDS_BIGENDIAN
    if (color_nb_bits > 8)
        png_set_swap(png_ptr);
#endif

    // Write the PNG
    //     png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

    // Fill the data structure
    png_byte** row_pointers = new png_byte*[imageRect.height()];
    for (int y = imageRect.y(); y < imageRect.y() + imageRect.height(); y++) {

        KisHLineConstIteratorSP it = device->createHLineConstIteratorNG(imageRect.x(), y, imageRect.width());

        row_pointers[y] = new png_byte[imageRect.width() * device->pixelSize()];

        switch (color_type) {
        case PNG_COLOR_TYPE_GRAY:
        case PNG_COLOR_TYPE_GRAY_ALPHA:
            if (color_nb_bits == 16) {
                quint16 *dst = reinterpret_cast<quint16 *>(row_pointers[y]);
                do {
                    const quint16 *d = reinterpret_cast<const quint16 *>(it->oldRawData());
                    *(dst++) = d[0];
                    if (options.alpha) *(dst++) = d[1];
                } while (it->nextPixel());
            } else {
                quint8 *dst = row_pointers[y];
                do {
                    const quint8 *d = it->oldRawData();
                    *(dst++) = d[0];
                    if (options.alpha) *(dst++) = d[1];
                } while (it->nextPixel());
            }
            break;
        case PNG_COLOR_TYPE_RGB:
        case PNG_COLOR_TYPE_RGB_ALPHA:
            if (color_nb_bits == 16) {
                quint16 *dst = reinterpret_cast<quint16 *>(row_pointers[y]);
                do {
                    const quint16 *d = reinterpret_cast<const quint16 *>(it->oldRawData());
                    *(dst++) = d[2];
                    *(dst++) = d[1];
                    *(dst++) = d[0];
                    if (options.alpha) *(dst++) = d[3];
                } while (it->nextPixel());
            } else {
                quint8 *dst = row_pointers[y];
                do {
                    const quint8 *d = it->oldRawData();
                    *(dst++) = d[2];
                    *(dst++) = d[1];
                    *(dst++) = d[0];
                    if (options.alpha) *(dst++) = d[3];
                } while (it->nextPixel());
            }
            break;
        case PNG_COLOR_TYPE_PALETTE: {
            quint8 *dst = row_pointers[y];
            KisPNGWriteStream writestream(dst, color_nb_bits);
            do {
                const quint8 *d = it->oldRawData();
                int i;
                for (i = 0; i < num_palette; i++) {
                    if (palette[i].red == d[2] &&
                            palette[i].green == d[1] &&
                            palette[i].blue == d[0]) {
                        break;
                    }
                }
                writestream.setNextValue(i);
            } while (it->nextPixel());
        }
            break;
        default:
            delete[] row_pointers;
            return KisImageBuilder_RESULT_UNSUPPORTED;
        }
    }

    png_write_image(png_ptr, row_pointers);

    // Writing is over
    png_write_end(png_ptr, info_ptr);

    // Free memory
    png_destroy_write_struct(&png_ptr, &info_ptr);
    for (int y = 0; y < imageRect.height(); y++) {
        delete[] row_pointers[y];
    }
    delete[] row_pointers;

    if (color_type == PNG_COLOR_TYPE_PALETTE) {
        delete [] palette;
    }
    iodevice->close();
    return KisImageBuilder_RESULT_OK;
}


void KisPNGConverter::cancel()
{
    m_stop = true;
}

void KisPNGConverter::progress(png_structp png_ptr, png_uint_32 row_number, int pass)
{
    if (png_ptr == NULL || row_number > PNG_MAX_UINT || pass > 7) return;
    //     setProgress(row_number);
}

bool KisPNGConverter::isColorSpaceSupported(const KoColorSpace *cs)
{
    return colorSpaceIdSupported(cs->id());
}


