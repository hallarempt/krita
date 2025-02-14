/*
 *  Copyright (c) 2004 Boudewijn Rempt <boud@valdyas.org>
 *  Copyright (c) 2007 Sven Langkamp <sven.langkamp@gmail.com>
 *
 *  The outline algorith uses the limn algorithm of fontutils by
 *  Karl Berry <karl@cs.umb.edu> and Kathryn Hargreaves <letters@cs.umb.edu>
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

#include "kis_selection_manager.h"
#include <QApplication>
#include <QClipboard>
#include <QColor>
#include <QTimer>

#include <QAction>
#include <ktoggleaction.h>
#include <klocalizedstring.h>
#include <kstandardaction.h>
#include <kactioncollection.h>

#include "KoCanvasController.h"
#include "KoChannelInfo.h"
#include "KoIntegerMaths.h"
#include <KisDocument.h>
#include <KisMainWindow.h>
#include <KisDocumentEntry.h>
#include <KoViewConverter.h>
#include <KoSelection.h>
#include <KoShapeManager.h>
#include <KoShapeStroke.h>
#include <KoColorSpace.h>
#include <KoCompositeOp.h>
#include <KoToolProxy.h>
#include <kis_icon.h>

#include "kis_adjustment_layer.h"
#include "kis_node_manager.h"
#include "canvas/kis_canvas2.h"
#include "kis_config.h"
#include "kis_convolution_painter.h"
#include "kis_convolution_kernel.h"
#include "kis_debug.h"
#include "KisDocument.h"
#include "kis_fill_painter.h"
#include "kis_group_layer.h"
#include "kis_layer.h"
#include "kis_statusbar.h"
#include "kis_paint_device.h"
#include "kis_paint_layer.h"
#include "kis_painter.h"
#include "kis_transaction.h"
#include "kis_selection.h"
#include "kis_types.h"
#include "kis_canvas_resource_provider.h"
#include "kis_undo_adapter.h"
#include "kis_pixel_selection.h"
#include "flake/kis_shape_selection.h"
#include "commands/kis_selection_commands.h"
#include "kis_selection_mask.h"
#include "flake/kis_shape_layer.h"
#include "kis_selection_decoration.h"
#include "canvas/kis_canvas_decoration.h"
#include "kis_node_commands_adapter.h"
#include "kis_iterator_ng.h"
#include "kis_clipboard.h"
#include "KisViewManager.h"
#include "kis_selection_filters.h"
#include "kis_figure_painting_tool_helper.h"
#include "KisView.h"

#include "actions/kis_selection_action_factories.h"
#include "kis_action.h"
#include "kis_action_manager.h"
#include "operations/kis_operation_configuration.h"


KisSelectionManager::KisSelectionManager(KisViewManager * view)
        : m_view(view),
          m_doc(0),
          m_imageView(0),
          m_adapter(new KisNodeCommandsAdapter(view)),
          m_copy(0),
          m_copyMerged(0),
          m_cut(0),
          m_paste(0),
          m_pasteNew(0),
          m_cutToNewLayer(0),
          m_selectAll(0),
          m_deselect(0),
          m_clear(0),
          m_reselect(0),
          m_invert(0),
          m_copyToNewLayer(0),
          m_fillForegroundColor(0),
          m_fillBackgroundColor(0),
          m_fillPattern(0),
          m_imageResizeToSelection(0),
          m_selectionDecoration(0)
{
    m_clipboard = KisClipboard::instance();
}

KisSelectionManager::~KisSelectionManager()
{
}

void KisSelectionManager::setup(KisActionManager* actionManager)
{
    m_cut = actionManager->createStandardAction(KStandardAction::Cut, this, SLOT(cut()));
    m_copy = actionManager->createStandardAction(KStandardAction::Copy, this, SLOT(copy()));
    m_paste = actionManager->createStandardAction(KStandardAction::Paste, this, SLOT(paste()));

    KisAction *action = actionManager->createAction("copy_sharp");
    connect(action, SIGNAL(triggered()), this, SLOT(copySharp()));

    action = actionManager->createAction("cut_sharp");
    connect(action, SIGNAL(triggered()), this, SLOT(cutSharp()));

    m_pasteNew = actionManager->createAction("paste_new");
    connect(m_pasteNew, SIGNAL(triggered()), this, SLOT(pasteNew()));

    m_pasteAt = actionManager->createAction("paste_at");
    connect(m_pasteAt, SIGNAL(triggered()), this, SLOT(pasteAt()));

    m_copyMerged = actionManager->createAction("copy_merged");
    connect(m_copyMerged, SIGNAL(triggered()), this, SLOT(copyMerged()));

    m_selectAll = actionManager->createAction("select_all");
    connect(m_selectAll, SIGNAL(triggered()), this, SLOT(selectAll()));

    m_deselect = actionManager->createAction("deselect");
    connect(m_deselect, SIGNAL(triggered()), this, SLOT(deselect()));

    m_clear = actionManager->createAction("clear");
    connect(m_clear, SIGNAL(triggered()), SLOT(clear()));

    m_reselect = actionManager->createAction("reselect");
    connect(m_reselect, SIGNAL(triggered()), this, SLOT(reselect()));

    m_invert = actionManager->createAction("invert");
    m_invert->setOperationID("invertselection");

    actionManager->registerOperation(new KisInvertSelectionOperaton);
    
    m_copyToNewLayer = actionManager->createAction("copy_selection_to_new_layer");
    connect(m_copyToNewLayer, SIGNAL(triggered()), this, SLOT(copySelectionToNewLayer()));

    m_cutToNewLayer = actionManager->createAction("cut_selection_to_new_layer");
    connect(m_cutToNewLayer, SIGNAL(triggered()), this, SLOT(cutToNewLayer()));

    m_fillForegroundColor = actionManager->createAction("fill_selection_foreground_color");
    connect(m_fillForegroundColor, SIGNAL(triggered()), this, SLOT(fillForegroundColor()));

    m_fillBackgroundColor  = actionManager->createAction("fill_selection_background_color");
    connect(m_fillBackgroundColor, SIGNAL(triggered()), this, SLOT(fillBackgroundColor()));

    m_fillPattern = actionManager->createAction("fill_selection_pattern");
    connect(m_fillPattern, SIGNAL(triggered()), this, SLOT(fillPattern()));

    m_fillForegroundColorOpacity  = actionManager->createAction("fill_selection_foreground_color_opacity");
    connect(m_fillForegroundColorOpacity, SIGNAL(triggered()), this, SLOT(fillForegroundColorOpacity()));

    m_fillBackgroundColorOpacity  = actionManager->createAction("fill_selection_background_color_opacity");
    connect(m_fillBackgroundColorOpacity, SIGNAL(triggered()), this, SLOT(fillBackgroundColorOpacity()));

    m_fillPatternOpacity  = actionManager->createAction("fill_selection_pattern_opacity");
    connect(m_fillPatternOpacity, SIGNAL(triggered()), this, SLOT(fillPatternOpacity()));

    m_strokeShapes  = actionManager->createAction("stroke_shapes");
    connect(m_strokeShapes, SIGNAL(triggered()), this, SLOT(paintSelectedShapes()));

    m_toggleDisplaySelection  = actionManager->createAction("toggle_display_selection");
    connect(m_toggleDisplaySelection, SIGNAL(triggered()), this, SLOT(toggleDisplaySelection()));
    m_toggleDisplaySelection->setChecked(true);

    m_imageResizeToSelection  = actionManager->createAction("resizeimagetoselection");
    connect(m_imageResizeToSelection, SIGNAL(triggered()), this, SLOT(imageResizeToSelection()));

    action = actionManager->createAction("convert_to_vector_selection");
    connect(action, SIGNAL(triggered()), SLOT(convertToVectorSelection()));

    action = actionManager->createAction("convert_shapes_to_vector_selection");
    connect(action, SIGNAL(triggered()), SLOT(convertShapesToVectorSelection()));

    action = actionManager->createAction("convert_selection_to_shape");
    connect(action, SIGNAL(triggered()), SLOT(convertToShape()));

    m_toggleSelectionOverlayMode  = actionManager->createAction("toggle-selection-overlay-mode");
    connect(m_toggleSelectionOverlayMode, SIGNAL(triggered()), SLOT(slotToggleSelectionDecoration()));

    QClipboard *cb = QApplication::clipboard();
    connect(cb, SIGNAL(dataChanged()), SLOT(clipboardDataChanged()));
}


void KisSelectionManager::setView(QPointer<KisView>imageView)
{
    if (m_imageView && m_imageView->canvasBase()) {
        disconnect(m_imageView->canvasBase()->toolProxy(), SIGNAL(toolChanged(const QString&)), this, SLOT(clipboardDataChanged()));

        KoSelection *selection = m_imageView->canvasBase()->globalShapeManager()->selection();
        selection->disconnect(this, SLOT(shapeSelectionChanged()));
        KisSelectionDecoration *decoration = qobject_cast<KisSelectionDecoration*>(m_imageView->canvasBase()->decoration("selection"));
        if (decoration) {
            disconnect(SIGNAL(currentSelectionChanged()), decoration);
        }
        m_imageView->image()->undoAdapter()->disconnect(this);
        m_selectionDecoration = 0;
    }

    m_imageView = imageView;
    if (m_imageView) {
        KoSelection * selection = m_imageView->canvasBase()->globalShapeManager()->selection();
        Q_ASSERT(selection);
        connect(selection, SIGNAL(selectionChanged()), this, SLOT(shapeSelectionChanged()));

        KisSelectionDecoration* decoration = qobject_cast<KisSelectionDecoration*>(m_imageView->canvasBase()->decoration("selection"));
        if (!decoration) {
            decoration = new KisSelectionDecoration(m_imageView);
            decoration->setVisible(true);
            m_imageView->canvasBase()->addDecoration(decoration);
        }
        m_selectionDecoration = decoration;
        connect(this, SIGNAL(currentSelectionChanged()), decoration, SLOT(selectionChanged()));
        connect(m_imageView->image()->undoAdapter(), SIGNAL(selectionChanged()), SLOT(selectionChanged()));
        connect(m_imageView->canvasBase()->toolProxy(), SIGNAL(toolChanged(const QString&)), SLOT(clipboardDataChanged()));

    }
}


void KisSelectionManager::clipboardDataChanged()
{
    m_view->updateGUI();
}

bool KisSelectionManager::havePixelsSelected()
{
    KisSelectionSP activeSelection = m_view->selection();
    return activeSelection && !activeSelection->selectedRect().isEmpty();
}

bool KisSelectionManager::havePixelsInClipboard()
{
    return m_clipboard->hasClip();
}

bool KisSelectionManager::haveShapesSelected()
{
    if (m_view && m_view->canvasBase() && m_view->canvasBase()->shapeManager() && m_view->canvasBase()->shapeManager()->selection()->count()) {
        return m_view->canvasBase()->shapeManager()->selection()->count() > 0;
    }
    return false;
}

bool KisSelectionManager::haveShapesInClipboard()
{
    KisShapeLayer *shapeLayer =
        dynamic_cast<KisShapeLayer*>(m_view->activeLayer().data());

    if (shapeLayer) {
        const QMimeData* data = QApplication::clipboard()->mimeData();
        if (data) {
            QStringList mimeTypes = m_view->canvasBase()->toolProxy()->supportedPasteMimeTypes();
            Q_FOREACH (const QString & mimeType, mimeTypes) {
                if (data->hasFormat(mimeType)) {
                    return true;
                }
            }
        }
    }
    return false;
}

bool KisSelectionManager::havePixelSelectionWithPixels()
{
    KisSelectionSP selection = m_view->selection();
    if (selection && selection->hasPixelSelection()) {
        return !selection->pixelSelection()->selectedRect().isEmpty();
    }
    return false;
}

void KisSelectionManager::updateGUI()
{
    Q_ASSERT(m_view);
    Q_ASSERT(m_clipboard);
    if (!m_view || !m_clipboard) return;

    bool havePixelsSelected = this->havePixelsSelected();
    bool havePixelsInClipboard = this->havePixelsInClipboard();
    bool haveShapesSelected = this->haveShapesSelected();
    bool haveShapesInClipboard = this->haveShapesInClipboard();
    bool haveDevice = m_view->activeDevice();

    KisLayerSP activeLayer = m_view->activeLayer();
    KisImageWSP image = activeLayer ? activeLayer->image() : 0;
    bool canReselect = image && image->canReselectGlobalSelection();
    bool canDeselect =  image && image->globalSelection();

    m_clear->setEnabled(haveDevice || havePixelsSelected || haveShapesSelected);
    m_cut->setEnabled(havePixelsSelected || haveShapesSelected);
    m_copy->setEnabled(havePixelsSelected || haveShapesSelected);
    m_paste->setEnabled(havePixelsInClipboard || haveShapesInClipboard);
    m_pasteAt->setEnabled(havePixelsInClipboard || haveShapesInClipboard);
    // FIXME: how about pasting shapes?
    m_pasteNew->setEnabled(havePixelsInClipboard);

    m_selectAll->setEnabled(true);
    m_deselect->setEnabled(canDeselect);
    m_reselect->setEnabled(canReselect);

//    m_load->setEnabled(true);
//    m_save->setEnabled(havePixelsSelected);

    updateStatusBar();
    emit signalUpdateGUI();
}

void KisSelectionManager::updateStatusBar()
{
    if (m_view && m_view->statusBar()) {
        m_view->statusBar()->setSelection(m_view->image());
    }
}

void KisSelectionManager::selectionChanged()
{
    m_view->updateGUI();
    emit currentSelectionChanged();
}

void KisSelectionManager::cut()
{
    KisCutCopyActionFactory factory;
    factory.run(true, false, m_view);
}

void KisSelectionManager::copy()
{
    KisCutCopyActionFactory factory;
    factory.run(false, false, m_view);
}

void KisSelectionManager::cutSharp()
{
    KisCutCopyActionFactory factory;
    factory.run(true, true, m_view);
}

void KisSelectionManager::copySharp()
{
    KisCutCopyActionFactory factory;
    factory.run(false, true, m_view);
}

void KisSelectionManager::copyMerged()
{
    KisCopyMergedActionFactory factory;
    factory.run(m_view);
}

void KisSelectionManager::paste()
{
    KisPasteActionFactory factory;
    factory.run(m_view);
}

void KisSelectionManager::pasteAt()
{
    //XXX
}

void KisSelectionManager::pasteNew()
{
    KisPasteNewActionFactory factory;
    factory.run(m_view);
}

void KisSelectionManager::selectAll()
{
    KisSelectAllActionFactory factory;
    factory.run(m_view);
}

void KisSelectionManager::deselect()
{
    KisDeselectActionFactory factory;
    factory.run(m_view);
}

void KisSelectionManager::invert()
{
    if(m_invert)
        m_invert->trigger();
}

void KisSelectionManager::reselect()
{
    KisReselectActionFactory factory;
    factory.run(m_view);
}

void KisSelectionManager::convertToVectorSelection()
{
    KisSelectionToVectorActionFactory factory;
    factory.run(m_view);
}

void KisSelectionManager::convertShapesToVectorSelection()
{
    KisShapesToVectorSelectionActionFactory factory;
    factory.run(m_view);
}

void KisSelectionManager::convertToShape()
{
    KisSelectionToShapeActionFactory factory;
    factory.run(m_view);
}

void KisSelectionManager::clear()
{
    KisClearActionFactory factory;
    factory.run(m_view);
}

void KisSelectionManager::fillForegroundColor()
{
    KisFillActionFactory  factory;
    factory.run("fg", m_view);
}

void KisSelectionManager::fillBackgroundColor()
{
    KisFillActionFactory factory;
    factory.run("bg", m_view);
}

void KisSelectionManager::fillPattern()
{
    KisFillActionFactory factory;
    factory.run("pattern", m_view);
}

void KisSelectionManager::fillForegroundColorOpacity()
{
    KisFillActionFactory  factory;
    factory.run("fg_opacity", m_view);
}

void KisSelectionManager::fillBackgroundColorOpacity()
{
    KisFillActionFactory factory;
    factory.run("bg_opacity", m_view);
}

void KisSelectionManager::fillPatternOpacity()
{
    KisFillActionFactory factory;
    factory.run("pattern_opacity", m_view);
}

void KisSelectionManager::copySelectionToNewLayer()
{
    copy();
    paste();
}

void KisSelectionManager::cutToNewLayer()
{
    cut();
    paste();
}

void KisSelectionManager::toggleDisplaySelection()
{
    KIS_ASSERT_RECOVER_RETURN(m_selectionDecoration);

    m_selectionDecoration->toggleVisibility();
    m_toggleDisplaySelection->blockSignals(true);
    m_toggleDisplaySelection->setChecked(m_selectionDecoration->visible());
    m_toggleDisplaySelection->blockSignals(false);

    emit displaySelectionChanged();
}

bool KisSelectionManager::displaySelection()
{
    return m_toggleDisplaySelection->isChecked();
}

void KisSelectionManager::shapeSelectionChanged()
{
    KoShapeManager* shapeManager = m_view->canvasBase()->globalShapeManager();

    KoSelection * selection = shapeManager->selection();
    QList<KoShape*> selectedShapes = selection->selectedShapes();

    KoShapeStroke* border = new KoShapeStroke(0, Qt::lightGray);
    Q_FOREACH (KoShape* shape, shapeManager->shapes()) {
        if (dynamic_cast<KisShapeSelection*>(shape->parent())) {
            if (selectedShapes.contains(shape))
                shape->setStroke(border);
            else
                shape->setStroke(0);
        }
    }
    m_view->updateGUI();
}

void KisSelectionManager::imageResizeToSelection()
{
    KisImageResizeToSelectionActionFactory factory;
    factory.run(m_view);
}

void KisSelectionManager::paintSelectedShapes()
{
    KisImageWSP image = m_view->image();
    if (!image) return;

    KisLayerSP layer = m_view->activeLayer();
    if (!layer) return;

    QList<KoShape*> shapes = m_view->canvasBase()->shapeManager()->selection()->selectedShapes();

    KisPaintLayerSP paintLayer = new KisPaintLayer(image, i18n("Stroked Shapes"), OPACITY_OPAQUE_U8);

    KUndo2MagicString actionName = kundo2_i18n("Stroke Shapes");

    m_adapter->beginMacro(actionName);
    m_adapter->addNode(paintLayer.data(), layer->parent().data(), layer.data());

    KisFigurePaintingToolHelper helper(actionName,
                                       image,
                                       paintLayer.data(),
                                       m_view->resourceProvider()->resourceManager(),
                                       KisPainter::StrokeStyleBrush,
                                       KisPainter::FillStyleNone);

    Q_FOREACH (KoShape* shape, shapes) {
        QTransform matrix = shape->absoluteTransformation(0) * QTransform::fromScale(image->xRes(), image->yRes());
        QPainterPath mapedOutline = matrix.map(shape->outline());
        helper.paintPainterPath(mapedOutline);
    }
    m_adapter->endMacro();
}

void KisSelectionManager::slotToggleSelectionDecoration()
{
    KIS_ASSERT_RECOVER_RETURN(m_selectionDecoration);

    KisSelectionDecoration::Mode mode =
        m_selectionDecoration->mode() ?
        KisSelectionDecoration::Ants : KisSelectionDecoration::Mask;

    m_selectionDecoration->setMode(mode);
    emit displaySelectionChanged();
}

bool KisSelectionManager::showSelectionAsMask() const
{
    if (m_selectionDecoration) {
        return m_selectionDecoration->mode() == KisSelectionDecoration::Mask;
    }
    return false;
}

