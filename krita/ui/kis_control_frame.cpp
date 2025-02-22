/*
 *  kis_control_frame.cc - part of Krita
 *
 *  Copyright (c) 1999 Matthias Elter  <elter@kde.org>
 *  Copyright (c) 2003 Patrick Julien  <freak@codepimps.org>
 *  Copyright (c) 2004 Sven Langkamp  <sven.langkamp@gmail.com>
 *  Copyright (c) 2006 Boudewijn Rempt <boud@valdyas.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.g
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "kis_control_frame.h"

#include <stdlib.h>

#include <QApplication>
#include <QLayout>
#include <QTabWidget>
#include <QFrame>
#include <QWidget>
#include <QEvent>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QMenu>
#include <QWidgetAction>
#include <QFontDatabase>

#include <klocalizedstring.h>
#include <QAction>
#include <kactioncollection.h>
#include <KoDualColorButton.h>
#include <KoAbstractGradient.h>
#include <KoResourceServer.h>
#include <KoResourceServerAdapter.h>
#include <KoResourceServerProvider.h>
#include <KoColorSpaceRegistry.h>

#include "KoPattern.h"
#include "kis_resource_server_provider.h"
#include "kis_canvas_resource_provider.h"

#include "widgets/kis_iconwidget.h"

#include "widgets/kis_gradient_chooser.h"
#include "KisViewManager.h"
#include "kis_config.h"
#include "kis_paintop_box.h"
#include "kis_custom_pattern.h"
#include "widgets/kis_pattern_chooser.h"
#include "kis_favorite_resource_manager.h"
#include "kis_display_color_converter.h"
#include <kis_canvas2.h>


KisControlFrame::KisControlFrame(KisViewManager *view, QWidget *parent, const char* name)
    : QObject(view)
    , m_viewManager(view)
    , m_patternWidget(0)
    , m_gradientWidget(0)
    , m_patternChooserPopup(0)
    , m_gradientChooserPopup(0)
    , m_paintopBox(0)
{
    setObjectName(name);
    KisConfig cfg;
    m_font = QFontDatabase::systemFont(QFontDatabase::GeneralFont);

    m_patternWidget = new KisIconWidget(parent, "patterns");
    m_patternWidget->setText(i18n("Fill Patterns"));
    m_patternWidget->setToolTip(i18n("Fill Patterns"));
    m_patternWidget->setFixedSize(26, 26);

    m_gradientWidget = new KisIconWidget(parent, "gradients");
    m_gradientWidget->setText(i18n("Gradients"));
    m_gradientWidget->setToolTip(i18n("Gradients"));
    m_gradientWidget->setFixedSize(26, 26);

    KoResourceServer<KoAbstractGradient> * rserver = KoResourceServerProvider::instance()->gradientServer(false);
    QSharedPointer<KoAbstractResourceServerAdapter> adapter (new KoResourceServerAdapter<KoAbstractGradient>(rserver));
    m_gradientWidget->setResourceAdapter(adapter);
}

void KisControlFrame::setup(QWidget *parent)
{
    createPatternsChooser(m_viewManager);
    createGradientsChooser(m_viewManager);

    QWidgetAction *action  = new QWidgetAction(this);
    action->setText(i18n("&Patterns"));
    m_viewManager->actionCollection()->addAction("patterns", action);
    action->setDefaultWidget(m_patternWidget);

    action = new QWidgetAction(this);
    action->setText(i18n("&Gradients"));
    m_viewManager->actionCollection()->addAction("gradients", action);
    action->setDefaultWidget(m_gradientWidget);


    // XXX: KOMVC we don't have a canvas here yet, needs a setImageView
    const KoColorDisplayRendererInterface *displayRenderer = \
        KisDisplayColorConverter::dumbConverterInstance()->displayRendererInterface();
    KoDualColorButton * dual = new KoDualColorButton(m_viewManager->resourceProvider()->fgColor(),
                                                     m_viewManager->resourceProvider()->bgColor(), displayRenderer,
                                                     m_viewManager->mainWindow(), m_viewManager->mainWindow());
    dual->setPopDialog(true);
    action = new QWidgetAction(this);
    action->setText(i18n("&Color"));
    m_viewManager->actionCollection()->addAction("dual", action);
    action->setDefaultWidget(dual);
    connect(dual, SIGNAL(foregroundColorChanged(KoColor)), m_viewManager->resourceProvider(), SLOT(slotSetFGColor(KoColor)));
    connect(dual, SIGNAL(backgroundColorChanged(KoColor)), m_viewManager->resourceProvider(), SLOT(slotSetBGColor(KoColor)));
    connect(m_viewManager->resourceProvider(), SIGNAL(sigFGColorChanged(KoColor)), dual, SLOT(setForegroundColor(KoColor)));
    connect(m_viewManager->resourceProvider(), SIGNAL(sigBGColorChanged(KoColor)), dual, SLOT(setBackgroundColor(KoColor)));
    dual->setFixedSize(26, 26);

    m_paintopBox = new KisPaintopBox(m_viewManager, parent, "paintopbox");

    action = new QWidgetAction(this);
    action->setText(i18n("&Painter's Tools"));
    m_viewManager->actionCollection()->addAction("paintops", action);
    action->setDefaultWidget(m_paintopBox);
}

void KisControlFrame::slotSetPattern(KoPattern * pattern)
{
    m_patternWidget->slotSetItem(pattern);
    m_patternChooser->setCurrentPattern(pattern);
}

void KisControlFrame::slotSetGradient(KoAbstractGradient * gradient)
{
    m_gradientWidget->slotSetItem(gradient);
}

void KisControlFrame::createPatternsChooser(KisViewManager * view)
{
    if (m_patternChooserPopup) delete m_patternChooserPopup;
    m_patternChooserPopup = new QWidget(m_patternWidget);
    m_patternChooserPopup->setObjectName("pattern_chooser_popup");
    QHBoxLayout * l2 = new QHBoxLayout(m_patternChooserPopup);
    l2->setObjectName("patternpopuplayout");

    m_patternsTab = new QTabWidget(m_patternChooserPopup);
    m_patternsTab->setObjectName("patternstab");
    m_patternsTab->setFocusPolicy(Qt::NoFocus);
    m_patternsTab->setFont(m_font);
    l2->addWidget(m_patternsTab);

    m_patternChooser = new KisPatternChooser(m_patternChooserPopup);
    m_patternChooser->setFont(m_font);
    QWidget *patternChooserPage = new QWidget(m_patternChooserPopup);
    QHBoxLayout *patternChooserPageLayout  = new QHBoxLayout(patternChooserPage);
    patternChooserPageLayout->addWidget(m_patternChooser);
    m_patternsTab->addTab(patternChooserPage, i18n("Patterns"));

    KisCustomPattern* customPatterns = new KisCustomPattern(0, "custompatterns",
                                                            i18n("Custom Pattern"), m_viewManager);
    customPatterns->setFont(m_font);
    m_patternsTab->addTab(customPatterns, i18n("Custom Pattern"));

    connect(m_patternChooser, SIGNAL(resourceSelected(KoResource*)),
            view->resourceProvider(), SLOT(slotPatternActivated(KoResource*)));

    connect(customPatterns, SIGNAL(activatedResource(KoResource*)),
            view->resourceProvider(), SLOT(slotPatternActivated(KoResource*)));

    connect(view->resourceProvider(), SIGNAL(sigPatternChanged(KoPattern*)),
            this, SLOT(slotSetPattern(KoPattern*)));

    m_patternChooser->setCurrentItem(0, 0);
    if (m_patternChooser->currentResource() && view->resourceProvider()) {
        view->resourceProvider()->slotPatternActivated(m_patternChooser->currentResource());
    }

    m_patternWidget->setPopupWidget(m_patternChooserPopup);


}

void KisControlFrame::createGradientsChooser(KisViewManager * view)
{
    if (m_gradientChooserPopup) delete m_gradientChooserPopup;
    m_gradientChooserPopup = new QWidget(m_gradientWidget);
    m_gradientChooserPopup->setObjectName("gradient_chooser_popup");
    QHBoxLayout * l2 = new QHBoxLayout(m_gradientChooserPopup);
    l2->setObjectName("gradientpopuplayout");

    m_gradientTab = new QTabWidget(m_gradientChooserPopup);
    m_gradientTab->setObjectName("gradientstab");
    m_gradientTab->setFocusPolicy(Qt::NoFocus);
    m_gradientTab->setFont(m_font);
    l2->addWidget(m_gradientTab);

    m_gradientChooser = new KisGradientChooser(m_gradientChooserPopup);
    m_gradientChooser->setFont(m_font);
    m_gradientTab->addTab(m_gradientChooser, i18n("Gradients"));

    connect(m_gradientChooser, SIGNAL(resourceSelected(KoResource*)),
            view->resourceProvider(), SLOT(slotGradientActivated(KoResource*)));

    connect(view->resourceProvider(), SIGNAL(sigGradientChanged(KoAbstractGradient*)),
            this, SLOT(slotSetGradient(KoAbstractGradient*)));

    m_gradientChooser->setCurrentItem(0, 0);
    if (m_gradientChooser->currentResource() && view->resourceProvider())
        view->resourceProvider()->slotGradientActivated(m_gradientChooser->currentResource());

    m_gradientWidget->setPopupWidget(m_gradientChooserPopup);

}


