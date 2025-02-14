/* This file is part of the KDE project
   Copyright (C) 2000 Werner Trobin <trobin@kde.org>

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

#include "KisTemplate.h"

#include <QImage>
#include <QPixmap>
#include <QIcon>
#include <QFile>
#include <kis_debug.h>
#include <KoResourcePaths.h>
#include <kis_icon.h>


KisTemplate::KisTemplate(const QString &name, const QString &description, const QString &file,
                       const QString &picture, const QString &fileName, const QString &_measureSystem,
                       bool hidden, bool touched) :
        m_name(name), m_descr(description), m_file(file), m_picture(picture), m_fileName(fileName),
        m_hidden(hidden), m_touched(touched), m_cached(false), m_measureSystem(_measureSystem)
{
}

const QPixmap &KisTemplate::loadPicture()
{
    if (m_cached)
        return m_pixmap;
    m_cached = true;
    if (m_picture[ 0 ] == '/') {
        QImage img(m_picture);
        if (img.isNull()) {
            dbgKrita << "Couldn't find icon " << m_picture;
            m_pixmap = QPixmap();
            return m_pixmap;
        }
        const int maxHeightWidth = 128; // ### TODO: some people would surely like to have 128x128
        if (img.width() > maxHeightWidth || img.height() > maxHeightWidth) {
            img = img.scaled(maxHeightWidth, maxHeightWidth, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }
        m_pixmap = QPixmap::fromImage(img);
        return m_pixmap;
    } else { // relative path
        QString filename = KoResourcePaths::findResource("kis_pics", m_picture + ".png");
        m_pixmap = QPixmap(filename);
        return m_pixmap;
    }
}

