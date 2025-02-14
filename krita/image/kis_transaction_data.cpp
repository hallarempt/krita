/*
 *  Copyright (c) 2002 Patrick Julien <freak@codepimps.org>
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

#include "kis_transaction_data.h"

#include "kis_pixel_selection.h"
#include "kis_paint_device.h"
#include "kis_paint_device_frames_interface.h"
#include "kis_datamanager.h"
#include "kis_image.h"

#include <config-tiles.h> // For the next define

//#define DEBUG_TRANSACTIONS

#ifdef DEBUG_TRANSACTIONS
#    define DEBUG_ACTION(action) dbgKrita << action << "for" << m_d->device->dataManager()
#else
#    define DEBUG_ACTION(action)
#endif


class Q_DECL_HIDDEN KisTransactionData::Private
{
public:
    KisPaintDeviceSP device;
    KisMementoSP memento;
    bool firstRedo;
    bool transactionFinished;
    QPoint oldOffset;
    QPoint newOffset;

    bool savedOutlineCacheValid;
    QPainterPath savedOutlineCache;
    KUndo2Command *flattenUndoCommand;
    bool resetSelectionOutlineCache;

    int transactionTime;
    int transactionFrameId;
    KisDataManagerSP savedDataManager;

    KUndo2Command newFrameCommand;

    void possiblySwitchCurrentTime();
    KisDataManagerSP dataManager();
    void moveDevice(const QPoint newOffset);

    void tryCreateNewFrame(KisPaintDeviceSP device, int time);
};


KisTransactionData::KisTransactionData(const KUndo2MagicString& name, KisPaintDeviceSP device, bool resetSelectionOutlineCache, KUndo2Command* parent)
    : KUndo2Command(name, parent)

    , m_d(new Private())
{
    m_d->resetSelectionOutlineCache = resetSelectionOutlineCache;
    setTimedID(-1);
    init(device);
    saveSelectionOutlineCache();
}

#include "kis_raster_keyframe_channel.h"
#include "kis_image_config.h"

void KisTransactionData::Private::tryCreateNewFrame(KisPaintDeviceSP device, int time)
{
    if (!device->framesInterface()) return;

    KisImageConfig cfg(true);
    if (!cfg.lazyFrameCreationEnabled()) return;

    KisRasterKeyframeChannel *channel = device->keyframeChannel();
    KIS_ASSERT_RECOVER(channel) { return; }

    KisKeyframeSP keyframe = channel->keyframeAt(time);

    if (!keyframe) {
        keyframe = channel->activeKeyframeAt(time);
        channel->copyKeyframe(keyframe, time, &newFrameCommand);
    }
}

void KisTransactionData::init(KisPaintDeviceSP device)
{
    m_d->device = device;
    DEBUG_ACTION("Transaction started");

    m_d->oldOffset = QPoint(device->x(), device->y());
    m_d->firstRedo = true;
    m_d->transactionFinished = false;
    m_d->flattenUndoCommand = 0;

    m_d->transactionTime = device->defaultBounds()->currentTime();

    m_d->tryCreateNewFrame(m_d->device, m_d->transactionTime);

    m_d->transactionFrameId = device->framesInterface() ? device->framesInterface()->currentFrameId() : -1;
    m_d->savedDataManager = m_d->transactionFrameId >= 0 ?
        m_d->device->framesInterface()->frameDataManager(m_d->transactionFrameId) :
        m_d->device->dataManager();
    m_d->memento = m_d->savedDataManager->getMemento();
}

KisTransactionData::~KisTransactionData()
{
    Q_ASSERT(m_d->memento);
    m_d->savedDataManager->purgeHistory(m_d->memento);

    delete m_d;
}

void KisTransactionData::Private::moveDevice(const QPoint newOffset)
{
    if (transactionFrameId >= 0) {
        device->framesInterface()->setFrameOffset(transactionFrameId, newOffset);
    } else {
        device->move(newOffset);
    }
}

void KisTransactionData::endTransaction()
{
    if(!m_d->transactionFinished) {
        // make sure the time didn't change during the transaction
        KIS_ASSERT_RECOVER_RETURN(
            m_d->transactionTime == m_d->device->defaultBounds()->currentTime());

        DEBUG_ACTION("Transaction ended");
        m_d->transactionFinished = true;
        m_d->savedDataManager->commit();
        m_d->newOffset = QPoint(m_d->device->x(), m_d->device->y());
    }
}

void KisTransactionData::startUpdates()
{
    if (m_d->transactionFrameId == -1 ||
        m_d->transactionFrameId ==
        m_d->device->framesInterface()->currentFrameId()) {

        QRect rc;
        QRect mementoExtent = m_d->memento->extent();

        if (m_d->newOffset == m_d->oldOffset) {
            rc = mementoExtent.translated(m_d->device->x(), m_d->device->y());
        } else {
            QRect totalExtent =
                m_d->savedDataManager->extent() | mementoExtent;

            rc = totalExtent.translated(m_d->oldOffset) |
                totalExtent.translated(m_d->newOffset);
        }

        m_d->device->setDirty(rc);
    } else {
        m_d->device->framesInterface()->invalidateFrameCache(m_d->transactionFrameId);
    }
}

void KisTransactionData::possiblyNotifySelectionChanged()
{
    KisPixelSelectionSP pixelSelection =
        dynamic_cast<KisPixelSelection*>(m_d->device.data());

    KisSelectionSP selection;
    if (pixelSelection && (selection = pixelSelection->parentSelection())) {
        selection->notifySelectionChanged();
    }
}

void KisTransactionData::possiblyResetOutlineCache()
{
    KisPixelSelectionSP pixelSelection;

    if (m_d->resetSelectionOutlineCache &&
        (pixelSelection =
         dynamic_cast<KisPixelSelection*>(m_d->device.data()))) {

        pixelSelection->invalidateOutlineCache();
    }
}

void KisTransactionData::Private::possiblySwitchCurrentTime()
{
    if (device->defaultBounds()->currentTime() == transactionTime) return;

    qWarning() << "WARNING: undo command has been executed, when another frame has been active. That shouldn't have happened.";
    device->requestTimeSwitch(transactionTime);
}

void KisTransactionData::redo()
{
    //KUndo2QStack calls redo(), so the first call needs to be blocked
    if (m_d->firstRedo) {
        m_d->firstRedo = false;


        possiblyResetOutlineCache();
        possiblyNotifySelectionChanged();
        return;
    }


    restoreSelectionOutlineCache(false);

    m_d->newFrameCommand.redo();

    DEBUG_ACTION("Redo()");

    Q_ASSERT(m_d->memento);
    m_d->savedDataManager->rollforward(m_d->memento);

    if (m_d->newOffset != m_d->oldOffset) {
        m_d->moveDevice(m_d->newOffset);
    }

    m_d->possiblySwitchCurrentTime();
    startUpdates();
    possiblyNotifySelectionChanged();
}

void KisTransactionData::undo()
{
    DEBUG_ACTION("Undo()");
    Q_ASSERT(m_d->memento);
    m_d->savedDataManager->rollback(m_d->memento);

    if (m_d->newOffset != m_d->oldOffset) {
        m_d->moveDevice(m_d->oldOffset);
    }

    restoreSelectionOutlineCache(true);

    m_d->possiblySwitchCurrentTime();
    startUpdates();
    possiblyNotifySelectionChanged();

    m_d->newFrameCommand.undo();
}

void KisTransactionData::saveSelectionOutlineCache()
{
    m_d->savedOutlineCacheValid = false;

    KisPixelSelectionSP pixelSelection =
        dynamic_cast<KisPixelSelection*>(m_d->device.data());

    if (pixelSelection) {
        m_d->savedOutlineCacheValid = pixelSelection->outlineCacheValid();
        if (m_d->savedOutlineCacheValid) {
            m_d->savedOutlineCache = pixelSelection->outlineCache();

            possiblyResetOutlineCache();
        }

        KisSelectionSP selection = pixelSelection->parentSelection();
        if (selection) {
            m_d->flattenUndoCommand = selection->flatten();
            if (m_d->flattenUndoCommand) {
                m_d->flattenUndoCommand->redo();
            }
        }
    }
}

void KisTransactionData::restoreSelectionOutlineCache(bool undo)
{
    KisPixelSelectionSP pixelSelection =
        dynamic_cast<KisPixelSelection*>(m_d->device.data());

    if (pixelSelection) {
        bool savedOutlineCacheValid;
        QPainterPath savedOutlineCache;

        savedOutlineCacheValid = pixelSelection->outlineCacheValid();
        if (savedOutlineCacheValid) {
            savedOutlineCache = pixelSelection->outlineCache();
        }

        if (m_d->savedOutlineCacheValid) {
            pixelSelection->setOutlineCache(m_d->savedOutlineCache);
        } else {
            pixelSelection->invalidateOutlineCache();
        }

        m_d->savedOutlineCacheValid = savedOutlineCacheValid;
        if (m_d->savedOutlineCacheValid) {
            m_d->savedOutlineCache = savedOutlineCache;
        }

        if (m_d->flattenUndoCommand) {
            if (undo) {
                m_d->flattenUndoCommand->undo();
            } else {
                m_d->flattenUndoCommand->redo();
            }
        }
    }
}
