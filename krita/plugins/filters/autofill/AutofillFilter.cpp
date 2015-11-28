/*
 * This file is part of the KDE project
 *
 * Copyright (c) 2015 Boudewijn Rempt <boud@valdyas.org>
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

#include "AutofillFilter.h"

#include <stdlib.h>
#include <vector>
#include <math.h>

#include <QPoint>

#include <kpluginfactory.h>
#include <klocalizedstring.h>

#include <KoUpdater.h>
#include <KoMixColorsOp.h>
#include <KoColorSet.h>
#include <KoResourceServerProvider.h>
#include <KoColor.h>

#include <kis_debug.h>
#include <filter/kis_filter_registry.h>
#include <kis_global.h>
#include <kis_image.h>
#include <kis_layer.h>
#include <kis_paint_device.h>
#include <kis_random_accessor_ng.h>
#include <kis_random_generator.h>
#include <kis_selection.h>
#include <kis_types.h>
#include <filter/kis_filter_configuration.h>
#include <kis_processing_information.h>
#include <kis_iterator_ng.h>
#include <kis_fill_painter.h>

#include "WdgAutofill.h"
#include "ui_WdgAutofillOptions.h"

K_PLUGIN_FACTORY_WITH_JSON(KritaAutofillFilterFactory, "kritaautofillfilter.json", registerPlugin<KritaAutofillFilter>();)

KritaAutofillFilter::KritaAutofillFilter(QObject *parent, const QVariantList &)
    : QObject(parent)
{
    KisFilterRegistry::instance()->add(new KisFilterAutofill());
}

KritaAutofillFilter::~KritaAutofillFilter()
{
}

KisFilterAutofill::KisFilterAutofill()
    : KisFilter(id(), categoryOther(), i18n("&Automatic Fill..."))
{
    setColorSpaceIndependence(FULLY_INDEPENDENT);
    setSupportsPainting(false);
    setSupportsThreading(false);
    setSupportsAdjustmentLayers(false);
    setSupportsLevelOfDetail(false);
}


KoColorSet *colorSet(const QString &name)
{
    KoResourceServer<KoColorSet>* rServer = KoResourceServerProvider::instance()->paletteServer();
    return rServer->resourceByName(name);
}


void KisFilterAutofill::processImpl(KisPaintDeviceSP device,
                                    const QRect& applyRect,
                                    const KisFilterConfiguration* config,
                                    KoUpdater* progressUpdater
                                    ) const
{
    Q_ASSERT(!device.isNull());

    if (progressUpdater) {
        progressUpdater->setRange(0, applyRect.height() * applyRect.width());
    }

    const KoColorSpace *cs = device->colorSpace();

    QVariant value;
    KoColor fillColor(Qt::white, cs);
    int colorThreshold = 16;
    bool useRandom = false;
    KoColorSet *palette = colorSet("Default");
    int ignoreArea = 0;
    int flagArea = 18;
    KoColor flagColor(Qt::darkGray, cs);

    if (config->getProperty("autofillcolor", value)) {
        fillColor = KoColor(value.value<QColor>(), cs);
    }
    if (config->getProperty("colorthreshold", value)) {
        colorThreshold = value.toUInt();
    }
    if (config->getProperty("userandom", value)) {
        useRandom = value.toBool();
    }
    if (!useRandom && config->getProperty("palette", value)) {
        palette = colorSet(value.toString());
    }
    if (config->getProperty("ignorearea", value)) {
        ignoreArea = value.toUInt();
    }
    if (config->getProperty("flagarea", value)) {
        flagArea = value.toUInt();
    }
    if (config->getProperty("flagcolor", value)) {
        flagColor = KoColor(value.value<QColor>(), cs);
    }

    KoColor newColor(cs);
    QColor c;
    KisFillPainter gc(device);
    int count = 0;

    KisHLineIteratorSP it = device->createHLineIteratorNG(applyRect.x(), applyRect.y(), applyRect.width());

    for (int y = 0; y < applyRect.height() && !(progressUpdater && progressUpdater->interrupted()); ++y) {
        int x = applyRect.x();
        do {
            quint8 difference = cs->difference(it->rawDataConst(), fillColor.data());
            if (difference > colorThreshold) {
                if (useRandom) {
                    c.setRgbF(rand() / (double)UINT64_MAX, rand() / (double)UINT64_MAX, rand() / (double)UINT64_MAX);
                    newColor.fromQColor(c);
                }
                else {
                    KoColorSetEntry colorsetEntry = palette->getColor(rand() % palette->nColors());
                    newColor = colorsetEntry.color;
                }
                gc.setFillStyle(KisPainter::FillStyleForegroundColor);
                gc.setFillThreshold(flagArea);
                gc.setPaintColor(newColor);
                gc.fillColor(x, y, device);
                gc.end();
            }

            if (progressUpdater) progressUpdater->setValue(++count);
            x++;
        } while(it->nextPixel());

        it->nextRow();
    }

}

KisConfigWidget * KisFilterAutofill::createConfigurationWidget(QWidget* parent, const KisPaintDeviceSP dev) const
{
    Q_UNUSED(dev);
    return new KisWdgAutofill((KisFilter*)this, (QWidget*)parent);
}

KisFilterConfiguration* KisFilterAutofill::factoryConfiguration(const KisPaintDeviceSP) const
{
    KisFilterConfiguration* config = new KisFilterConfiguration("autofill", 1);

    config->setProperty("autofillcolor", QColor(Qt::white));
    config->setProperty("colorthreshold", 16);
    config->setProperty("userandom", false);
    config->setProperty("palette", "Default");
    config->setProperty("ignorearea", 0);
    config->setProperty("flagarea", 18);
    config->setProperty("flagcolor", QColor(Qt::darkGray));
    config->setProperty("createnewlayer", false);

    return config;
}

#include "AutofillFilter.moc"
