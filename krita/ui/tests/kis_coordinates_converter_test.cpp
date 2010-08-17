/*
 *  Copyright (c) 2010 Dmitry Kazakov <dimula73@gmail.com>
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

#include "kis_coordinates_converter_test.h"
#include <QTest>

#include <qtest_kde.h>

#include <QTransform>

#include <KoZoomHandler.h>
#include <KoColorSpaceRegistry.h>
#include <kis_image.h>

#include "kis_coordinates_converter.h"


void initImage(KisImageSP *image, KoZoomHandler **zoomHandler)
{
    const KoColorSpace *cs = KoColorSpaceRegistry::instance()->rgb8();
    *image = new KisImage(0, 1000, 1000, cs, "projection test");
    (*image)->setResolution(100, 100);

    *zoomHandler = new KoZoomHandler();
    (*zoomHandler)->setResolution(100, 100);
}

void KisCoordinatesConverterTest::testConversion()
{
    KisImageSP image;
    KoZoomHandler *zoomHandler;
    initImage(&image, &zoomHandler);

    QRectF testRect(100,100,100,100);
    KisCoordinatesConverter converter(zoomHandler);

    converter.setImage(image);
    converter.setDocumentOrigin(QPoint(10,10));
    converter.setDocumentOffset(QPoint(30,30));

    zoomHandler->setZoom(1.);

    QCOMPARE(converter.imageToViewport(testRect), QRectF(70,70,100,100));
    QCOMPARE(converter.viewportToImage(testRect), QRectF(130,130,100,100));

    QCOMPARE(converter.widgetToViewport(testRect), QRectF(90,90,100,100));
    QCOMPARE(converter.viewportToWidget(testRect), QRectF(110,110,100,100));

    QCOMPARE(converter.widgetToDocument(testRect), QRectF(1.20,1.20,1,1));
    QCOMPARE(converter.documentToWidget(testRect), QRectF(9980,9980,10000,10000));

    zoomHandler->setZoom(0.5);

    QCOMPARE(converter.imageToViewport(testRect), QRectF(20,20,50,50));
    QCOMPARE(converter.viewportToImage(testRect), QRectF(260,260,200,200));

    QCOMPARE(converter.widgetToViewport(testRect), QRectF(90,90,100,100));
    QCOMPARE(converter.viewportToWidget(testRect), QRectF(110,110,100,100));

    QCOMPARE(converter.widgetToDocument(testRect), QRectF(2.4,2.4,2,2));
    QCOMPARE(converter.documentToWidget(testRect), QRectF(4980,4980,5000,5000));

    delete zoomHandler;
}

void KisCoordinatesConverterTest::testImageCropping()
{
    KisImageSP image;
    KoZoomHandler *zoomHandler;
    initImage(&image, &zoomHandler);

    KisCoordinatesConverter converter(zoomHandler);

    converter.setImage(image);
    converter.setDocumentOrigin(QPoint(0,0));
    converter.setDocumentOffset(QPoint(0,0));

    zoomHandler->setZoom(1.);

    // we do NOT crop here
    QCOMPARE(converter.viewportToImage(QRectF(900,900,200,200)),
             QRectF(900,900,200,200));
}

void KisCoordinatesConverterTest::testTransformations()
{
    KisImageSP image;
    KoZoomHandler *zoomHandler;
    initImage(&image, &zoomHandler);

    KisCoordinatesConverter converter(zoomHandler);

    converter.setImage(image);
    converter.setDocumentOrigin(QPoint(10,20));
    converter.setDocumentOffset(QPoint(30,50));

    zoomHandler->setZoom(1.);

    QRectF testRect(100,100,100,100);
    QTransform transform = converter.imageToWidgetTransform();

    QRectF rect1 = converter.viewportToWidget(converter.imageToViewport(testRect));
    QRectF rect2 = transform.map(testRect).boundingRect();

    QCOMPARE(rect1, rect2);

    zoomHandler->setZoom(0.5);
    transform = converter.imageToWidgetTransform();

    rect1 = converter.viewportToWidget(converter.imageToViewport(testRect));
    rect2 = transform.map(testRect).boundingRect();

    QCOMPARE(rect1, rect2);
}


QTEST_KDEMAIN(KisCoordinatesConverterTest, GUI)
#include "kis_coordinates_converter_test.moc"
