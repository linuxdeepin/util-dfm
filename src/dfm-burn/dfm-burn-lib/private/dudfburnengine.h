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
#ifndef UDFBURNENGINE_H
#define UDFBURNENGINE_H

#include "dburn_global.h"

#include <QObject>
#include <QLibrary>

DFM_BURN_BEGIN_NS

class DUDFBurnEngine : public QObject
{
    Q_OBJECT
public:
    explicit DUDFBurnEngine(QObject *parent = nullptr);
    virtual ~DUDFBurnEngine() override;

    bool doBurn(const QString &dev, const QPair<QString, QString> files, QString volId);
    QStringList lastErrorMessage() const;

Q_SIGNALS:
    void jobStatusChanged(JobStatus status, int progress);

private:
    bool canSafeUse() const;
    QStringList readErrorsFromLog() const;

private:
    QLibrary lib;
    bool libLoaded { false };
    bool funcsLoaded { true };
    QStringList message;
};

DFM_BURN_END_NS

#endif   // UDFBURNENGINE_H
