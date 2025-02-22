/*
 *  Copyright (c) 2014 Dmitry Kazakov <dimula73@gmail.com>
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

#include "kis_free_transform_strategy.h"

#include <QPointF>
#include <QPainter>
#include <QMatrix4x4>

#include <KoResourcePaths.h>

#include "kis_coordinates_converter.h"
#include "tool_transform_args.h"
#include "transform_transaction_properties.h"
#include "krita_utils.h"
#include "kis_cursor.h"
#include "kis_transform_utils.h"
#include "kis_free_transform_strategy_gsl_helpers.h"


enum StrokeFunction {
    ROTATE = 0,
    MOVE,
    RIGHTSCALE,
    TOPRIGHTSCALE,
    TOPSCALE,
    TOPLEFTSCALE,
    LEFTSCALE,
    BOTTOMLEFTSCALE,
    BOTTOMSCALE,
    BOTTOMRIGHTSCALE,
    BOTTOMSHEAR,
    RIGHTSHEAR,
    TOPSHEAR,
    LEFTSHEAR,
    MOVECENTER,
    PERSPECTIVE
};

struct KisFreeTransformStrategy::Private
{
    Private(KisFreeTransformStrategy *_q,
            const KisCoordinatesConverter *_converter,
            ToolTransformArgs &_currentArgs,
            TransformTransactionProperties &_transaction)
        : q(_q),
          converter(_converter),
          currentArgs(_currentArgs),
          transaction(_transaction),
          imageTooBig(false)
    {
        scaleCursors[0] = KisCursor::sizeHorCursor();
        scaleCursors[1] = KisCursor::sizeFDiagCursor();
        scaleCursors[2] = KisCursor::sizeVerCursor();
        scaleCursors[3] = KisCursor::sizeBDiagCursor();
        scaleCursors[4] = KisCursor::sizeHorCursor();
        scaleCursors[5] = KisCursor::sizeFDiagCursor();
        scaleCursors[6] = KisCursor::sizeVerCursor();
        scaleCursors[7] = KisCursor::sizeBDiagCursor();

        shearCursorPixmap.load(KoResourcePaths::locate("data", "krita/icons/cursor_shear.png"));

    }

    KisFreeTransformStrategy *q;

    /// standard members ///

    const KisCoordinatesConverter *converter;

    //////
    ToolTransformArgs &currentArgs;
    //////
    TransformTransactionProperties &transaction;


    QTransform thumbToImageTransform;
    QImage originalImage;

    QTransform paintingTransform;
    QPointF paintingOffset;

    QTransform handlesTransform;

    /// custom members ///

    StrokeFunction function;

    struct HandlePoints {
        QPointF topLeft;
        QPointF topMiddle;
        QPointF topRight;

        QPointF middleLeft;
        QPointF rotationCenter;
        QPointF middleRight;

        QPointF bottomLeft;
        QPointF bottomMiddle;
        QPointF bottomRight;
    };
    HandlePoints transformedHandles;

    QTransform transform;

    QCursor scaleCursors[8]; // cursors for the 8 directions
    QPixmap shearCursorPixmap;

    bool imageTooBig;

    ToolTransformArgs clickArgs;
    QPointF clickPos;

    QCursor getScaleCursor(const QPointF &handlePt);
    QCursor getShearCursor(const QPointF &start, const QPointF &end);
    void recalculateTransformations();
    void recalculateTransformedHandles();
};

KisFreeTransformStrategy::KisFreeTransformStrategy(const KisCoordinatesConverter *converter,
                                                   ToolTransformArgs &currentArgs,
                                                   TransformTransactionProperties &transaction)
    : KisSimplifiedActionPolicyStrategy(converter),
      m_d(new Private(this, converter, currentArgs, transaction))
{
}

KisFreeTransformStrategy::~KisFreeTransformStrategy()
{
}

void KisFreeTransformStrategy::Private::recalculateTransformedHandles()
{
    transformedHandles.topLeft = transform.map(transaction.originalTopLeft());
    transformedHandles.topMiddle = transform.map(transaction.originalMiddleTop());
    transformedHandles.topRight = transform.map(transaction.originalTopRight());

    transformedHandles.middleLeft = transform.map(transaction.originalMiddleLeft());
    transformedHandles.rotationCenter = transform.map(currentArgs.originalCenter() + currentArgs.rotationCenterOffset());
    transformedHandles.middleRight = transform.map(transaction.originalMiddleRight());

    transformedHandles.bottomLeft = transform.map(transaction.originalBottomLeft());
    transformedHandles.bottomMiddle = transform.map(transaction.originalMiddleBottom());
    transformedHandles.bottomRight = transform.map(transaction.originalBottomRight());
}

void KisFreeTransformStrategy::setTransformFunction(const QPointF &mousePos, bool perspectiveModifierActive)
{
    if (perspectiveModifierActive) {
        m_d->function = PERSPECTIVE;
        return;
    }

    QPolygonF transformedPolygon = m_d->transform.map(QPolygonF(m_d->transaction.originalRect()));
    qreal handleRadius = KisTransformUtils::effectiveHandleGrabRadius(m_d->converter);
    qreal rotationHandleRadius = KisTransformUtils::effectiveHandleGrabRadius(m_d->converter);


    StrokeFunction defaultFunction =
        transformedPolygon.containsPoint(mousePos, Qt::OddEvenFill) ? MOVE : ROTATE;
    KisTransformUtils::HandleChooser<StrokeFunction>
        handleChooser(mousePos, defaultFunction);

    handleChooser.addFunction(m_d->transformedHandles.topMiddle,
                              handleRadius, TOPSCALE);
    handleChooser.addFunction(m_d->transformedHandles.topRight,
                              handleRadius, TOPRIGHTSCALE);
    handleChooser.addFunction(m_d->transformedHandles.middleRight,
                              handleRadius, RIGHTSCALE);

    handleChooser.addFunction(m_d->transformedHandles.bottomRight,
                              handleRadius, BOTTOMRIGHTSCALE);
    handleChooser.addFunction(m_d->transformedHandles.bottomMiddle,
                              handleRadius, BOTTOMSCALE);
    handleChooser.addFunction(m_d->transformedHandles.bottomLeft,
                              handleRadius, BOTTOMLEFTSCALE);
    handleChooser.addFunction(m_d->transformedHandles.middleLeft,
                              handleRadius, LEFTSCALE);
    handleChooser.addFunction(m_d->transformedHandles.topLeft,
                              handleRadius, TOPLEFTSCALE);
    handleChooser.addFunction(m_d->transformedHandles.rotationCenter,
                              rotationHandleRadius, MOVECENTER);

    m_d->function = handleChooser.function();

    if (m_d->function == ROTATE || m_d->function == MOVE) {
        QRectF originalRect = m_d->transaction.originalRect();
        QPointF t = m_d->transform.inverted().map(mousePos);

        if (t.x() >= originalRect.left() && t.x() <= originalRect.right()) {
            if (fabs(t.y() - originalRect.top()) <= handleRadius)
                m_d->function = TOPSHEAR;
            if (fabs(t.y() - originalRect.bottom()) <= handleRadius)
                m_d->function = BOTTOMSHEAR;
        }
        if (t.y() >= originalRect.top() && t.y() <= originalRect.bottom()) {
            if (fabs(t.x() - originalRect.left()) <= handleRadius)
                m_d->function = LEFTSHEAR;
            if (fabs(t.x() - originalRect.right()) <= handleRadius)
                m_d->function = RIGHTSHEAR;
        }
    }
}

QCursor KisFreeTransformStrategy::Private::getScaleCursor(const QPointF &handlePt)
{
    QPointF handlePtInWidget = converter->imageToWidget(handlePt);
    QPointF centerPtInWidget = converter->imageToWidget(currentArgs.transformedCenter());

    QPointF direction = handlePtInWidget - centerPtInWidget;
    qreal angle = atan2(direction.y(), direction.x());
    angle = normalizeAngle(angle);

    int octant = qRound(angle * 4. / M_PI) % 8;
    return scaleCursors[octant];
}

QCursor KisFreeTransformStrategy::Private::getShearCursor(const QPointF &start, const QPointF &end)
{
    QPointF startPtInWidget = converter->imageToWidget(start);
    QPointF endPtInWidget = converter->imageToWidget(end);
    QPointF direction = endPtInWidget - startPtInWidget;

    qreal angle = atan2(-direction.y(), direction.x());
    return QCursor(shearCursorPixmap.transformed(QTransform().rotateRadians(-angle)));
}

QCursor KisFreeTransformStrategy::getCurrentCursor() const
{
    QCursor cursor;

    switch (m_d->function) {
    case MOVE:
        cursor = KisCursor::moveCursor();
        break;
    case ROTATE:
        cursor = KisCursor::rotateCursor();
        break;
    case PERSPECTIVE:
        //TODO: find another cursor for perspective
        cursor = KisCursor::rotateCursor();
        break;
    case RIGHTSCALE:
        cursor = m_d->getScaleCursor(m_d->transformedHandles.middleRight);
        break;
    case TOPSCALE:
        cursor = m_d->getScaleCursor(m_d->transformedHandles.topMiddle);
        break;
    case LEFTSCALE:
        cursor = m_d->getScaleCursor(m_d->transformedHandles.middleLeft);
        break;
    case BOTTOMSCALE:
        cursor = m_d->getScaleCursor(m_d->transformedHandles.bottomMiddle);
        break;
    case TOPRIGHTSCALE:
        cursor = m_d->getScaleCursor(m_d->transformedHandles.topRight);
        break;
    case BOTTOMLEFTSCALE:
        cursor = m_d->getScaleCursor(m_d->transformedHandles.bottomLeft);
        break;
    case TOPLEFTSCALE:
        cursor = m_d->getScaleCursor(m_d->transformedHandles.topLeft);
        break;
    case BOTTOMRIGHTSCALE:
        cursor = m_d->getScaleCursor(m_d->transformedHandles.bottomRight);
        break;
    case MOVECENTER:
        cursor = KisCursor::handCursor();
        break;
    case BOTTOMSHEAR:
        cursor = m_d->getShearCursor(m_d->transformedHandles.bottomLeft, m_d->transformedHandles.bottomRight);
        break;
    case RIGHTSHEAR:
        cursor = m_d->getShearCursor(m_d->transformedHandles.bottomRight, m_d->transformedHandles.topRight);
        break;
    case TOPSHEAR:
        cursor = m_d->getShearCursor(m_d->transformedHandles.topRight, m_d->transformedHandles.topLeft);
        break;
    case LEFTSHEAR:
        cursor = m_d->getShearCursor(m_d->transformedHandles.topLeft, m_d->transformedHandles.bottomLeft);
        break;
    }

    return cursor;
}

void KisFreeTransformStrategy::paint(QPainter &gc)
{
    gc.save();

    gc.setOpacity(m_d->transaction.basePreviewOpacity());
    gc.setTransform(m_d->paintingTransform, true);
    gc.drawImage(m_d->paintingOffset, originalImage());

    gc.restore();

    // Draw Handles

    qreal d = 1;
    QRectF handleRect =
        KisTransformUtils::handleRect(KisTransformUtils::handleVisualRadius,
                                      m_d->handlesTransform,
                                      m_d->transaction.originalRect(),
                                      &d);

    qreal r = 1;
    QRectF rotationCenterRect =
        KisTransformUtils::handleRect(KisTransformUtils::rotationHandleVisualRadius,
                                      m_d->handlesTransform,
                                      m_d->transaction.originalRect(),
                                      &r);

    QPainterPath handles;

    handles.moveTo(m_d->transaction.originalTopLeft());
    handles.lineTo(m_d->transaction.originalTopRight());
    handles.lineTo(m_d->transaction.originalBottomRight());
    handles.lineTo(m_d->transaction.originalBottomLeft());
    handles.lineTo(m_d->transaction.originalTopLeft());

    handles.addRect(handleRect.translated(m_d->transaction.originalTopLeft()));
    handles.addRect(handleRect.translated(m_d->transaction.originalTopRight()));
    handles.addRect(handleRect.translated(m_d->transaction.originalBottomLeft()));
    handles.addRect(handleRect.translated(m_d->transaction.originalBottomRight()));
    handles.addRect(handleRect.translated(m_d->transaction.originalMiddleLeft()));
    handles.addRect(handleRect.translated(m_d->transaction.originalMiddleRight()));
    handles.addRect(handleRect.translated(m_d->transaction.originalMiddleTop()));
    handles.addRect(handleRect.translated(m_d->transaction.originalMiddleBottom()));

    QPointF rotationCenter = m_d->currentArgs.originalCenter() + m_d->currentArgs.rotationCenterOffset();
    QPointF dx(r + 3, 0);
    QPointF dy(0, r + 3);
    handles.addEllipse(rotationCenterRect.translated(rotationCenter));
    handles.moveTo(rotationCenter - dx);
    handles.lineTo(rotationCenter + dx);
    handles.moveTo(rotationCenter - dy);
    handles.lineTo(rotationCenter + dy);

    gc.save();

    //gc.setTransform(m_d->handlesTransform, true); <-- don't do like this!
    QPainterPath mappedHandles = m_d->handlesTransform.map(handles);

    QPen pen[2];
    pen[0].setWidth(1);
    pen[1].setWidth(2);
    pen[1].setColor(Qt::lightGray);

    for (int i = 1; i >= 0; --i) {
        gc.setPen(pen[i]);
        gc.drawPath(mappedHandles);
    }

    gc.restore();
}

void KisFreeTransformStrategy::externalConfigChanged()
{
    m_d->recalculateTransformations();
}

bool KisFreeTransformStrategy::beginPrimaryAction(const QPointF &pt)
{
    m_d->clickArgs = m_d->currentArgs;
    m_d->clickPos = pt;

    return true;
}

void KisFreeTransformStrategy::continuePrimaryAction(const QPointF &mousePos, bool specialModifierActive)
{
    switch (m_d->function) {
    case MOVE: {
        QPointF diff = mousePos - m_d->clickPos;

        if (specialModifierActive) {

            KisTransformUtils::MatricesPack m(m_d->clickArgs);
            QTransform t = m.S * m.projectedP;
            QPointF originalDiff = t.inverted().map(diff);

            if (qAbs(originalDiff.x()) >= qAbs(originalDiff.y())) {
                originalDiff.setY(0);
            } else {
                originalDiff.setX(0);
            }

            diff = t.map(originalDiff);

        }

        m_d->currentArgs.setTransformedCenter(m_d->clickArgs.transformedCenter() + diff);

        break;
    }
    case ROTATE:
    {
        KisTransformUtils::MatricesPack clickM(m_d->clickArgs);
        QTransform clickT = clickM.finalTransform();

        QPointF rotationCenter = m_d->clickArgs.originalCenter() + m_d->clickArgs.rotationCenterOffset();
        QPointF clickMouseImagePos = clickT.inverted().map(m_d->clickPos) - rotationCenter;
        QPointF mouseImagePos = clickT.inverted().map(mousePos) - rotationCenter;

        qreal a1 = atan2(clickMouseImagePos.y(), clickMouseImagePos.x());
        qreal a2 = atan2(mouseImagePos.y(), mouseImagePos.x());

        double theta = -a1 + a2;

        m_d->currentArgs.setAZ(normalizeAngle(m_d->clickArgs.aZ() + theta));

        KisTransformUtils::MatricesPack m(m_d->currentArgs);
        QTransform t = m.finalTransform();
        QPointF newRotationCenter = t.map(m_d->currentArgs.originalCenter() + m_d->currentArgs.rotationCenterOffset());
        QPointF oldRotationCenter = clickT.map(m_d->clickArgs.originalCenter() + m_d->clickArgs.rotationCenterOffset());

        m_d->currentArgs.setTransformedCenter(m_d->currentArgs.transformedCenter() + oldRotationCenter - newRotationCenter);
    }
    break;
    case PERSPECTIVE:
    {
        QPointF diff = mousePos - m_d->clickPos;
        double thetaX = - diff.y() * M_PI / m_d->transaction.originalHalfHeight() / 2 / fabs(m_d->currentArgs.scaleY());
        m_d->currentArgs.setAX(normalizeAngle(m_d->clickArgs.aX() + thetaX));

        qreal sign = qAbs(m_d->currentArgs.aX() - M_PI) < M_PI / 2 ? -1.0 : 1.0;
        double thetaY = sign * diff.x() * M_PI / m_d->transaction.originalHalfWidth() / 2 / fabs(m_d->currentArgs.scaleX());
        m_d->currentArgs.setAY(normalizeAngle(m_d->clickArgs.aY() + thetaY));

        KisTransformUtils::MatricesPack m(m_d->currentArgs);
        QTransform t = m.finalTransform();
        QPointF newRotationCenter = t.map(m_d->currentArgs.originalCenter() + m_d->currentArgs.rotationCenterOffset());

        KisTransformUtils::MatricesPack clickM(m_d->clickArgs);
        QTransform clickT = clickM.finalTransform();
        QPointF oldRotationCenter = clickT.map(m_d->clickArgs.originalCenter() + m_d->clickArgs.rotationCenterOffset());

        m_d->currentArgs.setTransformedCenter(m_d->currentArgs.transformedCenter() + oldRotationCenter - newRotationCenter);
    }
    break;
    case TOPSCALE:
    case BOTTOMSCALE: {
        QPointF staticPoint;
        QPointF movingPoint;
        qreal extraSign;

        if (m_d->function == TOPSCALE) {

            staticPoint = m_d->transaction.originalMiddleBottom();
            movingPoint = m_d->transaction.originalMiddleTop();
            extraSign = -1.0;
        } else {
            staticPoint = m_d->transaction.originalMiddleTop();
            movingPoint = m_d->transaction.originalMiddleBottom();
            extraSign = 1.0;
        }

        QPointF mouseImagePos = m_d->transform.inverted().map(mousePos);

        qreal sign = mouseImagePos.y() <= staticPoint.y() ? -extraSign : extraSign;
        m_d->currentArgs.setScaleY(sign * m_d->currentArgs.scaleY());

        QPointF staticPointInView = m_d->transform.map(staticPoint);

        qreal dist = kisDistance(staticPointInView, mousePos);

        GSL::ScaleResult1D result =
            GSL::calculateScaleY(m_d->currentArgs,
                                 staticPoint,
                                 staticPointInView,
                                 movingPoint,
                                 dist);

        if (specialModifierActive ||  m_d->currentArgs.keepAspectRatio()) {
            qreal aspectRatio = m_d->clickArgs.scaleX() / m_d->clickArgs.scaleY();
            m_d->currentArgs.setScaleX(aspectRatio * result.scale);
        }

        m_d->currentArgs.setScaleY(result.scale);
        m_d->currentArgs.setTransformedCenter(result.transformedCenter);
        break;
    }

    case LEFTSCALE:
    case RIGHTSCALE: {
        QPointF staticPoint;
        QPointF movingPoint;
        qreal extraSign;

        if (m_d->function == LEFTSCALE) {

            staticPoint = m_d->transaction.originalMiddleRight();
            movingPoint = m_d->transaction.originalMiddleLeft();
            extraSign = -1.0;
        } else {
            staticPoint = m_d->transaction.originalMiddleLeft();
            movingPoint = m_d->transaction.originalMiddleRight();
            extraSign = 1.0;
        }

        QPointF mouseImagePos = m_d->transform.inverted().map(mousePos);

        qreal sign = mouseImagePos.x() <= staticPoint.x() ? -extraSign : extraSign;
        m_d->currentArgs.setScaleX(sign * m_d->currentArgs.scaleX());

        QPointF staticPointInView = m_d->transform.map(staticPoint);

        qreal dist = kisDistance(staticPointInView, mousePos);

        GSL::ScaleResult1D result =
            GSL::calculateScaleX(m_d->currentArgs,
                                 staticPoint,
                                 staticPointInView,
                                 movingPoint,
                                 dist);

        if (specialModifierActive  ||  m_d->currentArgs.keepAspectRatio()) {
            qreal aspectRatio = m_d->clickArgs.scaleY() / m_d->clickArgs.scaleX();
            m_d->currentArgs.setScaleY(aspectRatio * result.scale);
        }

        m_d->currentArgs.setScaleX(result.scale);
        m_d->currentArgs.setTransformedCenter(result.transformedCenter);
        break;
    }
    case TOPRIGHTSCALE:
    case BOTTOMRIGHTSCALE:
    case TOPLEFTSCALE:
    case BOTTOMLEFTSCALE: {
        QPointF staticPoint;
        QPointF movingPoint;

        if (m_d->function == TOPRIGHTSCALE) {
            staticPoint = m_d->transaction.originalBottomLeft();
            movingPoint = m_d->transaction.originalTopRight();
        } else if (m_d->function == BOTTOMRIGHTSCALE) {
            staticPoint = m_d->transaction.originalTopLeft();
            movingPoint = m_d->transaction.originalBottomRight();
        } else if (m_d->function == TOPLEFTSCALE) {
            staticPoint = m_d->transaction.originalBottomRight();
            movingPoint = m_d->transaction.originalTopLeft();
        } else {
            staticPoint = m_d->transaction.originalTopRight();
            movingPoint = m_d->transaction.originalBottomLeft();
        }

        QPointF staticPointInView = m_d->transform.map(staticPoint);
        QPointF movingPointInView = mousePos;

        if (specialModifierActive  ||  m_d->currentArgs.keepAspectRatio()) {
            KisTransformUtils::MatricesPack m(m_d->clickArgs);
            QTransform t = m.finalTransform();

            QPointF refDiff = t.map(movingPoint) - staticPointInView;
            QPointF realDiff = mousePos - staticPointInView;
            realDiff = kisProjectOnVector(refDiff, realDiff);

            movingPointInView = staticPointInView + realDiff;
        }

        GSL::ScaleResult2D result =
            GSL::calculateScale2D(m_d->currentArgs,
                                  staticPoint,
                                  staticPointInView,
                                  movingPoint,
                                  movingPointInView);

        m_d->currentArgs.setScaleX(result.scaleX);
        m_d->currentArgs.setScaleY(result.scaleY);
        m_d->currentArgs.setTransformedCenter(result.transformedCenter);
        break;
    }
    case MOVECENTER: {
        QPointF pt = m_d->transform.inverted().map(mousePos);
        pt = KisTransformUtils::clipInRect(pt, m_d->transaction.originalRect());

        QPointF newRotationCenterOffset = pt - m_d->currentArgs.originalCenter();

        if (specialModifierActive) {
            if (qAbs(newRotationCenterOffset.x()) > qAbs(newRotationCenterOffset.y())) {
                newRotationCenterOffset.ry() = 0;
            } else {
                newRotationCenterOffset.rx() = 0;
            }
        }

        m_d->currentArgs.setRotationCenterOffset(newRotationCenterOffset);
        emit requestResetRotationCenterButtons();
    }
        break;
    case TOPSHEAR:
    case BOTTOMSHEAR: {
        KisTransformUtils::MatricesPack m(m_d->clickArgs);

        QTransform backwardT = (m.S * m.projectedP).inverted();
        QPointF diff = backwardT.map(mousePos - m_d->clickPos);

        qreal sign = m_d->function == BOTTOMSHEAR ? 1.0 : -1.0;

        // get the dx pixels corresponding to the current shearX factor
        qreal dx = sign * m_d->clickArgs.shearX() * m_d->clickArgs.scaleY() * m_d->transaction.originalHalfHeight(); // get the dx pixels corresponding to the current shearX factor
        dx += diff.x();

        // calculate the new shearX factor
        m_d->currentArgs.setShearX(sign * dx / m_d->currentArgs.scaleY() / m_d->transaction.originalHalfHeight()); // calculate the new shearX factor
        break;
    }

    case LEFTSHEAR:
    case RIGHTSHEAR: {
        KisTransformUtils::MatricesPack m(m_d->clickArgs);

        QTransform backwardT = (m.S * m.projectedP).inverted();
        QPointF diff = backwardT.map(mousePos - m_d->clickPos);

        qreal sign = m_d->function == RIGHTSHEAR ? 1.0 : -1.0;

        // get the dx pixels corresponding to the current shearX factor
        qreal dy = sign *  m_d->clickArgs.shearY() * m_d->clickArgs.scaleX() * m_d->transaction.originalHalfWidth();
        dy += diff.y();

        // calculate the new shearY factor
        m_d->currentArgs.setShearY(sign * dy / m_d->clickArgs.scaleX() / m_d->transaction.originalHalfWidth());
        break;
    }
    }

    m_d->recalculateTransformations();
}

bool KisFreeTransformStrategy::endPrimaryAction()
{
    bool shouldSave = !m_d->imageTooBig;

    if (m_d->imageTooBig) {
        m_d->currentArgs = m_d->clickArgs;
        m_d->recalculateTransformations();
    }

    return shouldSave;
}

void KisFreeTransformStrategy::Private::recalculateTransformations()
{
    KisTransformUtils::MatricesPack m(currentArgs);
    QTransform sanityCheckMatrix = m.TS * m.SC * m.S * m.projectedP;

    /**
     * The center of the original image should still
     * stay the origin of CS
     */
    KIS_ASSERT_RECOVER_NOOP(sanityCheckMatrix.map(currentArgs.originalCenter()).manhattanLength() < 1e-4);

    transform = m.finalTransform();

    QTransform viewScaleTransform = converter->imageToDocumentTransform() * converter->documentToFlakeTransform();
    handlesTransform = transform * viewScaleTransform;

    QTransform tl = QTransform::fromTranslate(transaction.originalTopLeft().x(), transaction.originalTopLeft().y());
    paintingTransform = tl.inverted() * q->thumbToImageTransform() * tl * transform * viewScaleTransform;
    paintingOffset = transaction.originalTopLeft();

    // check whether image is too big to be displayed or not
    imageTooBig = KisTransformUtils::checkImageTooBig(transaction.originalRect(), m);

    // recalculate cached handles position
    recalculateTransformedHandles();

    emit q->requestShowImageTooBig(imageTooBig);
}
