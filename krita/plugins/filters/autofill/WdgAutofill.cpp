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
#include "WdgAutofill.h"

#include <QLayout>

#include <KoColorSet.h>
#include <KoResourceServerProvider.h>

#include <filter/kis_filter_configuration.h>
#include <KisColorsetChooser.h>
#include <kis_icon_utils.h>
#include "ui_WdgAutofillOptions.h"

KisWdgAutofill::KisWdgAutofill(KisFilter* /*nfilter*/, QWidget* parent)
    : KisConfigWidget(parent)
{
    m_widget = new Ui_WdgAutofillOptions();
    m_widget->setupUi(this);

    m_colorsetChooser = new KisColorsetChooser(this);
    connect(m_colorsetChooser, SIGNAL(paletteSelected(KoColorSet*)), this, SLOT(setColorSet(KoColorSet*)));

    m_widget->bnColorSets->setIcon(KisIconUtils::loadIcon("hi16-palette_library"));
    m_widget->bnColorSets->setToolTip(i18n("Choose palette"));
    m_widget->bnColorSets->setPopupWidget(m_colorsetChooser);

    connect(m_widget->chkUseRandomColors, SIGNAL(toggled(bool)), this, SLOT(randomSelected(bool)));
    connect(m_widget->bnAutofillColor, SIGNAL(changed(QColor)), SIGNAL(sigConfigurationItemChanged()));
    connect(m_widget->bnFlagColor, SIGNAL(changed(QColor)), SIGNAL(sigConfigurationItemChanged()));
    connect(m_widget->intColorThreshold, SIGNAL(valueChanged(int)), SIGNAL(sigConfigurationItemChanged()));
    connect(m_widget->intFlagAreas, SIGNAL(valueChanged(int)), SIGNAL(sigConfigurationItemChanged()));
    connect(m_widget->intIgnoreAreas, SIGNAL(valueChanged(int)), SIGNAL(sigConfigurationItemChanged()));
}

KisWdgAutofill::~KisWdgAutofill()
{
    delete m_widget;
}

void KisWdgAutofill::setConfiguration(const KisPropertiesConfiguration* config)
{
    QVariant value;
    if (config->getProperty("autofillcolor", value)) {
        m_widget->bnAutofillColor->setColor(value.value<QColor>());
    }
    if (config->getProperty("colorthreshold", value)) {
        m_widget->intColorThreshold->setValue(value.toUInt());
    }
    if (config->getProperty("userandom", value)) {
        m_widget->chkUseRandomColors->setChecked(value.toBool());
    }
    if (config->getProperty("palette", value)) {
        m_widget->lblPalette->setText(value.toString());

        KoResourceServer<KoColorSet>* rServer = KoResourceServerProvider::instance()->paletteServer();
        KoColorSet* colorSet = rServer->resourceByName(value.toString());
        if (colorSet) {
            setColorSet(colorSet);
        }

    }
    if (config->getProperty("ignorearea", value)) {
        m_widget->intIgnoreAreas->setValue(value.toUInt());
    }
    if (config->getProperty("flagarea", value)) {
        m_widget->intFlagAreas->setValue(value.toUInt());
    }
    if (config->getProperty("flagcolor", value)) {
        m_widget->bnFlagColor->setColor(value.value<QColor>());
    }
}

KisPropertiesConfiguration* KisWdgAutofill::configuration() const
{
    KisFilterConfiguration* config = new KisFilterConfiguration("autofill", 1);
    config->setProperty("autofillcolor", m_widget->bnAutofillColor->color());
    config->setProperty("colorthreshold", m_widget->intColorThreshold->value());
    config->setProperty("userandom", m_widget->chkUseRandomColors->isChecked());
    if (m_colorSet) {
        config->setProperty("palette", m_colorSet->name());
    }
    config->setProperty("ignorearea", m_widget->intIgnoreAreas->value());
    config->setProperty("flagarea", m_widget->intFlagAreas->value());
    config->setProperty("flagcolor", m_widget->bnFlagColor->color());
    return config;
}

void KisWdgAutofill::randomSelected(bool selected)
{
    m_widget->lblPalette->setEnabled(!selected);
    m_widget->bnColorSets->setEnabled(!selected);
}

void KisWdgAutofill::setColorSet(KoColorSet *colorSet)
{
    m_colorSet = colorSet;
    m_widget->lblPalette->setText(colorSet->name());
}


