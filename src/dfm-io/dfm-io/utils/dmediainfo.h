/*
 * Copyright (C) 2022 Uniontech Software Technology Co., Ltd.
 *
 * Author:     lanxuesong<lanxuesong@uniontech.com>
 *
 * Maintainer: lanxuesong<lanxuesong@uniontech.com>
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

#ifndef DMEDIAINFO_H
#define DMEDIAINFO_H

#include "dfmio_global.h"
#include "core/dfileinfo.h"

#include <QObject>
#include <QString>
#include <QScopedPointer>

#include <functional>

BEGIN_IO_NAMESPACE

class DMediaInfoPrivate;
class DMediaInfo : public QObject
{
public:
    using FinishedCallback = std::function<void()>;

    explicit DMediaInfo(const QString &fileName);
    ~DMediaInfo();

    QString value(const QString &key, DFileInfo::MediaType meidiaType = DFileInfo::MediaType::kGeneral);

    void startReadInfo(FinishedCallback callback);
    void stopReadInfo();

private:
    QScopedPointer<DMediaInfoPrivate> d;
};

END_IO_NAMESPACE

#endif   // DMEDIAINFO_H
