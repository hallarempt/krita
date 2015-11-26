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

#include "kis_brush_export.h"

#include <QCheckBox>
#include <QSlider>

#include <KoDialog.h>
#include <kpluginfactory.h>
#include <QUrl>

#include <KisFilterChain.h>

#include <kis_paint_device.h>
#include <KisDocument.h>
#include <kis_image.h>
#include <kis_paint_layer.h>
#include <kis_gbr_brush.h>
#include <kis_imagepipe_brush.h>
#include <KisAnimatedBrushAnnotation.h>
#include <KisImportExportManager.h>

#include <ui_wdg_export_gbr.h>

K_PLUGIN_FACTORY_WITH_JSON(KisBrushExportFactory, "krita_brush_export.json", registerPlugin<KisBrushExport>();)

KisBrushExport::KisBrushExport(QObject *parent, const QVariantList &) : KisImportExportFilter(parent)
{
}

KisBrushExport::~KisBrushExport()
{
}

KisImportExportFilter::ConversionStatus KisBrushExport::convert(const QByteArray& from, const QByteArray& to)
{
    qDebug() << "Brush export! From:" << from << ", To:" << to << "";

    KisDocument *input = m_chain->inputDocument();
    QString filename = m_chain->outputFile();

    if (!input)
        return KisImportExportFilter::NoDocumentCreated;

    if (filename.isEmpty()) return KisImportExportFilter::FileNotFound;

    if (from != "application/x-krita")
        return KisImportExportFilter::NotImplemented;


    KoDialog* dlgBrushExportOptions = new KoDialog(0);
    dlgBrushExportOptions->setWindowTitle(i18n("Brush Tip Export Options"));
    dlgBrushExportOptions->setButtons(KoDialog::Ok | KoDialog::Cancel);


    KisBrush *brush;

    if (to == "image/x-gimp-brush") {
        brush = new KisGbrBrush(filename);

        Ui::WdgExportGbr wdgUi;
        QWidget* wdg = new QWidget(dlgBrushExportOptions);
        wdgUi.setupUi(wdg);
        dlgBrushExportOptions->setMainWidget(wdg);

    }
    else if (to == "image/x-gimp-brush-animated") {
        brush = new KisImagePipeBrush(filename);
    }
    else {
        delete dlgBrushExportOptions;
        return KisImportExportFilter::BadMimeType;
    }

    if (!m_chain->manager()->getBatchMode()) {
        if (dlgBrushExportOptions->exec() == QDialog::Rejected) {
            delete dlgBrushExportOptions;
            return KisImportExportFilter::UserCancelled;
        }
    }
    else {
        qApp->processEvents(); // For vector layers to be updated
    }
    input->image()->waitForDone();
    QRect rc = input->image()->bounds();
    input->image()->refreshGraph();
    input->image()->lock();
    QImage image = input->image()->projection()->convertToQImage(0, 0, 0, rc.width(), rc.height(), KoColorConversionTransformation::internalRenderingIntent(), KoColorConversionTransformation::internalConversionFlags());
    input->image()->unlock();



    QFile f(filename);
    f.open(QIODevice::WriteOnly);
    QDataStream s(&f);
    s.setByteOrder(QDataStream::LittleEndian);


    delete dlgBrushExportOptions;

    return KisImportExportFilter::OK;
}

#include "kis_brush_export.moc"

