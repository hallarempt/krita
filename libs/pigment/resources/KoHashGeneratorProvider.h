/*
 * Copyright (c) 2015 Stefano Bonicatti <smjert@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
*/
#ifndef KOHASHGENERATORPROVIDER_H
#define KOHASHGENERATORPROVIDER_H

#include <QHash>
#include <QMutex>

#include <kritapigment_export.h>

class KoHashGenerator;

class KRITAPIGMENT_EXPORT KoHashGeneratorProvider
{
public:
    KoHashGeneratorProvider();
    ~KoHashGeneratorProvider();

    KoHashGenerator *getGenerator(QString algorithm);
    void setGenerator(QString algorithm, KoHashGenerator *generator);
    static KoHashGeneratorProvider *instance();
private:
    static KoHashGeneratorProvider *instance_var;
    QHash<QString, KoHashGenerator *> hashGenerators;
    QMutex mutex;
};

#endif
