/*
 *  Copyright (c) 2011 José Luis Vergara <pentalis@gmail.com>
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

#include "kis_preset_selector_strip.h"

#include "KoResourceModel.h"
#include "KoResourceItemView.h"
#include "KoResourceItemChooser.h"
#include "kis_paintop_registry.h"

#include <kis_icon.h>

#include <QAbstractScrollArea>
#include <QMouseEvent>

KisPresetSelectorStrip::KisPresetSelectorStrip(QWidget* parent)
    : QWidget(parent)
{
    setupUi(this);
    smallPresetChooser->showButtons(false);
    smallPresetChooser->setViewMode(KisPresetChooser::STRIP);
    m_resourceItemView = smallPresetChooser->itemChooser()->itemView();
    m_resourceItemView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_resourceItemView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    
    /* This is an heuristic to fill smallPresetChooser with only the presets
     * for the paintop that comes selected by default: Pixel Brush. */
    const QString PIXEL_BRUSH_ID = "paintbrush";
    m_currentPaintopID = PIXEL_BRUSH_ID;
}

KisPresetSelectorStrip::~KisPresetSelectorStrip()
{
}

void KisPresetSelectorStrip::setPresetFilter(const QString& paintOpId)
{
    smallPresetChooser->setPresetFilter(paintOpId);
    if (m_currentPaintopID != paintOpId) {
        m_resourceItemView->scrollTo(m_resourceItemView->model()->index(0, 0));
        m_currentPaintopID = paintOpId;
    }
}

void KisPresetSelectorStrip::on_leftScrollBtn_pressed()
{
    // Deciding how far beyond the left margin (10 pixels) was an arbitrary decision
    QPoint beyondLeftMargin(-10, 0);
    m_resourceItemView->scrollTo(m_resourceItemView->indexAt(beyondLeftMargin), QAbstractItemView::EnsureVisible);
}

void KisPresetSelectorStrip::on_rightScrollBtn_pressed()
{
    // Deciding how far beyond the right margin to put the point (10 pixels) was an arbitrary decision
    QPoint beyondRightMargin(10 + m_resourceItemView->viewport()->width(), 0);
    m_resourceItemView->scrollTo(m_resourceItemView->indexAt(beyondRightMargin), QAbstractItemView::EnsureVisible);
}

