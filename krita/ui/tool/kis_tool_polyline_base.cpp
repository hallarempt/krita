/* This file is part of the KDE project
 * Copyright (C) 2009 Boudewijn Rempt <boud@valdyas.org>
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

#include <QList>

#include <QAction>

#include <KoPointerEvent.h>
#include <KoCanvasBase.h>
#include <KoCanvasController.h>
#include <KoViewConverter.h>

#include "kis_tool_polyline_base.h"
#include "kis_canvas2.h"
#include <KisViewManager.h>
#include <kis_action.h>
#include <kactioncollection.h>

#include "kis_action_registry.h"

#define SNAPPING_THRESHOLD 10
#define SNAPPING_HANDLE_RADIUS 8
#define PREVIEW_LINE_WIDTH 1

KisToolPolylineBase::KisToolPolylineBase(KoCanvasBase * canvas,  KisToolPolylineBase::ToolType type, const QCursor & cursor)
    : KisToolShape(canvas, cursor),
      m_dragging(false),
      m_type(type),
      m_closeSnappingActivated(false)
{
    QAction *undo_polygon_selection =
        KisActionRegistry::instance()->makeQAction("undo_polygon_selection", this);
    addAction("undo_polygon_selection", undo_polygon_selection);
}


void KisToolPolylineBase::activate(KoToolBase::ToolActivation activation, const QSet<KoShape *> &shapes)
{
    KisToolShape::activate(activation, shapes);
    connect(actions().value("undo_polygon_selection"), SIGNAL(triggered()), SLOT(undoSelection()), Qt::UniqueConnection);
}

void KisToolPolylineBase::deactivate()
{
    disconnect(actions().value("undo_polygon_selection"), 0, this, 0);
    cancelStroke();
    KisToolShape::deactivate();
}

void KisToolPolylineBase::requestStrokeEnd()
{
    endStroke();
}

void KisToolPolylineBase::requestStrokeCancellation()
{
    cancelStroke();
}

void KisToolPolylineBase::beginPrimaryAction(KoPointerEvent *event)
{
    Q_UNUSED(event);

    if ((m_type == PAINT && (!nodeEditable() || nodePaintAbility() == NONE)) ||
        (m_type == SELECT && !selectionEditable())) {

        event->ignore();
        return;
    }

    setMode(KisTool::PAINT_MODE);

    if(m_dragging && m_closeSnappingActivated) {
        m_points.append(m_points.first());
        endStroke();
    } else {
        m_dragging = true;
    }
}

void KisToolPolylineBase::endPrimaryAction(KoPointerEvent *event)
{
    CHECK_MODE_SANITY_OR_RETURN(KisTool::PAINT_MODE);
    setMode(KisTool::HOVER_MODE);

    if(m_dragging) {
        m_dragStart = convertToPixelCoord(event);
        m_dragEnd = m_dragStart;
        m_points.append(m_dragStart);
    }
}

void KisToolPolylineBase::beginPrimaryDoubleClickAction(KoPointerEvent *event)
{
    endStroke();

    // this action will have no continuation
    event->ignore();
}

void KisToolPolylineBase::beginAlternateAction(KoPointerEvent *event, AlternateAction action)
{
    if (action != ChangeSize || !m_dragging) {
        KisToolPaint::beginAlternateAction(event, action);
    }

    if (m_closeSnappingActivated) {
        m_points.append(m_points.first());
    }
    endStroke();
}

void KisToolPolylineBase::mouseMoveEvent(KoPointerEvent *event)
{
    if (m_dragging && !m_points.empty()) {
        // erase old lines on canvas
        QRectF updateRect = dragBoundingRect();
        // get current mouse position
        m_dragEnd = convertToPixelCoord(event);
        // draw new lines on canvas
        updateRect |= dragBoundingRect();
        updateCanvasViewRect(updateRect);


        QPointF basePoint = pixelToView(m_points.first());
        m_closeSnappingActivated =
            m_points.size() > 1 &&
            (basePoint - pixelToView(m_dragEnd)).manhattanLength() < SNAPPING_THRESHOLD;

        updateCanvasViewRect(QRectF(basePoint, 2 * QSize(SNAPPING_HANDLE_RADIUS + PREVIEW_LINE_WIDTH, SNAPPING_HANDLE_RADIUS + PREVIEW_LINE_WIDTH)).translated(-SNAPPING_HANDLE_RADIUS + PREVIEW_LINE_WIDTH,-SNAPPING_HANDLE_RADIUS + PREVIEW_LINE_WIDTH));
        KisToolPaint::requestUpdateOutline(event->point, event);
    } else {
        KisToolPaint::mouseMoveEvent(event);
    }
}

void KisToolPolylineBase::undoSelection()
{
    if(m_dragging) {
        //Update canvas for drag before undo
        QRectF updateRect = dragBoundingRect();
        updateRect |= dragBoundingRect();
        updateCanvasViewRect(updateRect);

        //Update canvas for last segment
        QRectF rect;
        if (m_points.size() > 2) {
            rect = pixelToView(QRectF(m_points.last(), m_points.at(m_points.size()-2)).normalized());
            rect.adjust(-PREVIEW_LINE_WIDTH, -PREVIEW_LINE_WIDTH, PREVIEW_LINE_WIDTH, PREVIEW_LINE_WIDTH);
            rect |= rect;
            updateCanvasViewRect(rect);
        }
        if (m_points.size() > 0) {
            m_points.pop_back();
        }
        if (m_points.size() > 0) {
            m_dragStart = m_points.last();
        }
    }
}

void KisToolPolylineBase::paint(QPainter& gc, const KoViewConverter &converter)
{
    Q_UNUSED(converter);

    if (!canvas() || !currentImage())
        return;

    QPointF start, end;
    QPointF startPos;
    QPointF endPos;

    QPainterPath path;
    if (m_dragging && !m_points.empty()) {
        startPos = pixelToView(m_dragStart);
        endPos = pixelToView(m_dragEnd);
        path.moveTo(startPos);
        path.lineTo(endPos);
    }

    for (vQPointF::iterator it = m_points.begin(); it != m_points.end(); ++it) {

        if (it == m_points.begin()) {
            start = (*it);
        } else {
            end = (*it);

            startPos = pixelToView(start);
            endPos = pixelToView(end);
            path.moveTo(startPos);
            path.lineTo(endPos);
            start = end;
        }
    }

    if (m_closeSnappingActivated) {
        QPointF basePoint = pixelToView(m_points.first());
        path.addEllipse(basePoint, SNAPPING_HANDLE_RADIUS, SNAPPING_HANDLE_RADIUS);
    }

    paintToolOutline(&gc, path);
    KisToolPaint::paint(gc,converter);
}

void KisToolPolylineBase::updateArea()
{
    updateCanvasPixelRect(image()->bounds());
}

void KisToolPolylineBase::endStroke()
{
    if (!m_dragging) return;

    m_dragging = false;
    if(m_points.count() > 1) {
        finishPolyline(m_points);
    }
    m_points.clear();
    m_closeSnappingActivated = false;
    updateArea();
}

void KisToolPolylineBase::cancelStroke()
{
    if (!m_dragging) return;

    m_dragging = false;
    m_points.clear();
    m_closeSnappingActivated = false;
    updateArea();
}

QRectF KisToolPolylineBase::dragBoundingRect()
{
    QRectF rect = pixelToView(QRectF(m_dragStart, m_dragEnd).normalized());
    rect.adjust(-PREVIEW_LINE_WIDTH, -PREVIEW_LINE_WIDTH, PREVIEW_LINE_WIDTH, PREVIEW_LINE_WIDTH);
    return rect;
}

void KisToolPolylineBase::listenToModifiers(bool listen)
{
    Q_UNUSED(listen)
}

bool KisToolPolylineBase::listeningToModifiers()
{
    //Never grab modifier keys
    return false;
}
