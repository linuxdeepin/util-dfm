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
#ifndef OPTICALDISCMANAGER_H
#define OPTICALDISCMANAGER_H

#include "dfmburn_global.h"

#include <QObject>
#include <QHash>
#include <QUrl>

DFM_BURN_BEGIN_NS

class OpticalDiscInfo;
class OpticalDiscManagerPrivate;
class OpticalDiscManager : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(OpticalDiscManager)

public:
    static OpticalDiscInfo *createOpticalInfo(const QString &dev);

public:
    explicit OpticalDiscManager(const QString &dev, QObject *parent = nullptr);
    ~OpticalDiscManager();

    bool stageFile(const QString &diskPath, const QString &isoPath = "/");
    QHash<QString, QString> stageFiles() const;
    bool removeStageFile(const QString &diskPath);
    void clearStageFiles();
    bool commit(const BurnOptions &opts, int speed = 0, const QString &volId = "ISOIMAGE");
    bool erase();
    bool checkmedia(double *qgood, double *qslow, double *qbad);
    bool writeISO(const QString &isoPath, int speed = 0);
    QString lastError() const;

signals:
    // TODO(zhangs): update status, speed, info

private:
    QScopedPointer<OpticalDiscManagerPrivate> dptr;
};

DFM_BURN_END_NS

#endif   // OPTICALDISCMANAGER_H
