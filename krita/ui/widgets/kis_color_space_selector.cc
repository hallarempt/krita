    /*
 *  Copyright (C) 2007 Cyrille Berger <cberger@cberger.net>
 *  Copyright (C) 2011 Boudewijn Rempt <boud@valdyas.org>
 *  Copyright (C) 2011 Srikanth Tiyyagura <srikanth.tulasiram@gmail.com>
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

#include "kis_color_space_selector.h"
#include "kis_advanced_color_space_selector.h"


#include <QUrl>

#include <KoFileDialog.h>
#include <KoColorProfile.h>
#include <KoColorSpace.h>
#include <KoColorSpaceRegistry.h>
#include <KoColorSpaceEngine.h>
#include <KoID.h>

#include <KoConfig.h>
#include <kis_icon.h>

#include <QDesktopServices>

#include <KoResourcePaths.h>


#include <kis_debug.h>

#include "ui_wdgcolorspaceselector.h"

struct KisColorSpaceSelector::Private {
    Ui_WdgColorSpaceSelector* colorSpaceSelector;
    QString knsrcFile;
    bool profileValid;
    QString defaultsuffix;
};

KisColorSpaceSelector::KisColorSpaceSelector(QWidget* parent) : QWidget(parent), m_advancedSelector(0), d(new Private)
{
    setObjectName("KisColorSpaceSelector");
    d->colorSpaceSelector = new Ui_WdgColorSpaceSelector;
    d->colorSpaceSelector->setupUi(this);
    d->colorSpaceSelector->cmbColorModels->setIDList(KoColorSpaceRegistry::instance()->colorModelsList(KoColorSpaceRegistry::OnlyUserVisible));
    fillCmbDepths(d->colorSpaceSelector->cmbColorModels->currentItem());

    d->colorSpaceSelector->bnInstallProfile->setIcon(KisIconUtils::loadIcon("document-open"));
    d->colorSpaceSelector->bnInstallProfile->setToolTip( i18n("Open Color Profile") );

    connect(d->colorSpaceSelector->cmbColorModels, SIGNAL(activated(const KoID &)),
            this, SLOT(fillCmbDepths(const KoID &)));
    connect(d->colorSpaceSelector->cmbColorDepth, SIGNAL(activated(const KoID &)),
            this, SLOT(fillCmbProfiles()));
    connect(d->colorSpaceSelector->cmbColorModels, SIGNAL(activated(const KoID &)),
            this, SLOT(fillCmbProfiles()));
    connect(d->colorSpaceSelector->cmbProfile, SIGNAL(activated(const QString &)),
            this, SLOT(colorSpaceChanged()));
    connect(d->colorSpaceSelector->bnInstallProfile, SIGNAL(clicked()), 
            this, SLOT(installProfile()));

    d->defaultsuffix = " "+i18nc("This is appended to the color profile which is the default for the given colorspace and bit-depth","(Default)");

    connect(d->colorSpaceSelector->bnAdvanced, SIGNAL(clicked()), this,  SLOT(slotOpenAdvancedSelector()));

    fillCmbProfiles();
}

KisColorSpaceSelector::~KisColorSpaceSelector()
{
    delete d->colorSpaceSelector;
    delete d;
}


void KisColorSpaceSelector::fillCmbProfiles()
{
    QString s = KoColorSpaceRegistry::instance()->colorSpaceId(d->colorSpaceSelector->cmbColorModels->currentItem(), d->colorSpaceSelector->cmbColorDepth->currentItem());
    d->colorSpaceSelector->cmbProfile->clear();

    const KoColorSpaceFactory * csf = KoColorSpaceRegistry::instance()->colorSpaceFactory(s);
    if (csf == 0) return;

    QList<const KoColorProfile *>  profileList = KoColorSpaceRegistry::instance()->profilesFor(csf);
    QStringList profileNames;
    Q_FOREACH (const KoColorProfile *profile, profileList) {
        profileNames.append(profile->name());
    }
    qSort(profileNames);
    Q_FOREACH (QString stringName, profileNames) {
        if (stringName==csf->defaultProfile()) {
            d->colorSpaceSelector->cmbProfile->addSqueezedItem(stringName+d->defaultsuffix);
        } else {
            d->colorSpaceSelector->cmbProfile->addSqueezedItem(stringName);
        }
    }
    d->colorSpaceSelector->cmbProfile->setCurrent(csf->defaultProfile()+d->defaultsuffix);
    colorSpaceChanged();
}

void KisColorSpaceSelector::fillCmbDepths(const KoID& id)
{
    KoID activeDepth = d->colorSpaceSelector->cmbColorDepth->currentItem();
    d->colorSpaceSelector->cmbColorDepth->clear();
    QList<KoID> depths = KoColorSpaceRegistry::instance()->colorDepthList(id, KoColorSpaceRegistry::OnlyUserVisible);
    d->colorSpaceSelector->cmbColorDepth->setIDList(depths);
    if (depths.contains(activeDepth)) {
        d->colorSpaceSelector->cmbColorDepth->setCurrent(activeDepth);
    }
}

const KoColorSpace* KisColorSpaceSelector::currentColorSpace()
{
    QString profilenamestring = d->colorSpaceSelector->cmbProfile->itemHighlighted();
    if (profilenamestring.contains(d->defaultsuffix)) {
        profilenamestring.remove(d->defaultsuffix);
        return KoColorSpaceRegistry::instance()->colorSpace(
               d->colorSpaceSelector->cmbColorModels->currentItem().id(),
               d->colorSpaceSelector->cmbColorDepth->currentItem().id(),
               profilenamestring);
    } else {
        return KoColorSpaceRegistry::instance()->colorSpace(
               d->colorSpaceSelector->cmbColorModels->currentItem().id(),
               d->colorSpaceSelector->cmbColorDepth->currentItem().id(),
               profilenamestring);
    }
}

void KisColorSpaceSelector::setCurrentColorModel(const KoID& id)
{
    d->colorSpaceSelector->cmbColorModels->setCurrent(id);
    fillCmbDepths(id);
}

void KisColorSpaceSelector::setCurrentColorDepth(const KoID& id)
{
    d->colorSpaceSelector->cmbColorDepth->setCurrent(id);
    fillCmbProfiles();
}

void KisColorSpaceSelector::setCurrentProfile(const QString& name)
{
    d->colorSpaceSelector->cmbProfile->setCurrent(name);
}

void KisColorSpaceSelector::setCurrentColorSpace(const KoColorSpace* colorSpace)
{
  setCurrentColorModel(colorSpace->colorModelId());
  setCurrentColorDepth(colorSpace->colorDepthId());
  setCurrentProfile(colorSpace->profile()->name());
}

void KisColorSpaceSelector::colorSpaceChanged()
{
    bool valid = d->colorSpaceSelector->cmbProfile->count() != 0;
    d->profileValid = valid;
    emit(selectionChanged(valid));
    if(valid) {
        emit colorSpaceChanged(currentColorSpace());
        QString text = currentColorSpace()->profile()->name();
    }
}

void KisColorSpaceSelector::installProfile()
{
    QStringList mime;
    mime << "*.icm" <<  "*.icc";
    KoFileDialog dialog(this, KoFileDialog::OpenFiles, "OpenDocumentICC");
    dialog.setCaption(i18n("Install Color Profiles"));
    dialog.setDefaultDir(QDesktopServices::storageLocation(QDesktopServices::HomeLocation));
    dialog.setNameFilters(mime);
    QStringList profileNames = dialog.filenames();

    KoColorSpaceEngine *iccEngine = KoColorSpaceEngineRegistry::instance()->get("icc");
    Q_ASSERT(iccEngine);

    QString saveLocation = KoResourcePaths::saveLocation("icc_profiles");

    Q_FOREACH (const QString &profileName, profileNames) {
        QUrl file(profileName);
        if (!QFile::copy(profileName, saveLocation + file.fileName())) {
            dbgKrita << "Could not install profile!";
            return;
        }
        iccEngine->addProfile(saveLocation + file.fileName());

    }

    fillCmbProfiles();
}

void KisColorSpaceSelector::slotOpenAdvancedSelector()
{
    if (!m_advancedSelector) {
        m_advancedSelector = new KisAdvancedColorSpaceSelector(this, "Select a Colorspace");
        m_advancedSelector->setModal(true);
        m_advancedSelector->setCurrentColorSpace(currentColorSpace());
        connect(m_advancedSelector, SIGNAL(selectionChanged(bool)), this, SLOT(slotProfileValid(bool)) );
    }

    QDialog::DialogCode result = (QDialog::DialogCode)m_advancedSelector->exec();

    if (result) {
        if (d->profileValid==true) {
            setCurrentColorSpace(m_advancedSelector->currentColorSpace());
        }
    }
}

void KisColorSpaceSelector::slotProfileValid(bool valid)
{
    d->profileValid = valid;
}

