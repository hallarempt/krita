/*
 *  Copyright (c) 2015 Dmitry Kazakov <dimula73@gmail.com>
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

#ifndef KIS_URL_REQUESTER_H
#define KIS_URL_REQUESTER_H

#include "kritawidgets_export.h"

#include <QWidget>
#include <QString>
#include <QUrl>
#include <KoFileDialog.h>


namespace Ui {
    class WdgUrlRequester;
}

/**
 * This represents an editable file name.
 * Visual it presents a QLineEdit + a buton that pops up
 * a file chooser.
 *
 * Signals are fired when the user changes the text
 * or selects a new file via the button/file chooser.
 */
class KRITAWIDGETS_EXPORT KisUrlRequester : public QWidget
{
    Q_OBJECT

public:
    explicit KisUrlRequester(QWidget *parent = 0);
    ~KisUrlRequester();

    void setStartDir(const QString &path);

    QUrl url() const;
    void setUrl(const QUrl &url);

    void setMode(KoFileDialog::DialogType mode);
    KoFileDialog::DialogType mode() const;

    /**
     * Sets the mime type filters to use, same format as KoFileDialog::setMimeTypeFilters.
     * If this is not called, the default list is used, which simply selects all the image
     * file formats Krita can load.
     */
    void setMimeTypeFilters(const QStringList &filterList,
                            QString defaultFilter = QString());

    /**
     * Sets the name filter, same as KoFileDialog::setNameFilter
     */
    void setNameFilter(const QString &filter);

public Q_SLOTS:
    void slotSelectFile();

Q_SIGNALS:
    void textChanged(const QString &fileName);
    void urlSelected(const QUrl &url);

private:
    QString fileName() const;
    void setFileName(const QString &path);

private:
    QScopedPointer<Ui::WdgUrlRequester> m_ui;
    QString m_basePath;
    KoFileDialog::DialogType m_mode;
    QStringList m_mime_filter_list;
    QString m_mime_default_filter;
    QString m_nameFilter;
};

#endif // KIS_URL_REQUESTER_H
