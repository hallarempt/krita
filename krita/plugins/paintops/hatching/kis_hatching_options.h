/*
 *  Copyright (c) 2008 Lukas Tvrdy <lukast.dev@gmail.com>
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

#ifndef KIS_HATCHING_OPTIONS_H
#define KIS_HATCHING_OPTIONS_H

#include <kis_paintop_option.h>

class KisPaintopLodLimitations;
class KisHatchingOptionsWidget;

class KisHatchingOptions : public KisPaintOpOption
{

public:
    KisHatchingOptions();
    ~KisHatchingOptions();

    void writeOptionSetting(KisPropertiesConfiguration* setting) const;
    void readOptionSetting(const KisPropertiesConfiguration* setting);
    void lodLimitations(KisPaintopLodLimitations *l) const;

private:
    KisHatchingOptionsWidget * m_options;

};

#endif
