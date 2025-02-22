/* This file is part of the KDE project
 * Copyright 2010 (C) Boudewijn Rempt <boud@valdyas.org>
 * Copyright 2011 (C) Dmitry Kazakov <dimula73@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */
#include "kis_scratch_pad.h"

#include <QApplication>
#include <QDesktopWidget>
#include <QMutex>

#include <KoColorSpace.h>
#include <KoColorProfile.h>
#include <KoColorSpaceRegistry.h>
#include <KoPointerEvent.h>
#include <KoAbstractGradient.h>

#include <kis_cursor.h>
#include <kis_tool_utils.h>
#include <kis_paint_layer.h>
#include <kis_paint_device.h>
#include <kis_gradient_painter.h>
#include <kis_default_bounds.h>
#include <kis_canvas_resource_provider.h>

#include "kis_config.h"
#include "kis_image.h"
#include "kis_undo_stores.h"
#include "kis_update_scheduler.h"
#include "kis_post_execution_undo_adapter.h"
#include "kis_scratch_pad_event_filter.h"
#include "kis_painting_information_builder.h"
#include "kis_tool_freehand_helper.h"
#include "kis_image_patch.h"
#include "kis_canvas_widget_base.h"
#include "kis_layer_projection_plane.h"
#include "kis_node_graph_listener.h"


class KisScratchPadNodeListener : public KisNodeGraphListener
{
public:
    KisScratchPadNodeListener(KisScratchPad *scratchPad)
        : m_scratchPad(scratchPad)
    {
    }

    void requestProjectionUpdate(KisNode *node, const QRect& rect) {
        KisNodeGraphListener::requestProjectionUpdate(node, rect);

        QMutexLocker locker(&m_lock);
        m_scratchPad->imageUpdated(rect);
    }

private:
    KisScratchPad *m_scratchPad;
    QMutex m_lock;
};


class KisScratchPadDefaultBounds : public KisDefaultBounds
{
public:

    KisScratchPadDefaultBounds(KisScratchPad *scratchPad)
        : m_scratchPad(scratchPad)
    {
    }

    virtual ~KisScratchPadDefaultBounds() {}

    virtual QRect bounds() const {
        return m_scratchPad->imageBounds();
    }

private:
    Q_DISABLE_COPY(KisScratchPadDefaultBounds)

    KisScratchPad *m_scratchPad;
};


KisScratchPad::KisScratchPad(QWidget *parent)
    : QWidget(parent)
    , m_toolMode(HOVERING)
    , m_paintLayer(0)
    , m_displayProfile(0)
    , m_resourceProvider(0)
{
    setAutoFillBackground(false);

    m_cursor = KisCursor::load("tool_freehand_cursor.png", 5, 5);
    setCursor(m_cursor);

    KisConfig cfg;
    QImage checkImage = KisCanvasWidgetBase::createCheckersImage(cfg.checkSize());
    m_checkBrush = QBrush(checkImage);


    // We are not supposed to use updates here,
    // so just set the listener to null
    m_updateScheduler = new KisUpdateScheduler(0);
    m_undoStore = new KisSurrogateUndoStore();
    m_undoAdapter = new KisPostExecutionUndoAdapter(m_undoStore, m_updateScheduler);
    m_nodeListener = new KisScratchPadNodeListener(this);

    connect(this, SIGNAL(sigUpdateCanvas(QRect)), SLOT(slotUpdateCanvas(QRect)), Qt::QueuedConnection);

    // filter will be deleted by the QObject hierarchy
    m_eventFilter = new KisScratchPadEventFilter(this);

    m_infoBuilder = new KisPaintingInformationBuilder();
    m_helper = new KisToolFreehandHelper(m_infoBuilder);

    m_scaleBorderWidth = 1;
}

KisScratchPad::~KisScratchPad() {
    delete m_helper;
    delete m_infoBuilder;

    delete m_undoAdapter;
    delete m_undoStore;
    delete m_updateScheduler;
    delete m_nodeListener;
}

void KisScratchPad::pointerPress(KoPointerEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_toolMode = PAINTING;
        beginStroke(event);
        event->accept();
    }
    else if (event->button() == Qt::MidButton) {
        m_toolMode = PANNING;
        beginPan(event);
        event->accept();
    }
    else if (event->button() == Qt::RightButton) {
        m_toolMode = PICKING;
        event->accept();
    }
}

void KisScratchPad::pointerRelease(KoPointerEvent *event)
{
    if (m_toolMode == PAINTING) {
        endStroke(event);
        m_toolMode = HOVERING;
        event->accept();
    }
    else if (m_toolMode == PANNING) {
        endPan(event);
        m_toolMode = HOVERING;
        event->accept();
    }
    else if (m_toolMode == PICKING) {
        event->accept();
        m_toolMode = HOVERING;
    }
}

void KisScratchPad::pointerMove(KoPointerEvent *event)
{
    if (m_toolMode == PAINTING) {
        doStroke(event);
        event->accept();
    }
    else if (m_toolMode == PANNING) {
        doPan(event);
        event->accept();
    }
    else if (m_toolMode == PICKING) {
        pick(event);
        event->accept();
    }
}

void KisScratchPad::beginStroke(KoPointerEvent *event)
{
    KoCanvasResourceManager *resourceManager = m_resourceProvider->resourceManager();

    m_helper->initPaint(event,
                        resourceManager,
                        0,
                        0,
                        m_updateScheduler,
                        m_undoAdapter,
                        m_paintLayer,
                        m_paintLayer->paintDevice()->defaultBounds());
}

void KisScratchPad::doStroke(KoPointerEvent *event)
{
    m_helper->paint(event);
}

void KisScratchPad::endStroke(KoPointerEvent *event)
{
    Q_UNUSED(event);
    m_helper->endPaint();
}

void KisScratchPad::beginPan(KoPointerEvent *event)
{
    setCursor(QCursor(Qt::ClosedHandCursor));
    m_panDocPoint = event->point;
}

void KisScratchPad::doPan(KoPointerEvent *event)
{
    QPointF docOffset = event->point - m_panDocPoint;

    m_translateTransform.translate(-docOffset.x(), -docOffset.y());
    updateTransformations();
    update();
}

void KisScratchPad::endPan(KoPointerEvent *event)
{
    Q_UNUSED(event);
    setCursor(m_cursor);
}

void KisScratchPad::pick(KoPointerEvent *event)
{
    KoColor color;
    if (KisToolUtils::pick(m_paintLayer->projection(), event->point.toPoint(), &color)) {
        emit colorSelected(color);
    }
}

void KisScratchPad::setOnScreenResolution(qreal scaleX, qreal scaleY)
{
    m_scaleBorderWidth = BORDER_SIZE(qMax(scaleX, scaleY));

    m_scaleTransform = QTransform::fromScale(scaleX, scaleY);
    updateTransformations();
    update();
}

QTransform KisScratchPad::documentToWidget() const
{
    return m_translateTransform.inverted() * m_scaleTransform;
}

QTransform KisScratchPad::widgetToDocument() const
{
    return m_scaleTransform.inverted() * m_translateTransform;
}

void KisScratchPad::updateTransformations()
{
    m_eventFilter->setWidgetToDocumentTransform(widgetToDocument());
}

QRect KisScratchPad::imageBounds() const
{
    return widgetToDocument().mapRect(rect());
}

void KisScratchPad::imageUpdated(const QRect &rect)
{
    emit sigUpdateCanvas(documentToWidget().mapRect(QRectF(rect)).toAlignedRect());
}

void KisScratchPad::slotUpdateCanvas(const QRect &rect)
{
    update(rect);
}

void KisScratchPad::paintEvent ( QPaintEvent * event ) {
    if(!m_paintLayer) return;

    QRectF imageRect = widgetToDocument().mapRect(QRectF(event->rect()));

    QRect alignedImageRect =
        imageRect.adjusted(-m_scaleBorderWidth, -m_scaleBorderWidth,
                           m_scaleBorderWidth, m_scaleBorderWidth).toAlignedRect();

    QPointF offset = alignedImageRect.topLeft();

    m_paintLayer->projectionPlane()->recalculate(alignedImageRect, m_paintLayer);
    KisPaintDeviceSP projection = m_paintLayer->projection();

    QImage image = projection->convertToQImage(m_displayProfile,
                                               alignedImageRect.x(),
                                               alignedImageRect.y(),
                                               alignedImageRect.width(),
                                               alignedImageRect.height(),
                                               KoColorConversionTransformation::internalRenderingIntent(),
                                               KoColorConversionTransformation::internalConversionFlags());

    QPainter gc(this);
    gc.fillRect(event->rect(), m_checkBrush);

    gc.setRenderHints(QPainter::SmoothPixmapTransform);
    gc.drawImage(QRectF(event->rect()), image, imageRect.translated(-offset));

    QBrush brush(Qt::lightGray);
    QPen pen(brush, 1, Qt::DotLine);
    gc.setPen(pen);
    if (m_cutoutOverlay.isValid()) {
        gc.drawRect(m_cutoutOverlay);
    }

    if(!isEnabled()) {
        QColor color(Qt::lightGray);
        color.setAlphaF(0.5);
        QBrush disabledBrush(color);
        gc.fillRect(event->rect(), disabledBrush);
    }
    gc.end();
}

void KisScratchPad::setupScratchPad(KisCanvasResourceProvider* resourceProvider,
                                    const QColor &defaultColor)
{
    m_resourceProvider = resourceProvider;
    KisConfig cfg;
    setDisplayProfile(cfg.displayProfile(QApplication::desktop()->screenNumber(this)));
    connect(m_resourceProvider, SIGNAL(sigDisplayProfileChanged(const KoColorProfile*)),
            SLOT(setDisplayProfile(const KoColorProfile*)));

    connect(m_resourceProvider, SIGNAL(sigOnScreenResolutionChanged(qreal,qreal)),
            SLOT(setOnScreenResolution(qreal,qreal)));

    m_defaultColor = KoColor(defaultColor, KoColorSpaceRegistry::instance()->rgb8());

    KisPaintDeviceSP paintDevice =
        new KisPaintDevice(m_defaultColor.colorSpace(), "scratchpad");

    m_paintLayer = new KisPaintLayer(0, "ScratchPad", OPACITY_OPAQUE_U8, paintDevice);
    m_paintLayer->setGraphListener(m_nodeListener);
    m_paintLayer->paintDevice()->setDefaultBounds(new KisScratchPadDefaultBounds(this));

    fillDefault();
}

void KisScratchPad::setCutoutOverlayRect(const QRect& rc)
{
    m_cutoutOverlay = rc;
}

QImage KisScratchPad::cutoutOverlay() const
{
    if(!m_paintLayer) return QImage();
    KisPaintDeviceSP paintDevice = m_paintLayer->paintDevice();

    QRect rc = widgetToDocument().mapRect(m_cutoutOverlay);
    QImage rawImage = paintDevice->convertToQImage(0, rc.x(), rc.y(), rc.width(), rc.height(), KoColorConversionTransformation::internalRenderingIntent(), KoColorConversionTransformation::internalConversionFlags());

    QImage scaledImage = rawImage.scaled(m_cutoutOverlay.size(),
                                         Qt::IgnoreAspectRatio,
                                         Qt::SmoothTransformation);

    return scaledImage;
}

void KisScratchPad::setPresetImage(const QImage& image)
{
    m_presetImage = image;
}

void KisScratchPad::paintPresetImage()
{
    if(!m_paintLayer) return;
    KisPaintDeviceSP paintDevice = m_paintLayer->paintDevice();

    QRect overlayRect = widgetToDocument().mapRect(m_cutoutOverlay);
    QRect imageRect(QPoint(), overlayRect.size());

    QImage scaledImage = m_presetImage.scaled(overlayRect.size(),
                                              Qt::IgnoreAspectRatio,
                                              Qt::SmoothTransformation);

    KisPaintDeviceSP device = new KisPaintDevice(paintDevice->colorSpace());
    device->convertFromQImage(scaledImage, 0);

    KisPainter painter(paintDevice);
    painter.bitBlt(overlayRect.topLeft(), device, imageRect);
    update();
}

void KisScratchPad::setDisplayProfile(const KoColorProfile *colorProfile)
{
    m_displayProfile = colorProfile;
    QWidget::update();
}

void KisScratchPad::fillDefault()
{
    if(!m_paintLayer) return;
    KisPaintDeviceSP paintDevice = m_paintLayer->paintDevice();

    paintDevice->setDefaultPixel(m_defaultColor.data());
    paintDevice->clear();
    update();
}

void KisScratchPad::fillGradient()
{
    if(!m_paintLayer) return;
    KisPaintDeviceSP paintDevice = m_paintLayer->paintDevice();

    KoAbstractGradient* gradient = m_resourceProvider->currentGradient();
    QRect gradientRect = widgetToDocument().mapRect(rect());

    paintDevice->clear();

    KisGradientPainter painter(paintDevice);

    painter.setGradient(gradient);
    painter.setGradientShape(KisGradientPainter::GradientShapeLinear);
    painter.paintGradient(gradientRect.topLeft(),
                          gradientRect.bottomRight(),
                          KisGradientPainter::GradientRepeatNone,
                          0.2, false,
                          gradientRect.left(), gradientRect.top(),
                          gradientRect.width(), gradientRect.height());

    update();
}

void KisScratchPad::fillBackground()
{
    if(!m_paintLayer) return;
    KisPaintDeviceSP paintDevice = m_paintLayer->paintDevice();

    KoColor c(m_resourceProvider->bgColor(), paintDevice->colorSpace());
    paintDevice->setDefaultPixel(c.data());
    paintDevice->clear();
    update();
}

void KisScratchPad::fillLayer()
{
    // TODO
}
