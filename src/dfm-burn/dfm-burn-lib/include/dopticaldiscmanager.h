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

#include "dburn_global.h"

#include <QObject>
#include <QHash>
#include <QUrl>

DFM_BURN_BEGIN_NS

class DOpticalDiscInfo;
class DOpticalDiscManagerPrivate;
class DOpticalDiscManager : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(DOpticalDiscManager)

public:
    static DOpticalDiscInfo *createOpticalInfo(const QString &dev);

public:
    explicit DOpticalDiscManager(const QString &dev, QObject *parent = nullptr);
    virtual ~DOpticalDiscManager() override;

    bool setStageFile(const QString &diskPath, const QString &isoPath = "/");
    bool commit(const BurnOptions &opts, int speed = 0, const QString &volId = "ISOIMAGE");
    bool erase();
    bool checkmedia(double *qgood, double *qslow, double *qbad);
    bool writeISO(const QString &isoPath, int speed = 0);
    bool dumpISO(const QString &isoPath);
    QString lastError() const;

Q_SIGNALS:
    void jobStatusChanged(JobStatus status, int progress, QString speed, QStringList message);

private:
    QScopedPointer<DOpticalDiscManagerPrivate> dptr;
};

DFM_BURN_END_NS

#endif   // OPTICALDISCMANAGER_H
