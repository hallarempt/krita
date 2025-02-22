/* This file is part of the KDE project
 * Copyright (C) Boudewijn Rempt <boud@valdyas.org>, (C) 2008
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

#ifndef KIS_PAINTOP_OPTION_H
#define KIS_PAINTOP_OPTION_H

#include <kis_types.h>
#include <kritaui_export.h>
#include <kis_properties_configuration.h>
#include <kis_locked_properties_proxy.h>

class QWidget;
class QString;
class KisPaintopLodLimitations;


/**
 * Base interface for paintop options. A paintop option
 * can be enabled/disabled, has a configuration page
 * (for example, a curve), a user-visible name and can
 * be serialized and deserialized into KisPaintOpPresets
 *
 * Options are disabled by default.
 */
class KRITAUI_EXPORT KisPaintOpOption : public QObject
{
    Q_OBJECT
public:

    enum PaintopCategory {
        GENERAL,
        COLOR,
        TEXTURE,
        FILTER
    };

    KisPaintOpOption(KisPaintOpOption::PaintopCategory category, bool checked);
    virtual ~KisPaintOpOption();

    KisPaintOpOption::PaintopCategory category() const;
    virtual bool isCheckable() const;

    virtual bool isChecked() const;
    virtual void setChecked(bool checked);

    void setLocked(bool value);
    bool isLocked() const;

    /**
     * Reimplement this to use the image in the option widget
     */
    virtual void setImage(KisImageWSP image);

    virtual void setNode(KisNodeWSP node);

    void startReadOptionSetting(const KisPropertiesConfiguration* setting);
    void startWriteOptionSetting(KisPropertiesConfiguration* setting) const;

    QWidget* configurationPage() const;

    virtual void lodLimitations(KisPaintopLodLimitations *l) const;

protected:

    void setConfigurationPage(QWidget * page);

protected:
    /**
     * Re-implement this to save the configuration to the paint configuration.
     */
    virtual void writeOptionSetting(KisPropertiesConfiguration* setting) const {
        Q_UNUSED(setting);
    }

    /**
     * Re-implement this to set te widgets with the values in @param setting.
     */
    virtual void readOptionSetting(const KisPropertiesConfiguration* setting) {
        Q_UNUSED(setting);
    }

protected Q_SLOTS:
    void emitSettingChanged();

Q_SIGNALS:

    /**
     * emit this whenever a setting has changed. It will update the preview
     */
    void sigSettingChanged();

protected:

    bool m_checkable;
    bool m_locked;

private:

    struct Private;
    Private* const m_d;
};

#endif
