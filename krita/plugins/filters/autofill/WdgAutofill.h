/*
 * This file is part of Krita
 *
 * Copyright (c) 2015 Boudewijn Rempt <boud@valdyas.org>
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

#ifndef KIS_WDG_AUTOFILL_H
#define KIS_WDG_AUTOFILL_H

#include <kis_config_widget.h>

class Ui_WdgAutofillOptions;
class KisFilter;
class KoColorSet;
class KisColorsetChooser;

class KisWdgAutofill : public KisConfigWidget
{
    Q_OBJECT
public:
    KisWdgAutofill(KisFilter* nfilter, QWidget* parent = 0);
    ~KisWdgAutofill();
public:
    inline const Ui_WdgAutofillOptions* widget() const {
        return m_widget;
    }
    virtual void setConfiguration(const KisPropertiesConfiguration*);
    virtual KisPropertiesConfiguration* configuration() const;

private Q_SLOTS:

    void randomSelected(bool selected);
    void setColorSet(KoColorSet *colorSet);
private:
    Ui_WdgAutofillOptions* m_widget;
    KoColorSet *m_colorSet;
    KisColorsetChooser *m_colorsetChooser;
};

#endif

