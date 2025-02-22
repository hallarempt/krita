/*
 * Copyright (c) 1999 Matthias Elter <me@kde.org>
 * Copyright (c) 2002 Patrick Julien <freak@codepimps.org>
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

#include <stdlib.h>

#include <QString>
#include <QPixmap>
#include <kis_debug.h>
#include <QProcess>
#include <QProcessEnvironment>
#include <QDesktopServices>
#include <QDir>
#include <QDate>
#include <QLoggingCategory>

#include <KisApplication.h>
#include <KoConfig.h>

#include "data/splash/splash_screen.xpm"
#include "data/splash/splash_holidays.xpm"
#include "ui/KisDocument.h"
#include "kis_splash_screen.h"
#include "KisPart.h"
#include "KisApplicationArguments.h"

#if defined Q_OS_WIN
#include <Windows.h>
#include <stdlib.h>
#include <ui/input/wintab/kis_tablet_support_win.h>

#elif defined HAVE_X11
    #include <ui/input/wintab/kis_tablet_support_x11.h>
    #include <ui/input/wintab/kis_xi2_event_filter.h>
#endif

#if defined HAVE_KCRASH
#include <kcrash.h>
#endif
extern "C" int main(int argc, char **argv)
{
    bool runningInKDE = !qgetenv("KDE_FULL_SESSION").isEmpty();

#ifdef HAVE_X11
    if (runningInKDE) {
        qputenv("QT_NO_GLIB", "1");
    }
#endif

    /**
     * Disable debug output by default. (krita.input enables tablet debugging.)
     * Debug logs can be controlled by an environment variable QT_LOGGING_RULES.
     *
     * As an example, to get full debug output, run the following:
     * export QT_LOGGING_RULES="krita*=true"; krita
     *
     * See: http://doc.qt.io/qt-5/qloggingcategory.html
     */
    QLoggingCategory::setFilterRules("calligra*=false\n"
                                     "krita*=false\n"
                                     "krita.tabletlog=true");


    // A per-user unique string, without /, because QLocalServer cannot use names with a / in it
    QString key = "Krita3" +
                  QDesktopServices::storageLocation(QDesktopServices::HomeLocation).replace("/", "_");
    key = key.replace(":", "_").replace("\\","_");
#if defined HAVE_X11
    // we need to call XInitThreads() (which this does) because of gmic (and possibly others)
    // do their own X11 stuff in their own threads
    // this call must happen before the creation of the application (see AA_X11InitThreads docs)
    QCoreApplication::setAttribute(Qt::AA_X11InitThreads, true);
#endif

#if defined HAVE_OPENGL
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts, true);
    QCoreApplication::setAttribute(Qt::AA_DontCreateNativeWidgetSiblings, true);
#endif
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps, true);

    KLocalizedString::setApplicationDomain("krita");

    QCoreApplication::addLibraryPath(QCoreApplication::applicationDirPath());
    // first create the application so we can create a pixmap
    KisApplication app(key, argc, argv);

    if (qApp->applicationDirPath().contains(KRITA_BUILD_DIR)) {
        qFatal("FATAL: You're trying to run krita from the build location. You can only run Krita from the installation location.");
    }


#if defined HAVE_KCRASH
    KCrash::initialize();
#endif

    // If we should clear the config, it has to be done as soon as possible after
    // KisApplication has been created. Otherwise the config file may have been read
    // and stored in a KConfig object we have no control over.
    app.askClearConfig();

    KisApplicationArguments args(app);

    if (app.isRunning()) {
        // only pass arguments to main instance if they are not for batch processing
        // any batch processing would be done in this separate instance
        const bool batchRun = (args.print() || args.exportAs() || args.exportAsPdf());

        if (!batchRun) {
            QByteArray ba = args.serialize();
            if (app.sendMessage(ba)) {
                return 0;
            }
        }
    }

    if (!runningInKDE) {
        // Icons in menus are ugly and distracting
        app.setAttribute(Qt::AA_DontShowIconsInMenus);
    }

#if defined HAVE_X11
    app.installNativeEventFilter(KisXi2EventFilter::instance());
#endif

    // then create the pixmap from an xpm: we cannot get the
    // location of our datadir before we've started our components,
    // so use an xpm.
    QDate currentDate = QDate::currentDate();
    QWidget *splash = 0;
    if (currentDate > QDate(currentDate.year(), 12, 4) ||
            currentDate < QDate(currentDate.year(), 1, 9)) {
         splash = new KisSplashScreen(app.applicationVersion(), QPixmap(splash_holidays_xpm));
    }
    else {
        splash = new KisSplashScreen(app.applicationVersion(), QPixmap(splash_screen_xpm));
    }

    app.setSplashScreen(splash);


#if defined Q_OS_WIN
    KisTabletSupportWin::init();
    // app.installNativeEventFilter(new KisTabletSupportWin());
#endif

    if (!app.start(args)) {
        return 1;
    }

    // Set up remote arguments.
    QObject::connect(&app, SIGNAL(messageReceived(QByteArray,QObject*)),
                     &app, SLOT(remoteArguments(QByteArray,QObject*)));

    QObject::connect(&app, SIGNAL(fileOpenRequest(QString)),
                     &app, SLOT(fileOpenRequested(QString)));

    int state = app.exec();

    return state;
}

