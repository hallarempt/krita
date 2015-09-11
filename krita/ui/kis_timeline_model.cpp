/*
 *  Copyright (c) 2015 Jouni Pentikäinen <joupent@gmail.com>
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

#include "kis_timeline_model.h"
#include "kis_model_index_converter_animated_layers.h"
#include "kis_node_dummies_graph.h"
#include "kis_keyframe_channel.h"
#include "kis_dummies_facade_base.h"
#include "kundo2command.h"
#include "kis_post_execution_undo_adapter.h"
#include "kis_image_animation_interface.h"
#include "kis_node.h"


struct KisTimelineModel::Private
{
    Private();

    bool skipNoopMove;
    KisImageWSP image;
};

KisTimelineModel::Private::Private()
    : skipNoopMove(false)
{
}

KisTimelineModel::KisTimelineModel(QObject *parent)
    : KisNodeModel(parent)
    , m_d(new Private())
{
}

KisTimelineModel::~KisTimelineModel()
{}

void KisTimelineModel::beginMacro(const KUndo2MagicString& macroName)
{
    KisImageSP image = m_d->image;

    if (image) {
        image->undoAdapter()->beginMacro(macroName);
    }
}

void KisTimelineModel::endMacro()
{
    KisImageSP image = m_d->image;

    if (image) {
        image->undoAdapter()->endMacro();
    }
}

int KisTimelineModel::totalLength()
{
    if (!m_d->image || !m_d->image->animationInterface()) return 0;

   return m_d->image->animationInterface()->totalLength();
}

void KisTimelineModel::setDummiesFacade(KisDummiesFacadeBase *newDummiesFacade, KisImageWSP image, KisShapeController *shapeController)
{
    if(dummiesFacade()) {
        dummiesFacade()->disconnect(this);

        connectAllNodes(dummiesFacade()->rootDummy(), false);
    }

    m_d->image = image;
    KisNodeModel::setDummiesFacade(newDummiesFacade, image, shapeController);

    if (newDummiesFacade) {
        connectAllNodes(dummiesFacade()->rootDummy(), true);
        connect(dummiesFacade(), SIGNAL(sigEndInsertDummy(KisNodeDummy*)), SLOT(slotEndInsertDummy2(KisNodeDummy*)));
        connect(dummiesFacade(), SIGNAL(sigBeginRemoveDummy(KisNodeDummy*)), SLOT(slotBeginRemoveDummy2(KisNodeDummy*)));
    }
}

void KisTimelineModel::connectAllNodes(KisNodeDummy *nodeDummy, bool needConnect) {
    connectChannels(nodeDummy->node(), needConnect);

    nodeDummy = nodeDummy->firstChild();
    while(nodeDummy) {
        connectAllNodes(nodeDummy, needConnect);
        nodeDummy = nodeDummy->nextSibling();
    }
}

void KisTimelineModel::connectChannels(KisNodeSP node, bool needConnect) {
    if (needConnect) {
        connect(node.data(), SIGNAL(animatedAboutToChange(bool)), this, SLOT(slotNodeAnimatedAboutToChange(bool)));
        connect(node.data(), SIGNAL(animatedChanged(bool)), this, SLOT(slotNodeAnimatedChanged(bool)));
    } else {
        disconnect(node.data(), 0, this, 0);
    }

    QList<KisKeyframeChannel*> channels = node->keyframeChannels();

    foreach (KisKeyframeChannel *channel, channels) {
        if (needConnect) {
            connect(channel, SIGNAL(sigKeyframeAboutToBeAdded(KisKeyframe*)), this, SLOT(slotKeyframeAboutToBeAdded(KisKeyframe*)));
            connect(channel, SIGNAL(sigKeyframeAdded(KisKeyframe*)), this, SLOT(slotKeyframeAdded(KisKeyframe*)));
            connect(channel, SIGNAL(sigKeyframeAboutToBeRemoved(KisKeyframe*)), this, SLOT(slotKeyframeAboutToBeRemoved(KisKeyframe*)));
            connect(channel, SIGNAL(sigKeyframeRemoved(KisKeyframe*)), this, SLOT(slotKeyframeRemoved(KisKeyframe*)));
            connect(channel, SIGNAL(sigKeyframeAboutToBeMoved(KisKeyframe*,int)), this, SLOT(slotKeyframeAboutToBeMoved(KisKeyframe*,int)));
            connect(channel, SIGNAL(sigKeyframeMoved(KisKeyframe*, int)), this, SLOT(slotKeyframeMoved(KisKeyframe*)));
        } else {
            disconnect(channel, 0, this, 0);
        }
    }
}

QModelIndex KisTimelineModel::index(int row, int column, const QModelIndex &parent) const
{
    QObject *parentObj = static_cast<QObject*>(parent.internalPointer());

    if (!parentObj) {
        if (column == 0) {
            return KisNodeModel::index(row, column, parent);
        } else {
            return QModelIndex();
        }
    } else if (parentObj->inherits("KisKeyframe")) {
        return QModelIndex();
    } else if (parentObj->inherits("KisKeyframeChannel")) {
        if (parent.column() == 0) {
            return QModelIndex();
        } else {
            KisKeyframeChannel *channel = qobject_cast<KisKeyframeChannel*>(parentObj);
            if (row >= channel->keyframeCount()) return QModelIndex();
            KisKeyframe *keyframe = channel->keyframeAtRow(row);
            return createIndex(row, column, keyframe);
        }
    } else { // KisNodeDummy
        int childNodeCount = indexConverter()->rowCount(parent);
        if (row >= childNodeCount) {
            int channelIndex = row - childNodeCount;

            KisNodeSP parentNode = nodeFromIndex(parent);
            KisKeyframeChannel *channel = parentNode->keyframeChannels().at(channelIndex);

            return createIndex(row, column, channel);
        }

        if (column == 0) {
            return KisNodeModel::index(row, column, parent);
        } else {
            return QModelIndex();
        }
    }
}

QModelIndex KisTimelineModel::parent(const QModelIndex &child) const
{
    QObject *childObj = static_cast<QObject*>(child.internalPointer());

    if (!childObj) {
        return QModelIndex();
    } else if (childObj->inherits("KisKeyframe")) {
        KisKeyframe *keyframe = qobject_cast<KisKeyframe*>(childObj);
        KisKeyframeChannel *channel = keyframe->channel();
        return getChannelIndex(channel, 1);
    } else if (childObj->inherits("KisKeyframeChannel")) {
        KisKeyframeChannel *channel = qobject_cast<KisKeyframeChannel*>(childObj);

        KisNodeWSP node = channel->node();

        return KisNodeModel::indexFromNode(node);
    } else { // KisNodeDummy
        return KisNodeModel::parent(child);
    }
}

int KisTimelineModel::rowCount(const QModelIndex &parent) const
{
    QObject *parentObj = static_cast<QObject*>(parent.internalPointer());

    if (!parentObj) {
        return KisNodeModel::rowCount(parent);
    } else if (parentObj->inherits("KisKeyframe")) {
        return 0;
    } else if (parentObj->inherits("KisKeyframeChannel")) {
        if (parent.column() == 0) {
            return 0;
        } else {
            KisKeyframeChannel *channel = qobject_cast<KisKeyframeChannel*>(parentObj);
            return channel->keyframeCount();
        }
    } else { // KisNodeDummy
        KisNodeSP parentNode = nodeFromIndex(parent);
        int channelCount = parentNode->keyframeChannels().count();
        return KisNodeModel::rowCount(parent) + channelCount;
    }
}

int KisTimelineModel::columnCount(const QModelIndex &parent) const
{
    QObject *parentObj = static_cast<QObject*>(parent.internalPointer());

    if (!parentObj) {
        return 2;
    } else if (parentObj->inherits("KisKeyframe")) {
        return 0;
    } else if (parentObj && parentObj->inherits("KisKeyframeChannel")) {
        return  (parent.column() == 0) ? 0 : 1;
    } else { // KisNodeDummy
        return 2;
    }
}

QVariant KisTimelineModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::SizeHintRole) return QSize(42, 16); // TODO: good values?

    QObject *obj = static_cast<QObject*>(index.internalPointer());

    if (obj->inherits("KisKeyframe")) {
        KisKeyframe *keyframe = qobject_cast<KisKeyframe*>(obj);
        switch (role) {
        case KisTimelineModel::TimeRole: return keyframe->time();
        }
        return QVariant();
    } else if (obj->inherits("KisKeyframeChannel")) {
        KisKeyframeChannel *channel = qobject_cast<KisKeyframeChannel*>(obj);
        switch (role) {
        case Qt::DisplayRole: return channel->name();
        }
        return QVariant();
    } else { // KisNodeDummy
        return KisNodeModel::data(index, role);
    }
}

bool KisTimelineModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    QObject *obj = static_cast<QObject*>(index.internalPointer());
    if (!obj) return false;

    if (obj->inherits("KisKeyframe")) {
        KisKeyframe *keyframe = qobject_cast<KisKeyframe*>(obj);

        if (role == KisTimelineModel::TimeRole) {
            bool result = false;

            QScopedPointer<KUndo2Command> cmd(new KUndo2Command(kundo2_i18n("Move Keyframe")));
            result = keyframe->channel()->moveKeyframe(keyframe, value.toInt(), cmd.data());
            if (result) {
                m_d->image->postExecutionUndoAdapter()->addCommand(toQShared(cmd.take()));
            }

            emit dataChanged(index, index);

            return result;
        }
    }

    return false;
}

Qt::ItemFlags KisTimelineModel::flags(const QModelIndex &index) const
{
    if (!index.isValid()) return Qt::NoItemFlags;

    QObject *obj = static_cast<QObject*>(index.internalPointer());

    if (obj->inherits("KisKeyframe")) {
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    } else if (obj->inherits("KisKeyframeChannel")) {
        return Qt::ItemIsEnabled;
    } else {
        return Qt::ItemIsEnabled;
    }
}

KisModelIndexConverterBase *KisTimelineModel::createIndexConverter()
{
    return new KisModelIndexConverterAnimatedLayers(dummiesFacade(), this);
}

void KisTimelineModel::slotEndInsertDummy2(KisNodeDummy *dummy)
{
    connectChannels(dummy->node(), true);
}

void KisTimelineModel::slotBeginRemoveDummy2(KisNodeDummy *dummy)
{
    connectChannels(dummy->node(), true);
}

void KisTimelineModel::slotNodeAnimatedAboutToChange(bool animated)
{
    if (animated) {
        KisNode *node = qobject_cast<KisNode*>(QObject::sender());
        KisNodeDummy *parentDummy = dummiesFacade()->dummyForNode(node->parent());
        int index = node->parent()->index(node);

        slotBeginInsertDummy(parentDummy, index, "");
    }
}

void KisTimelineModel::slotNodeAnimatedChanged(bool animated)
{
    if (animated) {
        KisNode *node = qobject_cast<KisNode*>(QObject::sender());
        KisNodeDummy *dummy = dummiesFacade()->dummyForNode(node);

        slotEndInsertDummy(dummy);
    }
}

void KisTimelineModel::slotKeyframeAboutToBeAdded(KisKeyframe *keyframe)
{
    QModelIndex parent = getChannelIndex(keyframe->channel(), 1);
    int row = getInsertionPointByTime(keyframe->channel(), keyframe->time());
    beginInsertRows(parent, row, row);
}

void KisTimelineModel::slotKeyframeAdded(KisKeyframe *keyframe)
{
    Q_UNUSED(keyframe);
    endInsertRows();
}

void KisTimelineModel::slotKeyframeAboutToBeRemoved(KisKeyframe *keyframe)
{
    QModelIndex parent = getChannelIndex(keyframe->channel(), 1);

    int row = keyframe->channel()->keyframeRowIndexOf(keyframe);

    beginRemoveRows(parent, row, row);
}

void KisTimelineModel::slotKeyframeRemoved(KisKeyframe *keyframe)
{
    Q_UNUSED(keyframe);
    endRemoveRows();
}

void KisTimelineModel::slotKeyframeAboutToBeMoved(KisKeyframe *keyframe, int toTime)
{
    QModelIndex parent = getChannelIndex(keyframe->channel(), 1);

    int rowFrom = keyframe->channel()->keyframeRowIndexOf(keyframe);
    int rowTo = getInsertionPointByTime(keyframe->channel(), toTime);

    if (rowTo == rowFrom || rowTo == rowFrom + 1) {
        // Qt doesn't like NOP moves
        m_d->skipNoopMove = true;
        return;
    }

    beginMoveRows(parent, rowFrom, rowFrom, parent, rowTo);
}

void KisTimelineModel::slotKeyframeMoved(KisKeyframe *keyframe)
{
    Q_UNUSED(keyframe);

    if (!m_d->skipNoopMove) {
        endMoveRows();
    }

    m_d->skipNoopMove = false;
}

int KisTimelineModel::getInsertionPointByTime(KisKeyframeChannel *channel, int time)
{
    return channel->keyframeInsertionRow(time);
}

QModelIndex KisTimelineModel::getChannelIndex(KisKeyframeChannel *channel, int column) const
{
    KisNodeWSP node = channel->node();
    int row = node->childCount() + node->keyframeChannels().indexOf(channel);
    return createIndex(row, column, channel);
}

#include "kis_timeline_model.moc"