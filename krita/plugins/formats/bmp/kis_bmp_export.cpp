/*
 *  Copyright (c) 2007 Boudewijn Rempt <boud@valdyas.org>
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

#include "kis_bmp_export.h"

#include <QCheckBox>
#include <QSlider>

#include <kpluginfactory.h>
#include <QUrl>

#include <KisFilterChain.h>

#include <kis_paint_device.h>
#include <KisDocument.h>
#include <kis_image.h>
#include <kis_paint_layer.h>

K_PLUGIN_FACTORY_WITH_JSON(KisBMPExportFactory, "krita_bmp_export.json", registerPlugin<KisBMPExport>();)

KisBMPExport::KisBMPExport(QObject *parent, const QVariantList &) : KisImportExportFilter(parent)
{
}

KisBMPExport::~KisBMPExport()
{
}

KisImportExportFilter::ConversionStatus KisBMPExport::convert(const QByteArray& from, const QByteArray& to)
{
    dbgFile << "BMP export! From:" << from << ", To:" << to << "";

    KisDocument *input = m_chain->inputDocument();
    QString filename = m_chain->outputFile();

    if (!input)
        return KisImportExportFilter::NoDocumentCreated;

    if (filename.isEmpty()) return KisImportExportFilter::FileNotFound;

    if (from != "application/x-krita")
        return KisImportExportFilter::NotImplemented;

    QUrl url = QUrl::fromLocalFile(filename);

    qApp->processEvents(); // For vector layers to be updated
    input->image()->waitForDone();

    QRect rc = input->image()->bounds();
    input->image()->refreshGraph();
    input->image()->lock();
    QImage image = input->image()->projection()->convertToQImage(0, 0, 0, rc.width(), rc.height(), KoColorConversionTransformation::internalRenderingIntent(), KoColorConversionTransformation::internalConversionFlags());
    input->image()->unlock();
    image.save(url.toLocalFile());
    return KisImportExportFilter::OK;
}

#include "kis_bmp_export.moc"

