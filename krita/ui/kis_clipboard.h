/*
 *  kis_clipboard.h - part of Krayon
 *
 *  Copyright (c) 2004 Boudewijn Rempt <boud@valdyas.org>
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
#ifndef __KIS_CLIPBOARD_H_
#define __KIS_CLIPBOARD_H_

#include <QObject>
#include <QSize>
#include "kis_types.h"
#include <kritaui_export.h>

class QRect;

enum enumPasteBehaviour {
    PASTE_ASSUME_WEB,
    PASTE_ASSUME_MONITOR,
    PASTE_ASK
};

/**
 * The Krita clipboard is a clipboard that can store paint devices
 * instead of just qimage's.
 */
class KRITAUI_EXPORT KisClipboard : public QObject
{

    Q_OBJECT
    Q_PROPERTY(bool clip READ hasClip NOTIFY clipChanged)

public:
    KisClipboard();
    virtual ~KisClipboard();

    static KisClipboard* instance();

    /**
     * Sets the clipboard to the contents of the specified paint device; also
     * set the system clipboard to a QImage representation of the specified
     * paint device.
     *
     * @param dev The paint device that will be stored on the clipboard
     * @param topLeft a hint about the place where the clip should be pasted by default
     */
    void setClip(KisPaintDeviceSP dev, const QPoint& topLeft);

    /**
     * Get the contents of the clipboard in the form of a paint device.
     */
    KisPaintDeviceSP clip(const QRect &imageBounds, bool showPopup);

    bool hasClip() const;

    QSize clipSize() const;

Q_SIGNALS:

    void clipCreated();


private Q_SLOTS:

    void clipboardDataChanged();

private:

    KisClipboard(const KisClipboard &);
    KisClipboard operator=(const KisClipboard &);

    bool m_hasClip;

    bool m_pushedClipboard;

Q_SIGNALS:
    void clipChanged();
};

#endif // __KIS_CLIPBOARD_H_
