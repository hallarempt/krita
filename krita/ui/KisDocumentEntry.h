/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>
   Copyright     2007       David Faure <faure@kde.org>

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
 * Boston, MA 02110-1301, USA.
*/

#ifndef KIS_DOCUMENT_ENTRY_H
#define KIS_DOCUMENT_ENTRY_H

#include <QList>
#include <QString>
#include "kritaui_export.h"

class QStringList;
class QPluginLoader;

/**
 *  Represents an available Calligra component
 *  that supports the document interface.
 */
class KRITAUI_EXPORT KisDocumentEntry
{

public:
    /**
     * Represents an invalid entry (as returned by queryByMimeType for instance)
     */
    explicit KisDocumentEntry();
    /**
     * Represents a valid entry
     */
    explicit KisDocumentEntry(QPluginLoader *loader);
    ~KisDocumentEntry();

    static QString nativeMimeType();

    static QStringList extraNativeMimeTypes();

    QPluginLoader *loader() const;

    /**
     * @return TRUE if the service pointer is null
     */
    bool isEmpty() const;

    /**
     * @return name of the associated service
     */
    QString name() const;

    /**
     *  Mimetypes (and other service types) which this document can handle.
     */
    QStringList mimeTypes() const;

    /**
     *  @return TRUE if the document can handle the requested mimetype.
     */
    bool supportsMimeType(const QString & _mimetype) const;

        /**
     *  This function will query ksycoca to find all available components.
     *  The result will only contain parts, which are embeddable into a document
     *
     *  @param _constr is a constraint expression as used by KTrader.
     *                 You can use it to set additional restrictions on the available
     *                 components.
     */
    static QList<KisDocumentEntry> query(const QString &  mimetype = QString());

    /**
     *  This is a convenience function.
     *
     *  @return a document entry for the Calligra component that supports
     *          the requested mimetype and fits the user best.
     */
    static KisDocumentEntry queryByMimeType(const QString & mimetype);

private:
    QPluginLoader *m_loader;
};

#endif
