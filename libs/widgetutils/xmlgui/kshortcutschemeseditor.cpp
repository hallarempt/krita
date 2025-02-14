/* This file is part of the KDE libraries
    Copyright (C) 2008 Alexander Dymo <adymo@kdevelop.org>

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
    Boston, MA 02110-1301, USA.
*/
#include "KisShortcutsDialog_p.h"

#include <QDir>
#include <QLabel>
#include <QMenu>
#include <QFile>
#include <QPushButton>
#include <QTextStream>
#include <QtXml/QDomDocument>
#include <QFileDialog>
#include <QStandardPaths>
#include <QInputDialog>
#include <QComboBox>
#include <QHBoxLayout>
#include <QDebug>

#include <kconfiggroup.h>
#include <kmessagebox.h>
#include <ksharedconfig.h>

#include "KisShortcutsDialog.h"
#include "kshortcutschemeshelper_p.h"
#include "kactioncollection.h"
#include "kxmlguiclient.h"

#include "KoResourcePaths.h"


KShortcutSchemesEditor::KShortcutSchemesEditor(KisShortcutsDialog *parent)
    : QHBoxLayout(parent)
    , m_dialog(parent)
{
    KConfigGroup group(KSharedConfig::openConfig(), "Shortcut Schemes");

    QStringList schemes;
    schemes << QStringLiteral("Default");

    auto schemeFileLocations = KShortcutSchemesHelper::schemeFileLocations();
    schemes << schemeFileLocations.keys();

    const QString currentScheme = group.readEntry("Current Scheme", "Default");
    setMargin(0);

    QLabel *schemesLabel = new QLabel(i18n("Shortcut Schemes:"), m_dialog);
    addWidget(schemesLabel);

    m_schemesList = new QComboBox(m_dialog);
    m_schemesList->setEditable(false);
    m_schemesList->addItems(schemes);
    m_schemesList->setCurrentIndex(m_schemesList->findText(currentScheme));
    schemesLabel->setBuddy(m_schemesList);
    addWidget(m_schemesList);

    m_newScheme = new QPushButton(i18n("New..."));
    addWidget(m_newScheme);

    m_deleteScheme = new QPushButton(i18n("Delete"));
    addWidget(m_deleteScheme);

    QPushButton *moreActions = new QPushButton(i18n("More Actions"));
    addWidget(moreActions);

    QMenu *moreActionsMenu = new QMenu(m_dialog);
    // moreActionsMenu->addAction(i18n("Save as Scheme Defaults"),
                               // this, SLOT(saveAsDefaultsForScheme()));

    moreActionsMenu->addAction(i18n("Save Custom Shortcuts"),
                               this, SLOT(saveCustomShortcuts()));
    moreActionsMenu->addAction(i18n("Export Scheme..."),
                               this, SLOT(exportShortcutsScheme()));
    moreActionsMenu->addAction(i18n("Import Scheme..."),
                               this, SLOT(importShortcutsScheme()));
    moreActionsMenu->addAction(i18n("Restore Defaults"),
                               m_dialog, SLOT(allDefault()));
    moreActions->setMenu(moreActionsMenu);

    addStretch(1);

    connect(m_schemesList, SIGNAL(activated(QString)),
            this, SIGNAL(shortcutsSchemeChanged(QString)));
    connect(m_newScheme, SIGNAL(clicked()), this, SLOT(newScheme()));
    connect(m_deleteScheme, SIGNAL(clicked()), this, SLOT(deleteScheme()));
    updateDeleteButton();
}

void KShortcutSchemesEditor::newScheme()
{
    bool ok;
    const QString newName = QInputDialog::getText(m_dialog, i18n("Name for New Scheme"),
                            i18n("Name for new scheme:"), QLineEdit::Normal, i18n("New Scheme"), &ok);
    if (!ok) {
        return;
    }

    if (m_schemesList->findText(newName) != -1) {
        KMessageBox::sorry(m_dialog, i18n("A scheme with this name already exists."));
        return;
    }

    const QString newSchemeFileName = KShortcutSchemesHelper::shortcutSchemeFileName(newName);

    QFile schemeFile(newSchemeFileName);
    if (!schemeFile.open(QFile::WriteOnly | QFile::Truncate)) {
        return;
    }
    schemeFile.close();

    m_dialog->exportConfiguration(newSchemeFileName);
    m_schemesList->addItem(newName);
    m_schemesList->setCurrentIndex(m_schemesList->findText(newName));
    m_schemeFileLocations.insert(newName, newSchemeFileName);
    updateDeleteButton();
    emit shortcutsSchemeChanged(newName);
}

void KShortcutSchemesEditor::deleteScheme()
{
    if (KMessageBox::questionYesNo(m_dialog,
                                   i18n("Do you really want to delete the scheme %1?\n\
Note that this will not remove any system wide shortcut schemes.", currentScheme())) == KMessageBox::No) {
        return;
    }

    //delete the scheme for the app itself
    QFile::remove(KShortcutSchemesHelper::shortcutSchemeFileName(currentScheme()));

    m_schemesList->removeItem(m_schemesList->findText(currentScheme()));
    updateDeleteButton();
    emit shortcutsSchemeChanged(currentScheme());
}

QString KShortcutSchemesEditor::currentScheme()
{
    return m_schemesList->currentText();
}

void KShortcutSchemesEditor::exportShortcutsScheme()
{
    //ask user about dir
    QString path = QFileDialog::getSaveFileName(m_dialog, i18n("Export Shortcuts"),
                                                KoResourcePaths::saveLocation("kis_shortcuts"),
                                                i18n("Shortcuts (*.shortcuts)"));
    if (path.isEmpty()) {
        return;
    }

    m_dialog->exportConfiguration(path);
}

void KShortcutSchemesEditor::saveCustomShortcuts()
{
  //ask user about dir
  QString path = QFileDialog::getSaveFileName(m_dialog, i18n("Save Shortcuts"),
                                              QDir::currentPath(),
                                              i18n("Shortcuts (*.shortcuts)"));
  if (path.isEmpty()) {
    return;
  }

  m_dialog->saveCustomShortcuts(path);
}

void KShortcutSchemesEditor::importShortcutsScheme()
{
    //ask user about dir
    QString path = QFileDialog::getOpenFileName(m_dialog, i18n("Import Shortcuts"), QDir::currentPath(), i18n("Shortcuts (*.shortcuts)"));
    if (path.isEmpty()) {
        return;
    }

    m_dialog->importConfiguration(path);
}

#if 0
// XXX: Not implemented
void KShortcutSchemesEditor::saveAsDefaultsForScheme()
{
    foreach (KActionCollection *collection, m_dialog->actionCollections()) {
        KShortcutSchemesHelper::exportActionCollection(collection, currentScheme());
    }
}
#endif

void KShortcutSchemesEditor::updateDeleteButton()
{
    m_deleteScheme->setEnabled(m_schemesList->count() >= 1);
}
