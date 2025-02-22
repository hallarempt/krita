/*
   This file is part of the KDE project
   Copyright (C) 1998, 1999 Reginald Stadlbauer <reggie@kde.org>
                 2000 Werner Trobin <trobin@kde.org>
   Copyright (C) 2004 Nicolas GOUTTE <goutte@kde.org>

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

#include <KisTemplateCreateDia.h>

#include <QFile>
#include <QLabel>
#include <QRadioButton>
#include <QPushButton>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QPixmap>
#include <QHBoxLayout>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QGroupBox>
#include <QInputDialog>
#include <QTemporaryFile>
#include <QLineEdit>

#include <klocalizedstring.h>
#include <kis_icon.h>
#include <KisDocument.h>
#include <KisTemplates.h>
#include <KisTemplateTree.h>
#include <KisTemplateGroup.h>
#include <KisTemplate.h>
#include <QMessageBox>
#include <KoResourcePaths.h>
#include <kis_debug.h>
#include <kis_icon.h>
#include <kconfiggroup.h>
#include <QUrl>

#include <ksharedconfig.h>

// ODF thumbnail extent
static const int thumbnailExtent = 128;

class KisTemplateCreateDiaPrivate {
public:
    KisTemplateCreateDiaPrivate(const QString &templatesResourcePath,
                                const QString &filePath, const QPixmap &thumbnail)
        : m_tree(templatesResourcePath, true)
        , m_filePath(filePath)
        , m_thumbnail(thumbnail)
    { }

    KisTemplateTree m_tree;
    QLineEdit *m_name;
    QRadioButton *m_default;
    QRadioButton *m_custom;
    QPushButton *m_select;
    QLabel *m_preview;
    QString m_customFile;
    QPixmap m_customPixmap;
    QTreeWidget *m_groups;
    QPushButton *m_add;
    QPushButton *m_remove;
    QCheckBox *m_defaultTemplate;
    QString m_filePath;
    QPixmap m_thumbnail;
    bool m_changed;
};


/****************************************************************************
 *
 * Class: KisTemplateCreateDia
 *
 ****************************************************************************/

KisTemplateCreateDia::KisTemplateCreateDia(const QString &templatesResourcePath,
                                           const QString &filePath, const QPixmap &thumbnail, QWidget *parent)
  : KoDialog(parent)
  , d(new KisTemplateCreateDiaPrivate(templatesResourcePath, filePath, thumbnail))
{

    setButtons( KoDialog::Ok|KoDialog::Cancel );
    setDefaultButton( KoDialog::Ok );
    setCaption( i18n( "Create Template" ) );
    setModal( true );
    setObjectName( "template create dia" );

    QWidget *mainwidget = mainWidget();
    QHBoxLayout *mbox=new QHBoxLayout( mainwidget );
    QVBoxLayout* leftbox = new QVBoxLayout();
    mbox->addLayout( leftbox );

    QLabel *label=new QLabel(i18nc("Template name", "Name:"), mainwidget);
    QHBoxLayout *namefield=new QHBoxLayout();
    leftbox->addLayout( namefield );
    namefield->addWidget(label);
    d->m_name=new QLineEdit(mainwidget);
    d->m_name->setFocus();
    connect(d->m_name, SIGNAL(textChanged(const QString &)),
            this, SLOT(slotNameChanged(const QString &)));
    namefield->addWidget(d->m_name);

    label=new QLabel(i18n("Group:"), mainwidget);
    leftbox->addWidget(label);
    d->m_groups = new QTreeWidget(mainwidget);
    leftbox->addWidget(d->m_groups);
    d->m_groups->setColumnCount(1);
    d->m_groups->setHeaderHidden(true);
    d->m_groups->setRootIsDecorated(true);
    d->m_groups->setSortingEnabled(true);

    fillGroupTree();
    d->m_groups->sortItems(0, Qt::AscendingOrder);

    QHBoxLayout *bbox=new QHBoxLayout();
    leftbox->addLayout( bbox );
    d->m_add=new QPushButton(i18n("&Add Group..."), mainwidget);
    connect(d->m_add, SIGNAL(clicked()), this, SLOT(slotAddGroup()));
    bbox->addWidget(d->m_add);
    d->m_remove=new QPushButton(i18n("&Remove"), mainwidget);
    connect(d->m_remove, SIGNAL(clicked()), this, SLOT(slotRemove()));
    bbox->addWidget(d->m_remove);

    QVBoxLayout *rightbox=new QVBoxLayout();
    mbox->addLayout( rightbox );
    QGroupBox *pixbox = new QGroupBox(i18n("Picture"), mainwidget);
    rightbox->addWidget(pixbox);
    QVBoxLayout *pixlayout=new QVBoxLayout(pixbox );
    d->m_default=new QRadioButton(i18n("&Preview"), pixbox);
    d->m_default->setChecked(true);
    connect(d->m_default, SIGNAL(clicked()), this, SLOT(slotDefault()));
    pixlayout->addWidget(d->m_default);
    QHBoxLayout *custombox=new QHBoxLayout();
    d->m_custom=new QRadioButton(i18n("Custom:"), pixbox);
    d->m_custom->setChecked(false);
    connect(d->m_custom, SIGNAL(clicked()), this, SLOT(slotCustom()));
    custombox->addWidget(d->m_custom);
    d->m_select=new QPushButton(i18n("&Select..."), pixbox);
    connect(d->m_select, SIGNAL(clicked()), this, SLOT(slotSelect()));
    custombox->addWidget(d->m_select);
    custombox->addStretch(1);
    pixlayout->addLayout(custombox);
    d->m_preview=new QLabel(pixbox); // setPixmap() -> auto resize?
    pixlayout->addWidget(d->m_preview, 0, Qt::AlignCenter);
    pixlayout->addStretch(1);

    d->m_defaultTemplate = new QCheckBox( i18n("Use the new template as default"), mainwidget );
    d->m_defaultTemplate->setChecked( true );
    d->m_defaultTemplate->setToolTip(i18n("Use the new template every time Krita starts"));
    rightbox->addWidget( d->m_defaultTemplate );

    enableButtonOk(false);
    d->m_changed=false;
    updatePixmap();

    connect(d->m_groups, SIGNAL(itemSelectionChanged()), this, SLOT(slotSelectionChanged()));

    d->m_remove->setEnabled(d->m_groups->currentItem());
    connect(this,SIGNAL(okClicked()),this,SLOT(slotOk()));
}

KisTemplateCreateDia::~KisTemplateCreateDia() {
    delete d;
}

void KisTemplateCreateDia::slotSelectionChanged()
{
    const QTreeWidgetItem* item = d->m_groups->currentItem();
    d->m_remove->setEnabled( item );
    if ( ! item )
        return;

    if ( item->parent() != NULL )
    {
        d->m_name->setText( item->text( 0 ) );
    }
}

void KisTemplateCreateDia::createTemplate(const QString &templatesResourcePath,
                                         const char *suffix,
                                         KisDocument *document, QWidget *parent)
{
    QTemporaryFile *tempFile = new QTemporaryFile(QDir::tempPath() + QLatin1String("/krita_XXXXXX") + QLatin1String(QLatin1String(suffix)));
    //Check that creation of temp file was successful
    if (!tempFile->open()) {
        delete tempFile;
        qWarning("Creation of temporary file to store template failed.");
        return;
    }
    const QString fileName = tempFile->fileName();
    tempFile->close(); // need to close on Windows before we can open it again to save
    delete tempFile; // now the file has disappeared and we can create a new file with the generated name

    document->saveNativeFormat(fileName);

    const QPixmap thumbnail = document->generatePreview(QSize(thumbnailExtent, thumbnailExtent));

    KisTemplateCreateDia *dia = new KisTemplateCreateDia(templatesResourcePath, fileName, thumbnail, parent);
    dia->exec();
    delete dia;

    QDir d;
    d.remove(fileName);
}

static void
saveAsQuadraticPng(const QPixmap &pixmap, const QString &fileName)
{
    QImage icon = pixmap.toImage();
    icon = icon.convertToFormat(QImage::Format_ARGB32);
    const int iconExtent = qMax(icon.width(), icon.height());
    icon = icon.copy((icon.width() - iconExtent) / 2, (icon.height() - iconExtent) / 2, iconExtent, iconExtent);
    icon.save(fileName, "PNG");
}

void KisTemplateCreateDia::slotOk() {

    // get the current item, if there is one...
    QTreeWidgetItem *item = d->m_groups->currentItem();
    if(!item)
        item = d->m_groups->topLevelItem(0);
    if(!item) {    // safe :)
        d->m_tree.writeTemplateTree();
        slotButtonClicked( KoDialog::Cancel );
        return;
    }
    // is it a group or a template? anyway - get the group :)
    if(item->parent() != NULL)
        item=item->parent();
    if(!item) {    // *very* safe :P
        d->m_tree.writeTemplateTree();
        slotButtonClicked( KoDialog::Cancel );
        return;
    }

    KisTemplateGroup *group=d->m_tree.find(item->text(0));
    if(!group) {    // even safer
        d->m_tree.writeTemplateTree();
        slotButtonClicked( KoDialog::Cancel );
        return;
    }

    if(d->m_name->text().isEmpty()) {
        d->m_tree.writeTemplateTree();
        slotButtonClicked( KoDialog::Cancel );
        return;
    }

    // copy the tmp file and the picture the app provides
    QString dir = KoResourcePaths::saveLocation("data", d->m_tree.templatesResourcePath());
    dir+=group->name();
    QString templateDir=dir+"/.source/";
    QString iconDir=dir+"/.icon/";

    QString file=KisTemplates::trimmed(d->m_name->text());
    QString tmpIcon=".icon/"+file;
    tmpIcon+=".png";
    QString icon=iconDir+file;
    icon+=".png";

    // try to find the extension for the template file :P
    const int pos = d->m_filePath.lastIndexOf(QLatin1Char('.'));
    QString ext;
    if ( pos > -1 )
        ext = d->m_filePath.mid(pos);
    else
        warnUI << "Template extension not found!";

    QUrl dest;
    dest.setPath(templateDir+file+ext);
    if (QFile::exists(dest.toLocalFile())) {
        do {
            file.prepend( '_' );
            dest.setPath( templateDir + file + ext );
            tmpIcon=".icon/" + file + ".png";
            icon=iconDir + file + ".png";
        }
        while (QFile(dest.toLocalFile()).exists());
    }
    bool ignore = false;
    dbgUI <<"Trying to create template:" << d->m_name->text() <<"URL=" <<".source/"+file+ext <<" ICON=" << tmpIcon;
    KisTemplate *t=new KisTemplate(d->m_name->text(), QString(), ".source/"+file+ext, tmpIcon, "", "", false, true);
    if (!group->add(t)) {
        KisTemplate *existingTemplate=group->find(d->m_name->text());
        if (existingTemplate && !existingTemplate->isHidden()) {
            if (QMessageBox::warning(this,
                                     i18nc("@title:window", "Krita"),
                                     i18n("Do you really want to overwrite the existing '%1' template?", existingTemplate->name()),
                                     QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) == QMessageBox::Yes) {
                group->add(t, true);
            }
            else {
                delete t;
                return;
            }
        }
        else {
            ignore = true;
        }
    }

    QDir path;
    if (!path.mkpath(templateDir) || !path.mkpath(iconDir)) {
        d->m_tree.writeTemplateTree();
        slotButtonClicked( KoDialog::Cancel );
        return;
    }

    QUrl orig;
    orig.setPath(d->m_filePath);
    // don't overwrite the hidden template file with a new non-hidden one
    if ( !ignore )
    {
        QFile::copy(d->m_filePath, dest.toLocalFile());
        // save the picture as icon
        if(d->m_default->isChecked() && !d->m_thumbnail.isNull()) {
            saveAsQuadraticPng(d->m_thumbnail, icon);
        } else if(!d->m_customPixmap.isNull()) {
            saveAsQuadraticPng(d->m_customPixmap, icon);
        } else {
            warnUI << "Could not save the preview picture!";
        }
    }

    // if there's a .directory file, we copy this one, too
    bool ready=false;
    QStringList tmp=group->dirs();
    for(QStringList::ConstIterator it=tmp.constBegin(); it!=tmp.constEnd() && !ready; ++it) {
        if((*it).contains(dir)==0) {
            orig.setPath( (*it)+".directory" );
            // Check if we can read the file
            if (QFile(orig.toLocalFile()).exists()) {
                dest.setPath(dir + "/.directory");
                // We copy the file with overwrite
                if (!QFile(orig.toLocalFile()).copy(dest.toLocalFile())) {
                    warnKrita << "Failed to copy from" << orig.toLocalFile() << "to" << dest.toLocalFile();
                }
                ready = true;
            }
        }
    }

    d->m_tree.writeTemplateTree();

    if ( d->m_defaultTemplate->isChecked() )
    {

      KConfigGroup grp( KSharedConfig::openConfig(), "TemplateChooserDialog");
      grp.writeEntry( "LastReturnType", "Template" );
      grp.writePathEntry( "FullTemplateName", dir + '/' + t->file() );
      grp.writePathEntry( "AlwaysUseTemplate", dir + '/' + t->file() );
    }
}

void KisTemplateCreateDia::slotDefault() {

    d->m_default->setChecked(true);
    d->m_custom->setChecked(false);
    updatePixmap();
}

void KisTemplateCreateDia::slotCustom() {

    d->m_default->setChecked(false);
    d->m_custom->setChecked(true);
    if(d->m_customFile.isEmpty())
        slotSelect();
    else
        updatePixmap();
}

void KisTemplateCreateDia::slotSelect() {

    d->m_default->setChecked(false);
    d->m_custom->setChecked(true);

    // QT5TODO
//    QString name = KIconDialog::getIcon();
//    if( name.isEmpty() ) {
//        if(d->m_customFile.isEmpty()) {
//            d->m_default->setChecked(true);
//            d->m_custom->setChecked(false);
//        }
//        return;
//    }

//    const QString path = KIconLoader::global()->iconPath(name, -thumbnailExtent);
    d->m_customFile = QString();// path;
    d->m_customPixmap = QPixmap();
    updatePixmap();
}

void KisTemplateCreateDia::slotNameChanged(const QString &name) {

    if( ( name.trimmed().isEmpty() || !d->m_groups->topLevelItem(0) ) && !d->m_changed )
        enableButtonOk(false);
    else
        enableButtonOk(true);
}

void KisTemplateCreateDia::slotAddGroup() {

    const QString name = QInputDialog::getText(this, i18n("Add Group"), i18n("Enter group name:"));
    KisTemplateGroup *group = d->m_tree.find(name);
    if (group && !group->isHidden()) {
        QMessageBox::information( this, i18n("This name is already used."), i18n("Add Group") );
        return;
    }
    QString dir = KoResourcePaths::saveLocation("data", d->m_tree.templatesResourcePath());
    dir+=name;
    KisTemplateGroup *newGroup=new KisTemplateGroup(name, dir, 0, true);
    d->m_tree.add(newGroup);
    QTreeWidgetItem *item = new QTreeWidgetItem(d->m_groups, QStringList() << name);
    d->m_groups->setCurrentItem(item);
    d->m_groups->sortItems(0, Qt::AscendingOrder);
    d->m_name->setFocus();
    enableButtonOk(true);
    d->m_changed=true;
}

void KisTemplateCreateDia::slotRemove() {

    QTreeWidgetItem *item = d->m_groups->currentItem();
    if(!item)
        return;

    QString what;
        QString removed;
        if (item->parent() == NULL) {
                what =  i18n("Do you really want to remove that group?");
                removed = i18nc("@title:window", "Remove Group");
        } else {
                what =  i18n("Do you really want to remove that template?");
        removed = i18nc("@title:window", "Remove Template");
        }

    if (QMessageBox::warning(this,
                             removed,
                             what,
                             QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) == QMessageBox:: No) {
        d->m_name->setFocus();
        return;
    }

    if(item->parent() == NULL) {
        KisTemplateGroup *group=d->m_tree.find(item->text(0));
        if(group)
            group->setHidden(true);
    }
    else {
        bool done=false;
        QList<KisTemplateGroup*> groups = d->m_tree.groups();
        QList<KisTemplateGroup*>::const_iterator it = groups.constBegin();
        for(; it != groups.constEnd() && !done; ++it) {
            KisTemplate *t = (*it)->find(item->text(0));

            if(t) {
                t->setHidden(true);
                done=true;
            }
        }
    }
    delete item;
    item=0;
    enableButtonOk(true);
    d->m_name->setFocus();
    d->m_changed=true;
}

void KisTemplateCreateDia::updatePixmap() {

    if(d->m_default->isChecked() && !d->m_thumbnail.isNull())
        d->m_preview->setPixmap(d->m_thumbnail);
    else if(d->m_custom->isChecked() && !d->m_customFile.isEmpty()) {
        if(d->m_customPixmap.isNull()) {
            dbgUI <<"Trying to load picture" << d->m_customFile;
            // use the code in KisTemplate to load the image... hacky, I know :)
            KisTemplate t("foo", "bar", QString(), d->m_customFile);
            d->m_customPixmap=t.loadPicture();
        }
        else
            warnUI << "Trying to load picture";

        if(!d->m_customPixmap.isNull())
            d->m_preview->setPixmap(d->m_customPixmap);
        else
            d->m_preview->setText(i18n("Could not load picture."));
    }
    else
        d->m_preview->setText(i18n("No picture available."));
}

void KisTemplateCreateDia::fillGroupTree() {

    Q_FOREACH (KisTemplateGroup *group, d->m_tree.groups()) {
        if(group->isHidden())
            continue;
        QTreeWidgetItem *groupItem=new QTreeWidgetItem(d->m_groups, QStringList() << group->name());

        Q_FOREACH (KisTemplate *t, group->templates()) {
            if(t->isHidden())
                continue;
            (void)new QTreeWidgetItem(groupItem, QStringList() << t->name());
        }
    }
}
