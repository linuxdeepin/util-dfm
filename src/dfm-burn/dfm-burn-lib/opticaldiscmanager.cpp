/*
 * Copyright (C) 2022 Uniontech Software Technology Co., Ltd.
 *
 * Author:     zhangsheng<zhangsheng@uniontech.com>
 *
 * Maintainer: max-lv<lvwujun@uniontech.com>
 *             lanxuesong<lanxuesong@uniontech.com>
 *             xushitong<xushitong@uniontech.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "opticaldiscmanager.h"
#include "opticaldiscinfo.h"
#include "private/opticaldiscmanager_p.h"

#include <QUrl>

DFM_BURN_USE_NS

OpticalDiscManager::OpticalDiscManager(const QString &dev, QObject *parent)
    : QObject(parent), dptr(new OpticalDiscManagerPrivate)
{
    dptr->curDev = dev;
}

OpticalDiscManager::~OpticalDiscManager()
{
}

bool OpticalDiscManager::stageFile(const QString &diskPath, const QString &isoPath)
{
    QUrl diskUrl { diskPath };
    QUrl isoUrl { isoPath };

    if (diskUrl.isEmpty() || !diskUrl.isValid()) {
        dptr->errorMsg = "Invalid disk path";
        return false;
    }
    if (isoUrl.isEmpty() || !isoUrl.isValid()) {
        dptr->errorMsg = "Invalid iso path";
        return false;
    }
    if (dptr->files.contains(diskPath)) {
        dptr->errorMsg = "Disk path is repeated";
        return false;
    }

    QHash<QString, QString> newStage;
    newStage.insert(diskPath, isoPath);
    dptr->files.unite(newStage);

    return true;
}

QHash<QString, QString> OpticalDiscManager::stageFiles() const
{
    return dptr->files;
}

bool OpticalDiscManager::removeStageFile(const QString &diskPath)
{
    if (!dptr->files.contains(diskPath)) {
        dptr->errorMsg = "Disk path not staged";
        return false;
    }
    return dptr->files.remove(diskPath) != 0;
}

void OpticalDiscManager::clearStageFiles()
{
    dptr->files.clear();
}

bool OpticalDiscManager::commit(const BurnOptions &opts, int speed, const QString &volId)
{
    return false;
}

bool OpticalDiscManager::erase()
{
    return false;
}

bool OpticalDiscManager::checkmedia(double *qgood, double *qslow, double *qbad)
{
    return false;
}

bool OpticalDiscManager::writeISO(const QString &isoPath, int speed)
{
    return false;
}

QString OpticalDiscManager::lastError() const
{
    return dptr->errorMsg;
}

OpticalDiscInfo *OpticalDiscManager::createOpticalInfo(const QString &dev)
{
    return new OpticalDiscInfo(dev);
}
