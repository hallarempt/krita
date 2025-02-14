/*
 *  kis_paintop_box.h - part of KImageShop/Krayon/Krita
 *
 *  Copyright (c) 2004-2008 Boudewijn Rempt (boud@valdyas.org)
 *  Copyright (C) 2011      Silvio Heinrich <plassy@web.de>
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

#ifndef KIS_PAINTOP_BOX_H_
#define KIS_PAINTOP_BOX_H_

#include <QMap>
#include <QWidget>
#include <QList>


#include <KoID.h>
#include <KoInputDevice.h>

#include <kis_types.h>
#include <kis_paintop_settings.h>

#include "kis_locked_properties_proxy.h"
#include "kis_locked_properties_server.h"
#include "kis_locked_properties.h"
#include "kritaui_export.h"


class QToolButton;
class QString;
class QHBoxLayout;

class KoColorSpace;
class KoResource;
class KoCanvasController;

class KisViewManager;
class KisCanvasResourceProvider;
class KisPopupButton;
class KisToolOptionsPopup;
class KisPaintOpPresetsPopup;
class KisPaintOpPresetsChooserPopup;
class KisPaintOpConfigWidget;
class KisCompositeOpComboBox;
class KisWidgetChooser;
class KisFavoriteResourceManager;
class KisAction;

/**
 * This widget presents all paintops that a user can paint with.
 * Paintops represent real-world tools or the well-known Shoup
 * computer equivalents that do nothing but change color.
 *
 * To incorporate the dirty preset functionality and locked settings
 * the following slots are added
 *  void slotReloadPreset();
    void slotConfigurationItemChanged();
    void slotSaveLockedOptionToPreset(KisPropertiesConfiguration* p);
    void slotDropLockedOption(KisPropertiesConfiguration* p);
    void slotDirtyPresetToggled(bool);
    Everytime a value is changed in a preset, the preset is made dirty through the onChange() function.
    For Locked Settings however, a changed Locked Setting will not cause a preset to become dirty. That is
    becuase it borrows its values from the KisLockedPropertiesServer.
    Hence the dirty state of the Preset is kept consistent before and after a writeConfiguration operation in  most cases.
 * XXX: When we have a lot of paintops, replace the listbox
 * with a table, and for every category a combobox.
 *
 * XXX: instead of text, use pretty pictures.
 */
class KRITAUI_EXPORT KisPaintopBox : public QWidget
{
    Q_OBJECT

    enum {
        ENABLE_PRESETS      = 0x0001,
        DISABLE_PRESETS     = 0x0002,
        ENABLE_COMPOSITEOP  = 0x0004,
        DISABLE_COMPOSITEOP = 0x0008,
        ENABLE_OPACITY      = 0x0010,
        DISABLE_OPACITY     = 0x0020,
        ENABLE_FLOW         = 0x0040,
        DISABLE_FLOW        = 0x0080,
        ENABLE_SIZE         = 0x0100,
        DISABLE_SIZE        = 0x0200,
        ENABLE_ALL          = 0x5555,
        DISABLE_ALL         = 0xAAAA
    };

public:

    KisPaintopBox(KisViewManager* view, QWidget* parent, const char* name);
    ~KisPaintopBox();

    void restoreResource(KoResource* resource);
    /**
     * Update the option widgets to the argument ones, removing the currently set widgets.
     */
    void newOptionWidgets(const QList<QPointer<QWidget> > & optionWidgetList);

public Q_SLOTS:

    void slotColorSpaceChanged(const KoColorSpace* colorSpace);
    void slotInputDeviceChanged(const KoInputDevice & inputDevice);
    void slotCanvasResourceChanged(int key, const QVariant& v);
    void resourceSelected(KoResource* resource);

    KisFavoriteResourceManager *favoriteResourcesManager() { return m_favoriteResourceManager; }

private:

    KoID currentPaintop();

    void setCurrentPaintop(const KoID& paintop, KisPaintOpPresetSP preset = 0);
    void setCurrentPaintopAndReload(const KoID& paintop, KisPaintOpPresetSP preset);

    QPixmap paintopPixmap(const KoID& paintop);
    KoID defaultPaintOp();
    KisPaintOpPresetSP defaultPreset(const KoID& paintOp);
    KisPaintOpPresetSP activePreset(const KoID& paintOp);
    void updateCompositeOp(QString compositeOpID, bool localUpdate = false);
    void setWidgetState(int flags);
    void setSliderValue(const QString& sliderID, qreal value);
    void sliderChanged(int n);

private Q_SLOTS:

    void slotSaveActivePreset();
    void slotUpdatePreset();
    void slotSetupDefaultPreset();
    void slotNodeChanged(const KisNodeSP node);
    void slotToggleEraseMode(bool checked);
    void slotSetCompositeMode(int index);
    void slotSetPaintop(const QString& paintOpId);
    void slotHorizontalMirrorChanged(bool value);
    void slotVerticalMirrorChanged(bool value);
    void slotSlider1Changed();
    void slotSlider2Changed();
    void slotSlider3Changed();
    void slotToolChanged(KoCanvasController* canvas, int toolId);
    void slotOpacityChanged(qreal);
    void slotPreviousFavoritePreset();
    void slotNextFavoritePreset();
    void slotSwitchToPreviousPreset();
    void slotUnsetEraseMode();
    void slotToggleAlphaLockMode(bool);

    void toggleHighlightedButton(QToolButton* m_tool);

    void slotReloadPreset();
    void slotConfigurationItemChanged();
    void slotSaveLockedOptionToPreset(KisPropertiesConfiguration* p);
    void slotDropLockedOption(KisPropertiesConfiguration* p);
    void slotDirtyPresetToggled(bool);
    void slotEraserBrushSizeToggled(bool);    
    void slotUpdateSelectionIcon();

private:
    KisCanvasResourceProvider*          m_resourceProvider;
    QHBoxLayout*                        m_layout;
    QWidget*                            m_paintopWidget;
    KisPaintOpConfigWidget*             m_optionWidget;
    KisPopupButton*                     m_toolOptionsPopupButton;
    KisPopupButton*                     m_brushEditorPopupButton;
    KisPopupButton*                     m_presetSelectorPopupButton;
    KisCompositeOpComboBox*             m_cmbCompositeOp;
    QToolButton*                        m_eraseModeButton;
    QToolButton*                        m_alphaLockButton;
    QToolButton*                        m_hMirrorButton;
    QToolButton*                        m_vMirrorButton;
    KisToolOptionsPopup*                m_toolOptionsPopup;
    KisPaintOpPresetsPopup*             m_presetsPopup;
    KisPaintOpPresetsChooserPopup*      m_presetsChooserPopup;
    KisViewManager*                     m_viewManager;
    KisPopupButton*                     m_workspaceWidget;
    KisWidgetChooser*                   m_sliderChooser[3];
    QMap<KoID, KisPaintOpConfigWidget*> m_paintopOptionWidgets;
    KisFavoriteResourceManager*         m_favoriteResourceManager;
    QToolButton*                        m_reloadButton;
    KisAction*                          m_eraseAction;
    KisAction*                          m_reloadAction;

    QString             m_prevCompositeOpID;
    QString             m_currCompositeOpID;
    KisNodeWSP          m_previousNode;

    qreal normalBrushSize; // when toggling between eraser mode
    qreal eraserBrushSize;

    KisAction* m_hMirrorAction;
    KisAction* m_vMirrorAction;

    struct TabletToolID {
        TabletToolID(const KoInputDevice& dev) {
            uniqueID = dev.uniqueTabletId();
            // Only the eraser is special, and we don't look at Cursor
            pointer = QTabletEvent::Pen;
            if (dev.pointer() == QTabletEvent::Eraser) {
                pointer = QTabletEvent::Eraser;
            }
        }

        bool operator == (const TabletToolID& id) const {
            return pointer == id.pointer && uniqueID == id.uniqueID;
        }

        bool operator < (const TabletToolID& id) const {
            if (uniqueID == id.uniqueID)
                return pointer < id.pointer;
            return uniqueID < id.uniqueID;
        }

        QTabletEvent::PointerType  pointer;
        qint64                     uniqueID;
    };

    struct TabletToolData {
        KoID               paintOpID;
        KisPaintOpPresetSP preset;
    };

    typedef QMap<TabletToolID, TabletToolData> TabletToolMap;
    typedef QMap<KoID, KisPaintOpPresetSP>     PaintOpPresetMap;

    TabletToolMap    m_tabletToolMap;
    PaintOpPresetMap m_paintOpPresetMap;
    TabletToolID     m_currTabletToolID;
    bool             m_presetsEnabled;
    bool             m_blockUpdate;
    bool             m_dirtyPresetsEnabled;
    bool             m_eraserBrushSizeEnabled;


};

#endif //KIS_PAINTOP_BOX_H_
