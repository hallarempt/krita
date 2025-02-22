/* This file is part of the KDE project
 * Copyright (C) 2006-2009 Thomas Zander <zander@kde.org>
 * Copyright (C) 2008 Thorsten Zachmann <zachmann@kde.org>
 * Copyright (C) 2008 Roopesh Chander <roop@forwardbias.in>
 * Copyright (C) 2008 Girish Ramakrishnan <girish@forwardbias.in>
 * Copyright (C) 2009 KO GmbH <cbo@kogmbh.com>
 * Copyright (C) 2011 Pierre Ducroquet <pinaraf@pinaraf.info>
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
#include "KoTableRowStyle.h"

#include <KoGenStyle.h>
#include "Styles_p.h"

#include <KoUnit.h>
#include <KoStyleStack.h>
#include <KoOdfLoadingContext.h>
#include <KoXmlNS.h>

#include "TextDebug.h"


class Q_DECL_HIDDEN KoTableRowStyle::Private : public QSharedData
{
public:
    Private() : QSharedData(), parentStyle(0), next(0) {}

    ~Private() {
    }

    void setProperty(int key, const QVariant &value) {
        stylesPrivate.add(key, value);
    }

    QString name;
    KoTableRowStyle *parentStyle;
    int next;
    StylePrivate stylesPrivate;
};

KoTableRowStyle::KoTableRowStyle()
    :  d(new Private())
{
}

KoTableRowStyle::KoTableRowStyle(const KoTableRowStyle &rhs)
        : d(rhs.d)
{
}

KoTableRowStyle &KoTableRowStyle::operator=(const KoTableRowStyle &rhs)
{
    d = rhs.d;
    return *this;
}

KoTableRowStyle::~KoTableRowStyle()
{
}

void KoTableRowStyle::copyProperties(const KoTableRowStyle *style)
{
    d->stylesPrivate = style->d->stylesPrivate;
    setName(style->name()); // make sure we emit property change
    d->parentStyle = style->d->parentStyle;
}

KoTableRowStyle *KoTableRowStyle::clone() const
{
    KoTableRowStyle *newStyle = new KoTableRowStyle();
    newStyle->copyProperties(this);
    return newStyle;
}

void KoTableRowStyle::setParentStyle(KoTableRowStyle *parent)
{
    d->parentStyle = parent;
}

void KoTableRowStyle::setProperty(int key, const QVariant &value)
{
    if (d->parentStyle) {
        QVariant var = d->parentStyle->value(key);
        if (!var.isNull() && var == value) { // same as parent, so its actually a reset.
            d->stylesPrivate.remove(key);
            return;
        }
    }
    d->stylesPrivate.add(key, value);
}

void KoTableRowStyle::remove(int key)
{
    d->stylesPrivate.remove(key);
}

QVariant KoTableRowStyle::value(int key) const
{
    QVariant var = d->stylesPrivate.value(key);
    if (var.isNull() && d->parentStyle)
        return d->parentStyle->value(key);
    return var;
}

bool KoTableRowStyle::hasProperty(int key) const
{
    return d->stylesPrivate.contains(key);
}

qreal KoTableRowStyle::propertyDouble(int key) const
{
    QVariant variant = value(key);
    if (variant.isNull())
        return 0.0;
    return variant.toDouble();
}

int KoTableRowStyle::propertyInt(int key) const
{
    QVariant variant = value(key);
    if (variant.isNull())
        return 0;
    return variant.toInt();
}

bool KoTableRowStyle::propertyBoolean(int key) const
{
    QVariant variant = value(key);
    if (variant.isNull())
        return false;
    return variant.toBool();
}

QColor KoTableRowStyle::propertyColor(int key) const
{
    QVariant variant = value(key);
    if (variant.isNull()) {
        return QColor();
    }
    return qvariant_cast<QColor>(variant);
}

void KoTableRowStyle::setBackground(const QBrush &brush)
{
    d->setProperty(QTextFormat::BackgroundBrush, brush);
}

void KoTableRowStyle::clearBackground()
{
    d->stylesPrivate.remove(QTextFormat::BackgroundBrush);
}

QBrush KoTableRowStyle::background() const
{
    QVariant variant = d->stylesPrivate.value(QTextFormat::BackgroundBrush);

    if (variant.isNull()) {
        return QBrush();
    }
    return qvariant_cast<QBrush>(variant);
}

void KoTableRowStyle::setBreakBefore(KoText::KoTextBreakProperty state)
{
    setProperty(BreakBefore, state);
}

KoText::KoTextBreakProperty KoTableRowStyle::breakBefore() const
{
    return (KoText::KoTextBreakProperty) propertyInt(BreakBefore);
}

void KoTableRowStyle::setBreakAfter(KoText::KoTextBreakProperty state)
{
    setProperty(BreakAfter, state);
}

KoText::KoTextBreakProperty KoTableRowStyle::breakAfter() const
{
    return (KoText::KoTextBreakProperty) propertyInt(BreakAfter);
}

void KoTableRowStyle::setUseOptimalHeight(bool on)
{
    setProperty(UseOptimalHeight, on);
}

bool KoTableRowStyle::useOptimalHeight() const
{
    return propertyBoolean(UseOptimalHeight);
}

void KoTableRowStyle::setMinimumRowHeight(const qreal height)
{
    setProperty(MinimumRowHeight, height);
}


qreal KoTableRowStyle::minimumRowHeight() const
{
    return propertyDouble(MinimumRowHeight);
}

void KoTableRowStyle::setRowHeight(qreal height)
{
    if(height <= 0)
        d->stylesPrivate.remove(RowHeight);
    else
        setProperty(RowHeight, height);
}

qreal KoTableRowStyle::rowHeight() const
{
    return propertyDouble(RowHeight);
}

void KoTableRowStyle::setKeepTogether(bool on)
{
    setProperty(KeepTogether, on);
}

bool KoTableRowStyle::keepTogether() const
{
    return propertyBoolean(KeepTogether);
}

KoTableRowStyle *KoTableRowStyle::parentStyle() const
{
    return d->parentStyle;
}

QString KoTableRowStyle::name() const
{
    return d->name;
}

void KoTableRowStyle::setName(const QString &name)
{
    if (name == d->name)
        return;
    d->name = name;
}

int KoTableRowStyle::styleId() const
{
    return propertyInt(StyleId);
}

void KoTableRowStyle::setStyleId(int id)
{
    setProperty(StyleId, id); if (d->next == 0) d->next = id;
}

QString KoTableRowStyle::masterPageName() const
{
    return value(MasterPageName).toString();
}

void KoTableRowStyle::setMasterPageName(const QString &name)
{
    setProperty(MasterPageName, name);
}

void KoTableRowStyle::loadOdf(const KoXmlElement *element, KoOdfLoadingContext &context)
{
    if (element->hasAttributeNS(KoXmlNS::style, "display-name"))
        d->name = element->attributeNS(KoXmlNS::style, "display-name", QString());

    if (d->name.isEmpty()) // if no style:display-name is given us the style:name
        d->name = element->attributeNS(KoXmlNS::style, "name", QString());

    QString masterPage = element->attributeNS(KoXmlNS::style, "master-page-name", QString());
    if (! masterPage.isEmpty()) {
        setMasterPageName(masterPage);
    }
    context.styleStack().save();
    QString family = element->attributeNS(KoXmlNS::style, "family", "table-row");
    context.addStyles(element, family.toLocal8Bit().constData());   // Load all parents - only because we don't support inheritance.

    context.styleStack().setTypeProperties("table-row");   // load all style attributes from "style:table-column-properties"
    loadOdfProperties(context.styleStack());   // load the KoTableRowStyle from the stylestack
    context.styleStack().restore();
}


void KoTableRowStyle::loadOdfProperties(KoStyleStack &styleStack)
{
    // The fo:background-color attribute specifies the background color of a cell.
    if (styleStack.hasProperty(KoXmlNS::fo, "background-color")) {
        const QString bgcolor = styleStack.property(KoXmlNS::fo, "background-color");
        QBrush brush = background();
        if (bgcolor == "transparent")
           setBackground(Qt::NoBrush);
        else {
            if (brush.style() == Qt::NoBrush)
                brush.setStyle(Qt::SolidPattern);
            brush.setColor(bgcolor); // #rrggbb format
            setBackground(brush);
        }
    }

    // minimum row height
    if (styleStack.hasProperty(KoXmlNS::style, "min-row-height")) {
        setMinimumRowHeight(KoUnit::parseValue(styleStack.property(KoXmlNS::style, "min-row-height")));
    }

    // optimal row height
    if (styleStack.hasProperty(KoXmlNS::style, "use-optimal-row-height")) {
        setUseOptimalHeight(styleStack.property(KoXmlNS::style, "use-optimal-row-height") == "true");
    }

    // row height
    if (styleStack.hasProperty(KoXmlNS::style, "row-height")) {
        setRowHeight(KoUnit::parseValue(styleStack.property(KoXmlNS::style, "row-height")));
    }

    // The fo:keep-together specifies if a row is allowed to break in the middle of the row.
    if (styleStack.hasProperty(KoXmlNS::fo, "keep-together")) {
        setKeepTogether(styleStack.property(KoXmlNS::fo, "keep-together") != "auto");
    }

    // The fo:break-before and fo:break-after attributes insert a page or column break before or after a column.
    if (styleStack.hasProperty(KoXmlNS::fo, "break-before")) {
        setBreakBefore(KoText::textBreakFromString(styleStack.property(KoXmlNS::fo, "break-before")));
    }
    if (styleStack.hasProperty(KoXmlNS::fo, "break-after")) {
        setBreakAfter(KoText::textBreakFromString(styleStack.property(KoXmlNS::fo, "break-after")));
    }
}

bool KoTableRowStyle::operator==(const KoTableRowStyle &other) const
{
    return other.d == d;
}

void KoTableRowStyle::removeDuplicates(const KoTableRowStyle &other)
{
    d->stylesPrivate.removeDuplicates(other.d->stylesPrivate);
}

bool KoTableRowStyle::isEmpty() const
{
    return d->stylesPrivate.isEmpty();
}

void KoTableRowStyle::saveOdf(KoGenStyle &style) const
{
    QList<int> keys = d->stylesPrivate.keys();
    Q_FOREACH (int key, keys) {
        if (key == QTextFormat::BackgroundBrush) {
            QBrush backBrush = background();
            if (backBrush.style() != Qt::NoBrush)
                style.addProperty("fo:background-color", backBrush.color().name(), KoGenStyle::TableRowType);
            else
                style.addProperty("fo:background-color", "transparent", KoGenStyle::TableRowType);
        } else if (key == MinimumRowHeight) {
            style.addPropertyPt("style:min-row-height", minimumRowHeight(), KoGenStyle::TableRowType);
        } else if (key == RowHeight) {
            style.addPropertyPt("style:row-height", rowHeight(), KoGenStyle::TableRowType);
        } else if (key == UseOptimalHeight) {
            style.addProperty("style:use-optimal-row-height", useOptimalHeight(), KoGenStyle::TableRowType);
        } else if (key == BreakBefore) {
            style.addProperty("fo:break-before", KoText::textBreakToString(breakBefore()), KoGenStyle::TableRowType);
        } else if (key == BreakAfter) {
            style.addProperty("fo:break-after", KoText::textBreakToString(breakAfter()), KoGenStyle::TableRowType);
        } else if (key == KeepTogether) {
            if (keepTogether())
                style.addProperty("fo:keep-together", "always", KoGenStyle::TableRowType);
            else
                style.addProperty("fo:keep-together", "auto", KoGenStyle::TableRowType);
        }
    }
}
