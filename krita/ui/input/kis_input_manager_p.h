/*
 *  Copyright (C) 2015 Michael Abrahams <miabraha@gmail.com>
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


#include <QList>
#include <QPointer>
#include <QSet>
#include <QEvent>
#include <QTouchEvent>
#include <QScopedPointer>

#include "kis_input_manager.h"
#include "kis_shortcut_matcher.h"
#include "kis_shortcut_configuration.h"
#include "kis_canvas2.h"
#include "kis_tool_proxy.h"
#include "kis_signal_compressor.h"
#include "input/kis_tablet_debugger.h"

class KisToolInvocationAction;


class KisInputManager::Private
{
public:
    Private(KisInputManager *qq);
    bool tryHidePopupPalette();
    void addStrokeShortcut(KisAbstractInputAction* action, int index, const QList< Qt::Key >& modifiers, Qt::MouseButtons buttons);
    void addKeyShortcut(KisAbstractInputAction* action, int index,const QList<Qt::Key> &keys);
    void addTouchShortcut( KisAbstractInputAction* action, int index, KisShortcutConfiguration::GestureAction gesture );
    void addWheelShortcut(KisAbstractInputAction* action, int index, const QList< Qt::Key >& modifiers, KisShortcutConfiguration::MouseWheelMovement wheelAction);
    bool processUnhandledEvent(QEvent *event);
    void setupActions();
    void saveTouchEvent( QTouchEvent* event );
    bool handleCompressedTabletEvent(QObject *object, QTabletEvent *tevent);

    KisInputManager *q;

    KisCanvas2 *canvas = 0;
    KisToolProxy *toolProxy = 0;

    bool forwardAllEventsToTool = false;
    bool ignoreQtCursorEvents();

    bool disableTouchOnCanvas = false;
    bool touchHasBlockedPressEvents = false;

    KisShortcutMatcher matcher;
    QTouchEvent *lastTouchEvent = 0;

    KisToolInvocationAction *defaultInputAction = 0;

    QObject *eventsReceiver = 0;
    KisSignalCompressor moveEventCompressor;
    QScopedPointer<QTabletEvent> compressedMoveEvent;
    bool testingAcceptCompressedTabletEvents = false;
    bool testingCompressBrushEvents = false;


    QSet<QPointer<QObject> > priorityEventFilter;

    void blockMouseEvents();
    void allowMouseEvents();

    template <class Event, bool useBlocking>
    void debugEvent(QEvent *event)
    {
      if (!KisTabletDebugger::instance()->debugEnabled()) return;
      QString msg1 = useBlocking && ignoreQtCursorEvents() ? "[BLOCKED] " : "[       ]";
      Event *specificEvent = static_cast<Event*>(event);
      dbgTablet << KisTabletDebugger::instance()->eventToString(*specificEvent, msg1);
    }

    class ProximityNotifier : public QObject
    {
    public:
        ProximityNotifier(Private *_d, QObject *p);
        bool eventFilter(QObject* object, QEvent* event );
    private:
        KisInputManager::Private *d;
    };

    class CanvasSwitcher : public QObject
    {
    public:
        CanvasSwitcher(Private *_d, QObject *p);
        void addCanvas(KisCanvas2 *canvas);
        void removeCanvas(KisCanvas2 *canvas);
        bool eventFilter(QObject* object, QEvent* event );

    private:
        KisInputManager::Private *d;
        QMap<QObject*, KisCanvas2*> canvasResolver;
        int eatOneMouseStroke;
    };
    CanvasSwitcher canvasSwitcher;

#if !defined(Q_OS_WIN)
    class EventEater
    {
    public:
        bool eventFilter(QObject* target, QEvent* event);

        // This should be called after we're certain a tablet stroke has started.
        void activate();
        // This should be called after a tablet stroke has ended.
        void deactivate();
        bool isActive();

    private:
        bool hungry{false};   // Continue eating mouse strokes
        bool peckish{false};  // Eat a single mouse press event
    };
    EventEater eventEater;
#endif

    bool focusOnEnter = true;
    bool containsPointer = true;
};
