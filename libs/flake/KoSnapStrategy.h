/* This file is part of the KDE project
 * Copyright (C) 2008-2009 Jan Hambrecht <jaham@gmx.net>
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

#ifndef KOSNAPSTRATEGY_H
#define KOSNAPSTRATEGY_H

#include "KoSnapGuide.h"

#include <QLineF>

class TestSnapStrategy;
class KoPathPoint;
class KoSnapProxy;
class KoViewConverter;

class QTransform;
class QPainterPath;

class KRITAFLAKE_EXPORT KoSnapStrategy
{
public:
    KoSnapStrategy(KoSnapGuide::Strategy type);
    virtual ~KoSnapStrategy() {};

    virtual bool snap(const QPointF &mousePosition, KoSnapProxy * proxy, qreal maxSnapDistance) = 0;

    /// returns the strategies type
    KoSnapGuide::Strategy type() const;

    static qreal squareDistance(const QPointF &p1, const QPointF &p2);
    static qreal scalarProduct(const QPointF &p1, const QPointF &p2);

    /// returns the snapped position form the last call to snapToPoints
    QPointF snappedPosition() const;

    /// returns the current snap strategy decoration
    virtual QPainterPath decoration(const KoViewConverter &converter) const = 0;

protected:
    /// sets the current snapped position
    void setSnappedPosition(const QPointF &position);

private:
    KoSnapGuide::Strategy m_snapType;
    QPointF m_snappedPosition;
};

/// snaps to x- or y-coordinates of path points
class KRITAFLAKE_EXPORT OrthogonalSnapStrategy : public KoSnapStrategy
{
public:
    OrthogonalSnapStrategy();
    virtual bool snap(const QPointF &mousePosition, KoSnapProxy * proxy, qreal maxSnapDistance);
    virtual QPainterPath decoration(const KoViewConverter &converter) const;
private:
    QLineF m_hLine;
    QLineF m_vLine;
};

/// snaps to path points
class KRITAFLAKE_EXPORT NodeSnapStrategy : public KoSnapStrategy
{
public:
    NodeSnapStrategy();
    virtual bool snap(const QPointF &mousePosition, KoSnapProxy * proxy, qreal maxSnapDistance);
    virtual QPainterPath decoration(const KoViewConverter &converter) const;
};

/// snaps extension lines of path shapes
class KRITAFLAKE_EXPORT ExtensionSnapStrategy : public KoSnapStrategy
{
    friend class TestSnapStrategy;
public:
    ExtensionSnapStrategy();
    virtual bool snap(const QPointF &mousePosition, KoSnapProxy * proxy, qreal maxSnapDistance);
    virtual QPainterPath decoration(const KoViewConverter &converter) const;
private:
    qreal project(const QPointF &lineStart , const QPointF &lineEnd, const QPointF &point);
    QPointF extensionDirection(KoPathPoint * point, const QTransform &matrix);
    bool snapToExtension(QPointF &position, KoPathPoint * point, const QTransform &matrix);
    QList<QLineF> m_lines;
};

/// snaps to intersections of shapes
class KRITAFLAKE_EXPORT IntersectionSnapStrategy : public KoSnapStrategy
{
public:
    IntersectionSnapStrategy();
    virtual bool snap(const QPointF &mousePosition, KoSnapProxy * proxy, qreal maxSnapDistance);
    virtual QPainterPath decoration(const KoViewConverter &converter) const;
};

/// snaps to the canvas grid
class KRITAFLAKE_EXPORT GridSnapStrategy : public KoSnapStrategy
{
public:
    GridSnapStrategy();
    virtual bool snap(const QPointF &mousePosition, KoSnapProxy * proxy, qreal maxSnapDistance);
    virtual QPainterPath decoration(const KoViewConverter &converter) const;
};

/// snaps to shape bounding boxes
class KRITAFLAKE_EXPORT BoundingBoxSnapStrategy : public KoSnapStrategy
{
    friend class TestSnapStrategy;
public:
    BoundingBoxSnapStrategy();
    virtual bool snap(const QPointF &mousePosition, KoSnapProxy * proxy, qreal maxSnapDistance);
    virtual QPainterPath decoration(const KoViewConverter &converter) const;
private:
    qreal squareDistanceToLine(const QPointF &lineA, const QPointF &lineB, const QPointF &point, QPointF &pointOnLine);
    QPointF m_boxPoints[5];
};

/// snaps to line guides
class KRITAFLAKE_EXPORT LineGuideSnapStrategy : public KoSnapStrategy
{
public:
    LineGuideSnapStrategy();
    virtual bool snap(const QPointF &mousePosition, KoSnapProxy * proxy, qreal maxSnapDistance);
    virtual QPainterPath decoration(const KoViewConverter &converter) const;
private:
    int m_orientation;
};

#endif // KOSNAPSTRATEGY_H
