/* This file is part of the KDE project
 * Copyright (C) 2006,2008 Jan Hambrecht <jaham@gmx.net>
 * Copyright (C) 2006,2007 Thorsten Zachmann <zachmann@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "KoPathCombineCommand.h"
#include "KoShapeBasedDocumentBase.h"
#include "KoShapeContainer.h"
#include <klocalizedstring.h>

class Q_DECL_HIDDEN KoPathCombineCommand::Private
{
public:
    Private(KoShapeBasedDocumentBase *c, const QList<KoPathShape*> &p)
        : controller(c), paths(p)
        , combinedPath(0), combinedPathParent(0)
        , isCombined(false)
    {
        foreach (KoPathShape * path, paths)
            oldParents.append(path->parent());
    }
    ~Private() {
        if (isCombined && controller) {
            Q_FOREACH (KoPathShape* path, paths)
                delete path;
        } else
            delete combinedPath;
    }

    KoShapeBasedDocumentBase *controller;
    QList<KoPathShape*> paths;
    QList<KoShapeContainer*> oldParents;
    KoPathShape *combinedPath;
    KoShapeContainer *combinedPathParent;
    bool isCombined;
};

KoPathCombineCommand::KoPathCombineCommand(KoShapeBasedDocumentBase *controller,
        const QList<KoPathShape*> &paths, KUndo2Command *parent)
: KUndo2Command(parent)
, d(new Private(controller, paths))
{
    setText(kundo2_i18n("Combine paths"));

    d->combinedPath = new KoPathShape();
    d->combinedPath->setStroke(d->paths.first()->stroke());
    d->combinedPath->setShapeId(d->paths.first()->shapeId());
    // combine the paths
    Q_FOREACH (KoPathShape* path, d->paths) {
        d->combinedPath->combine(path);
        if (! d->combinedPathParent && path->parent())
            d->combinedPathParent = path->parent();
    }
}

KoPathCombineCommand::~KoPathCombineCommand()
{
    delete d;
}

void KoPathCombineCommand::redo()
{
    KUndo2Command::redo();

    if (! d->paths.size())
        return;

    d->isCombined = true;

    if (d->controller) {
        QList<KoShapeContainer*>::iterator parentIt = d->oldParents.begin();
        Q_FOREACH (KoPathShape* p, d->paths) {
            d->controller->removeShape(p);
            if (*parentIt)
                (*parentIt)->removeShape(p);
            ++parentIt;

        }
        if (d->combinedPathParent)
            d->combinedPathParent->addShape(d->combinedPath);
        d->controller->addShape(d->combinedPath);
    }
}

void KoPathCombineCommand::undo()
{
    if (! d->paths.size())
        return;

    d->isCombined = false;

    if (d->controller) {
        d->controller->removeShape(d->combinedPath);
        if (d->combinedPath->parent())
            d->combinedPath->parent()->removeShape(d->combinedPath);
        QList<KoShapeContainer*>::iterator parentIt = d->oldParents.begin();
        Q_FOREACH (KoPathShape* p, d->paths) {
            d->controller->addShape(p);
            p->setParent(*parentIt);
            ++parentIt;
        }
    }
    KUndo2Command::undo();
}

