/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Reginald Stadlbauer <reggie@kde.org>
   Copyright (C) 2006 Peter Simonsson <peter.simonsson@gmail.com>
   Copyright (C) 2007 C. Boemann <cbo@boemann.dk>
   Copyright (C) 2007-2008 Jan Hambrecht <jaham@gmx.net>
   Copyright (C) 2007 Thomas Zander <zander@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "KoRuler.h"

#include "KoRuler_p.h"
#include <KoToolBase.h>
#include <KoToolManager.h>

#include <klocalizedstring.h>
#include <WidgetsDebug.h>

#include <QPainter>
#include <QResizeEvent>
#include <QMenu>
#include <QMouseEvent>
#include <QFontDatabase>

#include <KoViewConverter.h>

// the distance in pixels of a mouse position considered outside the rule
static const int OutsideRulerThreshold = 20;
//
static const int fullStepMarkerLength = 6;
static const int halfStepMarkerLength = 6;
static const int quarterStepMarkerLength = 3;
static const int measurementTextAboveBelowMargin = 1;


void RulerTabChooser::mousePressEvent(QMouseEvent *)
{
    if (! m_showTabs) {
        return;
    }

    switch(m_type) {
    case QTextOption::LeftTab:
        m_type = QTextOption::RightTab;
        break;
    case QTextOption::RightTab:
        m_type = QTextOption::CenterTab;
        break;
    case QTextOption::CenterTab:
        m_type = QTextOption::DelimiterTab;
        break;
    case QTextOption::DelimiterTab:
        m_type = QTextOption::LeftTab;
        break;
    }
    update();
}

void RulerTabChooser::paintEvent(QPaintEvent *)
{
    if (! m_showTabs) {
        return;
    }

    QPainter painter(this);
    QPolygonF polygon;

    painter.setPen(palette().color(QPalette::Text));
    painter.setBrush(palette().color(QPalette::Text));
    painter.setRenderHint( QPainter::Antialiasing );

    qreal x= width()/2;
    painter.translate(0,-height()/2+5);

    switch (m_type) {
    case QTextOption::LeftTab:
        polygon << QPointF(x+0.5, height() - 8.5)
            << QPointF(x+6.5, height() - 2.5)
            << QPointF(x+0.5, height() - 2.5);
        painter.drawPolygon(polygon);
        break;
    case QTextOption::RightTab:
        polygon << QPointF(x+0.5, height() - 8.5)
            << QPointF(x-5.5, height() - 2.5)
            << QPointF(x+0.5, height() - 2.5);
        painter.drawPolygon(polygon);
        break;
    case QTextOption::CenterTab:
        polygon << QPointF(x+0.5, height() - 8.5)
            << QPointF(x-5.5, height() - 2.5)
            << QPointF(x+6.5, height() - 2.5);
        painter.drawPolygon(polygon);
        break;
    case QTextOption::DelimiterTab:
        polygon << QPointF(x-5.5, height() - 2.5)
            << QPointF(x+6.5, height() - 2.5);
        painter.drawPolyline(polygon);
        polygon << QPointF(x+0.5, height() - 2.5)
            << QPointF(x+0.5, height() - 8.5);
        painter.drawPolyline(polygon);
        break;
    default:
        break;
    }
}

static int compareTabs(KoRuler::Tab &tab1, KoRuler::Tab &tab2)
{
    return tab1.position < tab2.position;
}

QRectF HorizontalPaintingStrategy::drawBackground(const KoRulerPrivate *d, QPainter &painter)
{
    lengthInPixel = d->viewConverter->documentToViewX(d->rulerLength);
    QRectF rectangle;
    rectangle.setX(qMax(0, d->offset));
    rectangle.setY(0);
    rectangle.setWidth(qMin(qreal(d->ruler->width() - 1.0 - rectangle.x()),
                            (d->offset >= 0) ? lengthInPixel : lengthInPixel + d->offset));
    rectangle.setHeight(d->ruler->height() - 1);
    QRectF activeRangeRectangle;
    activeRangeRectangle.setX(qMax(rectangle.x() + 1,
          d->viewConverter->documentToViewX(d->effectiveActiveRangeStart()) + d->offset));
    activeRangeRectangle.setY(rectangle.y() + 1);
    activeRangeRectangle.setRight(qMin(rectangle.right() - 1,
          d->viewConverter->documentToViewX(d->effectiveActiveRangeEnd()) + d->offset));
    activeRangeRectangle.setHeight(rectangle.height() - 2);

    painter.setPen(d->ruler->palette().color(QPalette::Mid));
    painter.drawRect(rectangle);

    if(d->effectiveActiveRangeStart() != d->effectiveActiveRangeEnd())
        painter.fillRect(activeRangeRectangle, d->ruler->palette().brush(QPalette::Base));

    if(d->showSelectionBorders) {
        // Draw first selection border
        if(d->firstSelectionBorder > 0) {
            qreal border = d->viewConverter->documentToViewX(d->firstSelectionBorder) + d->offset;
            painter.drawLine(QPointF(border, rectangle.y() + 1), QPointF(border, rectangle.bottom() - 1));
        }
        // Draw second selection border
        if(d->secondSelectionBorder > 0) {
            qreal border = d->viewConverter->documentToViewX(d->secondSelectionBorder) + d->offset;
            painter.drawLine(QPointF(border, rectangle.y() + 1), QPointF(border, rectangle.bottom() - 1));
        }
    }

    return rectangle;
}

void HorizontalPaintingStrategy::drawTabs(const KoRulerPrivate *d, QPainter &painter)
{
    if (! d->showTabs)
        return;
    QPolygonF polygon;

    const QColor tabColor = d->ruler->palette().color(QPalette::Text);
    painter.setPen(tabColor);
    painter.setBrush(tabColor);
    painter.setRenderHint( QPainter::Antialiasing );

    qreal position = -10000;

    foreach (const KoRuler::Tab & t, d->tabs) {
        qreal x;
        if (d->rightToLeft) {
            x = d->viewConverter->documentToViewX(d->effectiveActiveRangeEnd()
                    - (d->relativeTabs ? d->paragraphIndent : 0) - t.position) + d->offset;
        } else {
            x = d->viewConverter->documentToViewX(d->effectiveActiveRangeStart()
                    + (d->relativeTabs ? d->paragraphIndent : 0) + t.position) + d->offset;
        }
        position = qMax(position, t.position);

        polygon.clear();
        switch (t.type) {
        case QTextOption::LeftTab:
            polygon << QPointF(x+0.5, d->ruler->height() - 6.5)
                << QPointF(x+6.5, d->ruler->height() - 0.5)
                << QPointF(x+0.5, d->ruler->height() - 0.5);
            painter.drawPolygon(polygon);
            break;
        case QTextOption::RightTab:
            polygon << QPointF(x+0.5, d->ruler->height() - 6.5)
                << QPointF(x-5.5, d->ruler->height() - 0.5)
                << QPointF(x+0.5, d->ruler->height() - 0.5);
            painter.drawPolygon(polygon);
            break;
        case QTextOption::CenterTab:
            polygon << QPointF(x+0.5, d->ruler->height() - 6.5)
                << QPointF(x-5.5, d->ruler->height() - 0.5)
                << QPointF(x+6.5, d->ruler->height() - 0.5);
            painter.drawPolygon(polygon);
            break;
        case QTextOption::DelimiterTab:
            polygon << QPointF(x-5.5, d->ruler->height() - 0.5)
                << QPointF(x+6.5, d->ruler->height() - 0.5);
            painter.drawPolyline(polygon);
            polygon << QPointF(x+0.5, d->ruler->height() - 0.5)
                << QPointF(x+0.5, d->ruler->height() - 6.5);
            painter.drawPolyline(polygon);
            break;
        default:
            break;
        }
    }

    // and also draw the regular interval tab that are non editable
    if (d->tabDistance > 0.0) {
        // first possible position
        position = qMax(position, d->relativeTabs ? 0 : d->paragraphIndent);
        if (position < 0) {
            position = int(position / d->tabDistance) * d->tabDistance;
        } else {
            position = (int(position / d->tabDistance) + 1) * d->tabDistance;
        }
        while (position < d->effectiveActiveRangeEnd() - d->effectiveActiveRangeStart()
                - d->endIndent) {
            qreal x;
            if (d->rightToLeft) {
                x = d->viewConverter->documentToViewX(d->effectiveActiveRangeEnd()
                        - (d->relativeTabs ? d->paragraphIndent : 0) - position) + d->offset;
            } else {
                x = d->viewConverter->documentToViewX(d->effectiveActiveRangeStart()
                        + (d->relativeTabs ? d->paragraphIndent : 0) + position) + d->offset;
            }

            polygon.clear();
            polygon << QPointF(x+0.5, d->ruler->height() - 3.5)
                << QPointF(x+4.5, d->ruler->height() - 0.5)
                << QPointF(x+0.5, d->ruler->height() - 0.5);
            painter.drawPolygon(polygon);

            position += d->tabDistance;
        }
    }
}

void HorizontalPaintingStrategy::drawMeasurements(const KoRulerPrivate *d, QPainter &painter, const QRectF &rectangle)
{
    qreal numberStep = d->numberStepForUnit(); // number step in unit
//    QRectF activeRangeRectangle;
    int numberStepPixel = qRound(d->viewConverter->documentToViewX(d->unit.fromUserValue(numberStep)));
//    const bool adjustMillimeters = (d->unit.type() == KoUnit::Millimeter);

    const QFont font = QFontDatabase::systemFont(QFontDatabase::SmallestReadableFont);
    const QFontMetrics fontMetrics(font);
    painter.setFont(font);

    if (numberStepPixel == 0 || numberStep == 0)
        return;

    // Calc the longest text length
    int textLength = 0;
    for(int i = 0; i < lengthInPixel; i += numberStepPixel) {
        int number = qRound((i / numberStepPixel) * numberStep);

        textLength = qMax(textLength, fontMetrics.width(QString::number(number)));
    }
    textLength += 4;  // Add some padding

    // Change number step so all digits fits
    while(textLength > numberStepPixel) {
        numberStepPixel += numberStepPixel;
        numberStep += numberStep;
    }

    int start=0;
    // Calc the first number step
    if(d->offset < 0)
        start = qAbs(d->offset);

    // make a little hack so rulers shows correctly inversed number aligned
    const qreal lengthInUnit = d->unit.toUserValue(d->rulerLength);
    const qreal hackyLength = lengthInUnit - fmod(lengthInUnit, numberStep);
    if(d->rightToLeft) {
        start -= int(d->viewConverter->documentToViewX(fmod(d->rulerLength,
                    d->unit.fromUserValue(numberStep))));
    }

    int stepCount = (start / numberStepPixel) + 1;
    int halfStepCount = (start / qRound(numberStepPixel * 0.5)) + 1;
    int quarterStepCount = (start / qRound(numberStepPixel * 0.25)) + 1;

    int pos = 0;
    const QPen numberPen(d->ruler->palette().color(QPalette::Text));
    const QPen markerPen(d->ruler->palette().color(QPalette::Inactive, QPalette::Text));
    painter.setPen(markerPen);

    if(d->offset > 0)
        painter.translate(d->offset, 0);

    const int len = qRound(rectangle.width()) + start;
    int nextStep = qRound(d->viewConverter->documentToViewX(
        d->unit.fromUserValue(numberStep * stepCount)));
    int nextHalfStep = qRound(d->viewConverter->documentToViewX(d->unit.fromUserValue(
        numberStep * 0.5 * halfStepCount)));
    int nextQuarterStep = qRound(d->viewConverter->documentToViewX(d->unit.fromUserValue(
        numberStep * 0.25 * quarterStepCount)));

    for(int i = start; i < len; ++i) {
        pos = i - start;

        if(i == nextStep) {
            if(pos != 0)
                painter.drawLine(QPointF(pos, rectangle.bottom()-1),
                                 QPointF(pos, rectangle.bottom() - fullStepMarkerLength));

            int number = qRound(stepCount * numberStep);

            QString numberText = QString::number(number);
            int x = pos;
            if (d->rightToLeft) { // this is done in a hacky way with the fine tuning done above
                numberText = QString::number(hackyLength - stepCount * numberStep);
            }
            painter.setPen(numberPen);
            painter.drawText(QPointF(x-fontMetrics.width(numberText)/2.0,
                                     rectangle.bottom() -fullStepMarkerLength -measurementTextAboveBelowMargin),
                             numberText);
            painter.setPen(markerPen);

            ++stepCount;
            nextStep = qRound(d->viewConverter->documentToViewX(
                d->unit.fromUserValue(numberStep * stepCount)));
            ++halfStepCount;
            nextHalfStep = qRound(d->viewConverter->documentToViewX(d->unit.fromUserValue(
                numberStep * 0.5 * halfStepCount)));
            ++quarterStepCount;
            nextQuarterStep = qRound(d->viewConverter->documentToViewX(d->unit.fromUserValue(
                numberStep * 0.25 * quarterStepCount)));
        }
        else if(i == nextHalfStep) {
            if(pos != 0)
                painter.drawLine(QPointF(pos, rectangle.bottom()-1),
                                 QPointF(pos, rectangle.bottom() - halfStepMarkerLength));

            ++halfStepCount;
            nextHalfStep = qRound(d->viewConverter->documentToViewX(d->unit.fromUserValue(
                numberStep * 0.5 * halfStepCount)));
            ++quarterStepCount;
            nextQuarterStep = qRound(d->viewConverter->documentToViewX(d->unit.fromUserValue(
                numberStep * 0.25 * quarterStepCount)));
        }
        else if(i == nextQuarterStep) {
            if(pos != 0)
                painter.drawLine(QPointF(pos, rectangle.bottom()-1),
                                 QPointF(pos, rectangle.bottom() - quarterStepMarkerLength));

            ++quarterStepCount;
            nextQuarterStep = qRound(d->viewConverter->documentToViewX(d->unit.fromUserValue(
                numberStep * 0.25 * quarterStepCount)));
        }
    }

    // Draw the mouse indicator
    const int mouseCoord = d->mouseCoordinate - start;
    if (d->selected == KoRulerPrivate::None || d->selected == KoRulerPrivate::HotSpot) {
        const qreal top = rectangle.y() + 1;
        const qreal bottom = rectangle.bottom() -1;
        if (d->selected == KoRulerPrivate::None && d->showMousePosition && mouseCoord > 0 && mouseCoord < rectangle.width() )
            painter.drawLine(QPointF(mouseCoord, top), QPointF(mouseCoord, bottom));
        foreach (const KoRulerPrivate::HotSpotData & hp, d->hotspots) {
            const qreal x = d->viewConverter->documentToViewX(hp.position) + d->offset;
            painter.drawLine(QPointF(x, top), QPointF(x, bottom));
        }
    }
}

void HorizontalPaintingStrategy::drawIndents(const KoRulerPrivate *d, QPainter &painter)
{
    QPolygonF polygon;

    painter.setBrush(d->ruler->palette().brush(QPalette::Base));
    painter.setRenderHint( QPainter::Antialiasing );

    qreal x;
    // Draw first line start indent
    if (d->rightToLeft)
        x = d->effectiveActiveRangeEnd() - d->firstLineIndent - d->paragraphIndent;
    else
        x = d->effectiveActiveRangeStart() + d->firstLineIndent + d->paragraphIndent;
    // convert and use the +0.5 to go to nearest integer so that the 0.5 added below ensures sharp lines
    x = int(d->viewConverter->documentToViewX(x) + d->offset + 0.5);
    polygon << QPointF(x+6.5, 0.5)
        << QPointF(x+0.5, 8.5)
        << QPointF(x-5.5, 0.5)
        << QPointF(x+5.5, 0.5);
    painter.drawPolygon(polygon);

    // draw the hanging indent.
    if (d->rightToLeft)
        x = d->effectiveActiveRangeStart() + d->endIndent;
    else
        x = d->effectiveActiveRangeStart() + d->paragraphIndent;
    // convert and use the +0.5 to go to nearest integer so that the 0.5 added below ensures sharp lines
    x = int(d->viewConverter->documentToViewX(x) + d->offset + 0.5);
    const int bottom = d->ruler->height();
    polygon.clear();
    polygon << QPointF(x+6.5, bottom - 0.5)
        << QPointF(x+0.5, bottom - 8.5)
        << QPointF(x-5.5, bottom - 0.5)
        << QPointF(x+5.5, bottom - 0.5);
    painter.drawPolygon(polygon);

    // Draw end-indent or paragraph indent if mode is rightToLeft
    qreal diff;
    if (d->rightToLeft)
        diff = d->viewConverter->documentToViewX(d->effectiveActiveRangeEnd()
                     - d->paragraphIndent) + d->offset - x;
    else
        diff = d->viewConverter->documentToViewX(d->effectiveActiveRangeEnd() - d->endIndent)
                + d->offset - x;
    polygon.translate(diff, 0);
    painter.drawPolygon(polygon);
}

QSize HorizontalPaintingStrategy::sizeHint()
{
    // assumes that digits for the number only use glyphs which do not go below the baseline
    const QFontMetrics fm(QFontDatabase::systemFont(QFontDatabase::SmallestReadableFont));
    const int digitsHeight = fm.ascent() + 1; // +1 for baseline
    const int minimum = digitsHeight + fullStepMarkerLength + 2*measurementTextAboveBelowMargin;

    return QSize(0, minimum);
}

QRectF VerticalPaintingStrategy::drawBackground(const KoRulerPrivate *d, QPainter &painter)
{
    lengthInPixel = d->viewConverter->documentToViewY(d->rulerLength);
    QRectF rectangle;
    rectangle.setX(0);
    rectangle.setY(qMax(0, d->offset));
    rectangle.setWidth(d->ruler->width() - 1.0);
    rectangle.setHeight(qMin(qreal(d->ruler->height() - 1.0 - rectangle.y()),
                             (d->offset >= 0) ? lengthInPixel : lengthInPixel + d->offset));

    QRectF activeRangeRectangle;
    activeRangeRectangle.setX(rectangle.x() + 1);
    activeRangeRectangle.setY(qMax(rectangle.y() + 1,
        d->viewConverter->documentToViewY(d->effectiveActiveRangeStart()) + d->offset));
    activeRangeRectangle.setWidth(rectangle.width() - 2);
    activeRangeRectangle.setBottom(qMin(rectangle.bottom() - 1,
        d->viewConverter->documentToViewY(d->effectiveActiveRangeEnd()) + d->offset));

    painter.setPen(d->ruler->palette().color(QPalette::Mid));
    painter.drawRect(rectangle);

    if(d->effectiveActiveRangeStart() != d->effectiveActiveRangeEnd())
        painter.fillRect(activeRangeRectangle, d->ruler->palette().brush(QPalette::Base));

    if(d->showSelectionBorders) {
        // Draw first selection border
        if(d->firstSelectionBorder > 0) {
            qreal border = d->viewConverter->documentToViewY(d->firstSelectionBorder) + d->offset;
            painter.drawLine(QPointF(rectangle.x() + 1, border), QPointF(rectangle.right() - 1, border));
        }
        // Draw second selection border
        if(d->secondSelectionBorder > 0) {
            qreal border = d->viewConverter->documentToViewY(d->secondSelectionBorder) + d->offset;
            painter.drawLine(QPointF(rectangle.x() + 1, border), QPointF(rectangle.right() - 1, border));
        }
    }

    return rectangle;
}

void VerticalPaintingStrategy::drawMeasurements(const KoRulerPrivate *d, QPainter &painter, const QRectF &rectangle)
{
    qreal numberStep = d->numberStepForUnit(); // number step in unit
    int numberStepPixel = qRound(d->viewConverter->documentToViewY( d->unit.fromUserValue(numberStep)));
    if (numberStepPixel <= 0)
        return;

    const QFont font = QFontDatabase::systemFont(QFontDatabase::SmallestReadableFont);
    const QFontMetrics fontMetrics(font);
    painter.setFont(font);

    // Calc the longest text length
    int textLength = 0;

    for(int i = 0; i < lengthInPixel; i += numberStepPixel) {
        int number = qRound((i / numberStepPixel) * numberStep);
        textLength = qMax(textLength, fontMetrics.width(QString::number(number)));
    }
    textLength += 4;  // Add some padding

    if (numberStepPixel == 0 || numberStep == 0)
        return;
    // Change number step so all digits will fit
    while(textLength > numberStepPixel) {
        numberStepPixel += numberStepPixel;
        numberStep += numberStep;
    }

    // Calc the first number step
    const int start = d->offset < 0 ? qAbs(d->offset) : 0;

    // make a little hack so rulers shows correctly inversed number aligned
    int stepCount = (start / numberStepPixel) + 1;
    int halfStepCount = (start / qRound(numberStepPixel * 0.5)) + 1;
    int quarterStepCount = (start / qRound(numberStepPixel * 0.25)) + 1;

    const QPen numberPen(d->ruler->palette().color(QPalette::Text));
    const QPen markerPen(d->ruler->palette().color(QPalette::Inactive, QPalette::Text));
    painter.setPen(markerPen);

    if(d->offset > 0)
        painter.translate(0, d->offset);

    const int len = qRound(rectangle.height()) + start;
    int nextStep = qRound(d->viewConverter->documentToViewY(
        d->unit.fromUserValue(numberStep * stepCount)));
    int nextHalfStep = qRound(d->viewConverter->documentToViewY(d->unit.fromUserValue(
        numberStep * 0.5 * halfStepCount)));
    int nextQuarterStep = qRound(d->viewConverter->documentToViewY(d->unit.fromUserValue(
        numberStep * 0.25 * quarterStepCount)));

    int pos = 0;
    for(int i = start; i < len; ++i) {
        pos = i - start;

        if(i == nextStep) {
            painter.save();
            painter.translate(rectangle.right()-fullStepMarkerLength, pos);
            if(pos != 0)
                painter.drawLine(QPointF(0, 0), QPointF(fullStepMarkerLength-1, 0));

            painter.rotate(-90);
            int number = qRound(stepCount * numberStep);
            QString numberText = QString::number(number);
            painter.setPen(numberPen);
            painter.drawText(QPointF(-fontMetrics.width(numberText) / 2.0, -measurementTextAboveBelowMargin), numberText);
            painter.restore();

            ++stepCount;
            nextStep = qRound(d->viewConverter->documentToViewY(
                d->unit.fromUserValue(numberStep * stepCount)));
            ++halfStepCount;
            nextHalfStep = qRound(d->viewConverter->documentToViewY(d->unit.fromUserValue(
                numberStep * 0.5 * halfStepCount)));
            ++quarterStepCount;
            nextQuarterStep = qRound(d->viewConverter->documentToViewY(d->unit.fromUserValue(
                numberStep * 0.25 * quarterStepCount)));
        } else if(i == nextHalfStep) {
            if(pos != 0)
                painter.drawLine(QPointF(rectangle.right() - halfStepMarkerLength, pos),
                                 QPointF(rectangle.right() - 1, pos));

            ++halfStepCount;
            nextHalfStep = qRound(d->viewConverter->documentToViewY(d->unit.fromUserValue(
                numberStep * 0.5 * halfStepCount)));
            ++quarterStepCount;
            nextQuarterStep = qRound(d->viewConverter->documentToViewY(d->unit.fromUserValue(
                numberStep * 0.25 * quarterStepCount)));
        } else if(i == nextQuarterStep) {
            if(pos != 0)
                painter.drawLine(QPointF(rectangle.right() - quarterStepMarkerLength, pos),
                                 QPointF(rectangle.right() - 1, pos));

            ++quarterStepCount;
            nextQuarterStep = qRound(d->viewConverter->documentToViewY(d->unit.fromUserValue(
                numberStep * 0.25 * quarterStepCount)));
        }
    }

    // Draw the mouse indicator
    const int mouseCoord = d->mouseCoordinate - start;
    if (d->selected == KoRulerPrivate::None || d->selected == KoRulerPrivate::HotSpot) {
        const qreal left = rectangle.left() + 1;
        const qreal right = rectangle.right() -1;
        if (d->selected == KoRulerPrivate::None && d->showMousePosition && mouseCoord > 0 && mouseCoord < rectangle.height() )
            painter.drawLine(QPointF(left, mouseCoord), QPointF(right, mouseCoord));
        foreach (const KoRulerPrivate::HotSpotData & hp, d->hotspots) {
            const qreal y = d->viewConverter->documentToViewY(hp.position) + d->offset;
            painter.drawLine(QPointF(left, y), QPointF(right, y));
        }
    }
}

QSize VerticalPaintingStrategy::sizeHint()
{
    // assumes that digits for the number only use glyphs which do not go below the baseline
    const QFontMetrics fm(QFontDatabase::systemFont(QFontDatabase::SmallestReadableFont));
    const int digitsHeight = fm.ascent() + 1; // +1 for baseline
    const int minimum = digitsHeight + fullStepMarkerLength + 2*measurementTextAboveBelowMargin;

    return QSize(minimum, 0);
}


void HorizontalDistancesPaintingStrategy::drawDistanceLine(const KoRulerPrivate *d, QPainter &painter, const qreal start, const qreal end)
{

    // Don't draw too short lines
    if (qMax(start, end) - qMin(start, end) < 1)
        return;

    painter.save();
    painter.translate(d->offset, d->ruler->height() / 2);
    painter.setPen(d->ruler->palette().color(QPalette::Text));
    painter.setBrush(d->ruler->palette().color(QPalette::Text));

    QLineF line(QPointF(d->viewConverter->documentToViewX(start), 0),
            QPointF(d->viewConverter->documentToViewX(end), 0));
    QPointF midPoint = line.pointAt(0.5);

    // Draw the label text
    const QFont font = QFontDatabase::systemFont(QFontDatabase::SmallestReadableFont);
    const QFontMetrics fontMetrics(font);
    QString label = d->unit.toUserStringValue(
            d->viewConverter->viewToDocumentX(line.length())) + ' ' + d->unit.symbol();
    QPointF labelPosition = QPointF(midPoint.x() - fontMetrics.width(label)/2,
            midPoint.y() + fontMetrics.ascent()/2);
    painter.setFont(font);
    painter.drawText(labelPosition, label);

    // Draw the arrow lines
    qreal arrowLength = (line.length() - fontMetrics.width(label)) / 2 - 2;
    arrowLength = qMax(qreal(0.0), arrowLength);
    QLineF startArrow(line.p1(), line.pointAt(arrowLength / line.length()));
    QLineF endArrow(line.p2(), line.pointAt(1.0 - arrowLength / line.length()));
    painter.drawLine(startArrow);
    painter.drawLine(endArrow);

    // Draw the arrow heads
    QPolygonF arrowHead;
    arrowHead << line.p1() << QPointF(line.x1()+3, line.y1()-3)
        << QPointF(line.x1()+3, line.y1()+3);
    painter.drawPolygon(arrowHead);
    arrowHead.clear();
    arrowHead << line.p2() << QPointF(line.x2()-3, line.y2()-3)
        << QPointF(line.x2()-3, line.y2()+3);
    painter.drawPolygon(arrowHead);

    painter.restore();
}

void HorizontalDistancesPaintingStrategy::drawMeasurements(const KoRulerPrivate *d, QPainter &painter, const QRectF&)
{
    QList<qreal> points;
    points << 0.0;
    points << d->effectiveActiveRangeStart() + d->paragraphIndent + d->firstLineIndent;
    points << d->effectiveActiveRangeStart() + d->paragraphIndent;
    points << d->effectiveActiveRangeEnd() - d->endIndent;
    points << d->effectiveActiveRangeStart();
    points << d->effectiveActiveRangeEnd();
    points << d->rulerLength;
    qSort(points.begin(), points.end());
    QListIterator<qreal> i(points);
    i.next();
    while (i.hasNext() && i.hasPrevious()) {
        drawDistanceLine(d, painter, i.peekPrevious(), i.peekNext());
        i.next();
    }
}

KoRulerPrivate::KoRulerPrivate(KoRuler *parent, const KoViewConverter *vc, Qt::Orientation o)
    : unit(KoUnit(KoUnit::Point)),
    orientation(o),
    viewConverter(vc),
    offset(0),
    rulerLength(0),
    activeRangeStart(0),
    activeRangeEnd(0),
    activeOverrideRangeStart(0),
    activeOverrideRangeEnd(0),
    mouseCoordinate(-1),
    showMousePosition(0),
    showSelectionBorders(false),
    firstSelectionBorder(0),
    secondSelectionBorder(0),
    showIndents(false),
    firstLineIndent(0),
    paragraphIndent(0),
    endIndent(0),
    showTabs(false),
    relativeTabs(false),
    tabMoved(false),
    originalIndex(-1),
    currentIndex(0),
    rightToLeft(false),
    selected(None),
    selectOffset(0),
    tabChooser(0),
    normalPaintingStrategy(o == Qt::Horizontal ?
            (PaintingStrategy*)new HorizontalPaintingStrategy() : (PaintingStrategy*)new VerticalPaintingStrategy()),
    distancesPaintingStrategy((PaintingStrategy*)new HorizontalDistancesPaintingStrategy()),
    paintingStrategy(normalPaintingStrategy),
    ruler(parent)
{
}

KoRulerPrivate::~KoRulerPrivate()
{
    delete normalPaintingStrategy;
    delete distancesPaintingStrategy;
}

qreal KoRulerPrivate::numberStepForUnit() const
{
    switch(unit.type()) {
        case KoUnit::Inch:
        case KoUnit::Centimeter:
        case KoUnit::Decimeter:
        case KoUnit::Millimeter:
            return 1.0;
        case KoUnit::Pica:
        case KoUnit::Cicero:
            return 10.0;
        case KoUnit::Point:
        default:
            return 100.0;
    }
}

qreal KoRulerPrivate::doSnapping(const qreal value) const
{
    qreal numberStep = unit.fromUserValue(numberStepForUnit()/4.0);
    return numberStep * qRound(value / numberStep);
}

KoRulerPrivate::Selection KoRulerPrivate::selectionAtPosition(const QPoint & pos, int *selectOffset )
{
    const int height = ruler->height();
    if (rightToLeft) {
        int x = int(viewConverter->documentToViewX(effectiveActiveRangeEnd() - firstLineIndent - paragraphIndent) + offset);
        if (pos.x() >= x - 8 && pos.x() <= x +8 && pos.y() < height / 2) {
            if (selectOffset)
                *selectOffset = x - pos.x();
            return KoRulerPrivate::FirstLineIndent;
        }

        x = int(viewConverter->documentToViewX(effectiveActiveRangeEnd() - paragraphIndent) + offset);
        if (pos.x() >= x - 8 && pos.x() <= x +8 && pos.y() > height / 2) {
            if (selectOffset)
                *selectOffset = x - pos.x();
            return KoRulerPrivate::ParagraphIndent;
        }

        x = int(viewConverter->documentToViewX(effectiveActiveRangeStart() + endIndent) + offset);
        if (pos.x() >= x - 8 && pos.x() <= x + 8) {
            if (selectOffset)
                *selectOffset = x - pos.x();
            return KoRulerPrivate::EndIndent;
        }
    }
    else {
        int x = int(viewConverter->documentToViewX(effectiveActiveRangeStart() + firstLineIndent + paragraphIndent) + offset);
        if (pos.x() >= x -8 && pos.x() <= x + 8 && pos.y() < height / 2) {
            if (selectOffset)
                *selectOffset = x - pos.x();
            return KoRulerPrivate::FirstLineIndent;
        }

        x = int(viewConverter->documentToViewX(effectiveActiveRangeStart() + paragraphIndent) + offset);
        if (pos.x() >= x - 8 && pos.x() <= x + 8 && pos.y() > height/2) {
            if (selectOffset)
                *selectOffset = x - pos.x();
            return KoRulerPrivate::ParagraphIndent;
        }

        x = int(viewConverter->documentToViewX(effectiveActiveRangeEnd() - endIndent) + offset);
        if (pos.x() >= x - 8 && pos.x() <= x + 8) {
            if (selectOffset)
                *selectOffset = x - pos.x();
            return KoRulerPrivate::EndIndent;
        }
    }

    return KoRulerPrivate::None;
}

int KoRulerPrivate::hotSpotIndex(const QPoint & pos)
{
    for(int counter = 0; counter < hotspots.count(); counter++) {
        bool hit;
        if (orientation == Qt::Horizontal)
            hit = qAbs(viewConverter->documentToViewX(hotspots[counter].position) - pos.x() + offset) < 3;
        else
            hit = qAbs(viewConverter->documentToViewY(hotspots[counter].position) - pos.y() + offset) < 3;

        if (hit)
            return counter;
    }
    return -1;
}

qreal KoRulerPrivate::effectiveActiveRangeStart() const
{
    if (activeOverrideRangeStart != activeOverrideRangeEnd) {
        return activeOverrideRangeStart;
    } else {
        return activeRangeStart;
    }
}

qreal KoRulerPrivate::effectiveActiveRangeEnd() const
{
    if (activeOverrideRangeStart != activeOverrideRangeEnd) {
        return activeOverrideRangeEnd;
    } else {
        return activeRangeEnd;
    }
}

void KoRulerPrivate::emitTabChanged()
{
    KoRuler::Tab tab;
    if (currentIndex >= 0)
        tab = tabs[currentIndex];
    emit ruler->tabChanged(originalIndex, currentIndex >= 0 ? &tab : 0);
}


KoRuler::KoRuler(QWidget* parent, Qt::Orientation orientation, const KoViewConverter* viewConverter)
  : QWidget(parent)
  , d( new KoRulerPrivate( this, viewConverter, orientation) )
{
    setMouseTracking( true );
}

KoRuler::~KoRuler()
{
    delete d;
}

KoUnit KoRuler::unit() const
{
    return d->unit;
}

void KoRuler::setUnit(const KoUnit &unit)
{
    d->unit = unit;
    update();
}

qreal KoRuler::rulerLength() const
{
    return d->rulerLength;
}

Qt::Orientation KoRuler::orientation() const
{
    return d->orientation;
}

void KoRuler::setOffset(int offset)
{
    d->offset = offset;
    update();
}

void KoRuler::setRulerLength(qreal length)
{
    d->rulerLength = length;
    update();
}

void KoRuler::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    painter.setClipRegion(event->region());
    painter.save();
    QRectF rectangle = d->paintingStrategy->drawBackground(d, painter);
    painter.restore();
    painter.save();
    d->paintingStrategy->drawMeasurements(d, painter, rectangle);
    painter.restore();
    if (d->showIndents) {
        painter.save();
        d->paintingStrategy->drawIndents(d, painter);
        painter.restore();
    }
    d->paintingStrategy->drawTabs(d, painter);
}

QSize KoRuler::minimumSizeHint() const
{
    return d->paintingStrategy->sizeHint();
}

QSize KoRuler::sizeHint() const
{
    return d->paintingStrategy->sizeHint();
}

void KoRuler::setActiveRange(qreal start, qreal end)
{
    d->activeRangeStart = start;
    d->activeRangeEnd = end;
    update();
}

void KoRuler::setOverrideActiveRange(qreal start, qreal end)
{
    d->activeOverrideRangeStart = start;
    d->activeOverrideRangeEnd = end;
    update();
}

void KoRuler::updateMouseCoordinate(int coordinate)
{
    if(d->mouseCoordinate == coordinate)
        return;
    d->mouseCoordinate = coordinate;
    update();
}

void KoRuler::setShowMousePosition(bool show)
{
    d->showMousePosition = show;
    update();
}

void KoRuler::setRightToLeft(bool isRightToLeft)
{
    d->rightToLeft = isRightToLeft;
    update();
}

void KoRuler::setShowIndents(bool show)
{
    d->showIndents = show;
    update();
}

void KoRuler::setFirstLineIndent(qreal indent)
{
    d->firstLineIndent = indent;
    if (d->showIndents) {
        update();
    }
}

void KoRuler::setParagraphIndent(qreal indent)
{
    d->paragraphIndent = indent;
    if (d->showIndents) {
        update();
    }
}

void KoRuler::setEndIndent(qreal indent)
{
    d->endIndent = indent;
    if (d->showIndents) {
        update();
    }
}

qreal KoRuler::firstLineIndent() const
{
    return d->firstLineIndent;
}

qreal KoRuler::paragraphIndent() const
{
    return d->paragraphIndent;
}

qreal KoRuler::endIndent() const
{
    return d->endIndent;
}

QWidget *KoRuler::tabChooser()
{
    if ((d->tabChooser == 0) && (d->orientation == Qt::Horizontal)) {
        d->tabChooser = new RulerTabChooser(parentWidget());
        d->tabChooser->setShowTabs(d->showTabs);
    }

    return d->tabChooser;
}

void KoRuler::setShowSelectionBorders(bool show)
{
    d->showSelectionBorders = show;
    update();
}

void KoRuler::updateSelectionBorders(qreal first, qreal second)
{
    d->firstSelectionBorder = first;
    d->secondSelectionBorder = second;

    if(d->showSelectionBorders)
        update();
}

void KoRuler::setShowTabs(bool show)
{
    if (d->showTabs == show) {
        return;
    }

    d->showTabs = show;
    if (d->tabChooser) {
        d->tabChooser->setShowTabs(show);
    }
    update();
}

void KoRuler::setRelativeTabs(bool relative)
{
    d->relativeTabs = relative;
    if (d->showTabs) {
        update();
    }
}

void KoRuler::updateTabs(const QList<KoRuler::Tab> &tabs, qreal tabDistance)
{
    d->tabs = tabs;
    d->tabDistance = tabDistance;
    if (d->showTabs) {
        update();
    }
}

QList<KoRuler::Tab> KoRuler::tabs() const
{
    QList<Tab> answer = d->tabs;
    qSort(answer.begin(), answer.end(), compareTabs);

    return answer;
}

void KoRuler::setPopupActionList(const QList<QAction*> &popupActionList)
{
    d->popupActions = popupActionList;
}

QList<QAction*> KoRuler::popupActionList() const
{
    return d->popupActions;
}

void KoRuler::mousePressEvent ( QMouseEvent* ev )
{
    d->tabMoved = false;
    d->selected = KoRulerPrivate::None;
    if (ev->button() == Qt::RightButton && !d->popupActions.isEmpty())
        QMenu::exec(d->popupActions, ev->globalPos());
    if (ev->button() != Qt::LeftButton) {
        ev->ignore();
        return;
    }

    QPoint pos = ev->pos();

    if (d->showTabs) {
        int i = 0;
        int x;
        foreach (const Tab & t, d->tabs) {
            if (d->rightToLeft) {
                x = d->viewConverter->documentToViewX(d->effectiveActiveRangeEnd()
                        - (d->relativeTabs ? d->paragraphIndent : 0) - t.position) + d->offset;
            } else {
                x = d->viewConverter->documentToViewX(d->effectiveActiveRangeStart()
                        + (d->relativeTabs ? d->paragraphIndent : 0) + t.position) + d->offset;
            }
            if (pos.x() >= x-6 && pos.x() <= x+6) {
                d->selected = KoRulerPrivate::Tab;
                d->selectOffset = x - pos.x();
                d->currentIndex = i;
                break;
            }
            i++;
        }
        d->originalIndex = d->currentIndex;
    }

    if (d->selected == KoRulerPrivate::None)
        d->selected = d->selectionAtPosition(ev->pos(), &d->selectOffset);
    if (d->selected == KoRulerPrivate::None) {
        int hotSpotIndex = d->hotSpotIndex(ev->pos());
        if (hotSpotIndex >= 0) {
            d->selected = KoRulerPrivate::HotSpot;
            update();
        }
    }

    if (d->showTabs && d->selected == KoRulerPrivate::None) {
        // still haven't found something so let assume the user wants to add a tab
        qreal tabpos;
        if (d->rightToLeft) {
            tabpos = d->viewConverter->viewToDocumentX(pos.x() - d->offset)
                    + d->effectiveActiveRangeEnd() + (d->relativeTabs ? d->paragraphIndent : 0);
        } else {
            tabpos = d->viewConverter->viewToDocumentX(pos.x() - d->offset)
                    - d->effectiveActiveRangeStart() - (d->relativeTabs ? d->paragraphIndent : 0);
        }
        Tab t = {tabpos, d->tabChooser ?  d->tabChooser->type() :
                         d->rightToLeft ? QTextOption::RightTab :
                                          QTextOption::LeftTab};
        d->tabs.append(t);
        d->selectOffset = 0;
        d->selected = KoRulerPrivate::Tab;
        d->currentIndex = d->tabs.count() - 1;
        d->originalIndex = -1; // new!
        update();
    }
    if (d->orientation == Qt::Horizontal && (ev->modifiers() & Qt::ShiftModifier) &&
            (d->selected == KoRulerPrivate::FirstLineIndent ||
             d->selected == KoRulerPrivate::ParagraphIndent ||
             d->selected == KoRulerPrivate::Tab ||
             d->selected == KoRulerPrivate::EndIndent))
        d->paintingStrategy = d->distancesPaintingStrategy;

    if (d->selected != KoRulerPrivate::None)
        emit aboutToChange();
}

void KoRuler::mouseReleaseEvent ( QMouseEvent* ev )
{
    ev->accept();
    if (d->selected == KoRulerPrivate::Tab) {
        if (d->originalIndex >= 0 && !d->tabMoved) {
            int type = d->tabs[d->currentIndex].type;
            type++;
            if (type > 3)
                type = 0;
            d->tabs[d->currentIndex].type = static_cast<QTextOption::TabType> (type);
            update();
        }
        d->emitTabChanged();
    }
    else if( d->selected != KoRulerPrivate::None)
        emit indentsChanged(true);
    else
        ev->ignore();

    d->paintingStrategy = d->normalPaintingStrategy;
    d->selected = KoRulerPrivate::None;
}

void KoRuler::mouseMoveEvent ( QMouseEvent* ev )
{
    QPoint pos = ev->pos();

    qreal activeLength = d->effectiveActiveRangeEnd() - d->effectiveActiveRangeStart();

    switch (d->selected) {
    case KoRulerPrivate::FirstLineIndent:
        if (d->rightToLeft)
            d->firstLineIndent = d->effectiveActiveRangeEnd() - d->paragraphIndent -
                d->viewConverter->viewToDocumentX(pos.x() + d->selectOffset - d->offset);
        else
            d->firstLineIndent = d->viewConverter->viewToDocumentX(pos.x() + d->selectOffset
                - d->offset) - d->effectiveActiveRangeStart() - d->paragraphIndent;
        if( ! (ev->modifiers() & Qt::ShiftModifier)) {
            d->firstLineIndent = d->doSnapping(d->firstLineIndent);
            d->paintingStrategy = d->normalPaintingStrategy;
        } else {
            if (d->orientation == Qt::Horizontal)
                d->paintingStrategy = d->distancesPaintingStrategy;
        }

        emit indentsChanged(false);
        break;
    case KoRulerPrivate::ParagraphIndent:
        if (d->rightToLeft)
            d->paragraphIndent = d->effectiveActiveRangeEnd() -
                d->viewConverter->viewToDocumentX(pos.x() + d->selectOffset - d->offset);
        else
            d->paragraphIndent = d->viewConverter->viewToDocumentX(pos.x() + d->selectOffset
                - d->offset) - d->effectiveActiveRangeStart();
        if( ! (ev->modifiers() & Qt::ShiftModifier)) {
            d->paragraphIndent = d->doSnapping(d->paragraphIndent);
            d->paintingStrategy = d->normalPaintingStrategy;
        } else {
            if (d->orientation == Qt::Horizontal)
                d->paintingStrategy = d->distancesPaintingStrategy;
        }

        if (d->paragraphIndent + d->endIndent > activeLength)
            d->paragraphIndent = activeLength - d->endIndent;
        emit indentsChanged(false);
        break;
    case KoRulerPrivate::EndIndent:
        if (d->rightToLeft)
            d->endIndent = d->viewConverter->viewToDocumentX(pos.x()
                 + d->selectOffset - d->offset) - d->effectiveActiveRangeStart();
        else
            d->endIndent = d->effectiveActiveRangeEnd() - d->viewConverter->viewToDocumentX(pos.x()
                 + d->selectOffset - d->offset);
        if (!(ev->modifiers() & Qt::ShiftModifier)) {
            d->endIndent = d->doSnapping(d->endIndent);
            d->paintingStrategy = d->normalPaintingStrategy;
        } else {
            if (d->orientation == Qt::Horizontal)
                d->paintingStrategy = d->distancesPaintingStrategy;
        }

        if (d->paragraphIndent + d->endIndent > activeLength)
            d->endIndent = activeLength - d->paragraphIndent;
        emit indentsChanged(false);
        break;
    case KoRulerPrivate::Tab:
        d->tabMoved = true;
        if (d->currentIndex < 0) { // tab is deleted.
            if (ev->pos().y() < height()) { // reinstante it.
                d->currentIndex = d->tabs.count();
                d->tabs.append(d->deletedTab);
            } else {
                break;
            }
        }
        if (d->rightToLeft)
            d->tabs[d->currentIndex].position = d->effectiveActiveRangeEnd() -
                d->viewConverter->viewToDocumentX(pos.x() + d->selectOffset - d->offset);
        else
            d->tabs[d->currentIndex].position = d->viewConverter->viewToDocumentX(pos.x() + d->selectOffset
                - d->offset) - d->effectiveActiveRangeStart();
        if (!(ev->modifiers() & Qt::ShiftModifier))
            d->tabs[d->currentIndex].position = d->doSnapping(d->tabs[d->currentIndex].position);
        if (d->tabs[d->currentIndex].position < 0)
            d->tabs[d->currentIndex].position = 0;
        if (d->tabs[d->currentIndex].position > activeLength)
            d->tabs[d->currentIndex].position = activeLength;

        if (ev->pos().y() > height() + OutsideRulerThreshold ) { // moved out of the ruler, delete it.
            d->deletedTab = d->tabs.takeAt(d->currentIndex);
            d->currentIndex = -1;
            // was that a temporary added tab?
            if ( d->originalIndex == -1 )
                emit guideLineCreated(d->orientation,
                        d->orientation == Qt::Horizontal
                        ? d->viewConverter->viewToDocumentY(ev->pos().y())
                        : d->viewConverter->viewToDocumentX(ev->pos().x()));
        }

        d->emitTabChanged();
        break;
    case KoRulerPrivate::HotSpot:
        qreal newPos;
        if (d->orientation == Qt::Horizontal)
            newPos= d->viewConverter->viewToDocumentX(pos.x() - d->offset);
        else
            newPos= d->viewConverter->viewToDocumentY(pos.y() - d->offset);
        d->hotspots[d->currentIndex].position = newPos;
        emit hotSpotChanged(d->hotspots[d->currentIndex].id, newPos);
        break;
    case KoRulerPrivate::None:
        d->mouseCoordinate = (d->orientation == Qt::Horizontal ?  pos.x() : pos.y()) - d->offset;
        int hotSpotIndex = d->hotSpotIndex(pos);
        if (hotSpotIndex >= 0) {
            setCursor(QCursor( d->orientation == Qt::Horizontal ? Qt::SplitHCursor : Qt::SplitVCursor ));
            break;
        }
        unsetCursor();

        KoRulerPrivate::Selection selection = d->selectionAtPosition(pos);
        QString text;
        switch(selection) {
        case KoRulerPrivate::FirstLineIndent: text = i18n("First line indent"); break;
        case KoRulerPrivate::ParagraphIndent: text = i18n("Left indent"); break;
        case KoRulerPrivate::EndIndent: text = i18n("Right indent"); break;
        case KoRulerPrivate::None:
            if (ev->buttons() & Qt::LeftButton) {
                if (d->orientation == Qt::Horizontal && ev->pos().y() > height() + OutsideRulerThreshold)
                    emit guideLineCreated(d->orientation, d->viewConverter->viewToDocumentY(ev->pos().y()));
                else if (d->orientation == Qt::Vertical && ev->pos().x() > width() + OutsideRulerThreshold)
                    emit guideLineCreated(d->orientation, d->viewConverter->viewToDocumentX(ev->pos().x()));
            }
            break;
        default:
            break;
        }
        setToolTip(text);
    }
    update();
}

void KoRuler::clearHotSpots()
{
    if (d->hotspots.isEmpty())
        return;
    d->hotspots.clear();
    update();
}

void KoRuler::setHotSpot(qreal position, int id)
{
    uint hotspotCount = d->hotspots.count();
    for( uint i = 0; i < hotspotCount; ++i ) {
        KoRulerPrivate::HotSpotData & hs = d->hotspots[i];
        if (hs.id == id) {
            hs.position = position;
            update();
            return;
        }
    }
    // not there yet, then insert it.
    KoRulerPrivate::HotSpotData hs;
    hs.position = position;
    hs.id = id;
    d->hotspots.append(hs);
}

bool KoRuler::removeHotSpot(int id)
{
    QList<KoRulerPrivate::HotSpotData>::Iterator iter = d->hotspots.begin();
    while(iter != d->hotspots.end()) {
        if (iter->id == id) {
            d->hotspots.erase(iter);
            update();
            return true;
        }
    }
    return false;
}

void KoRuler::createGuideToolConnection(KoCanvasBase *canvas)
{
    Q_ASSERT(canvas);
    KoToolBase *tool = KoToolManager::instance()->toolById(canvas, QLatin1String("GuidesTool"));
    if (!tool) return; // It's perfectly fine to have no guides tool, we don't have to warn the user about it
    connect(this, SIGNAL(guideLineCreated(Qt::Orientation,qreal)),
        tool, SLOT(createGuideLine(Qt::Orientation,qreal)));
}
