/*
 * Copyright (C) 2020 ~ 2021 Uniontech Software Technology Co., Ltd.
 *
 * Author:     dengkeyun<dengkeyun@uniontech.com>
 *
 * Maintainer: max-lv<lvwujun@uniontech.com>
 *             xushitong<xushitong@uniontech.com>
 *             zhangsheng<zhangsheng@uniontech.com>
 *             lanxuesong<lanxuesong@uniontech.com>
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

#ifndef DLOCALWATCHER_H
#define DLOCALWATCHER_H

#include "dfmio_global.h"

#include "core/dwatcher.h"

BEGIN_IO_NAMESPACE

class DLocalWatcherPrivate;

class DLocalWatcher : public DWatcher
{
    Q_OBJECT
public:
    explicit DLocalWatcher(const QUrl &uri, QObject *parent = nullptr);
    ~DLocalWatcher() override;

    bool start(int timeRate);
    bool stop();

private:
    QSharedPointer<DLocalWatcherPrivate> d_ptr;
    Q_DECLARE_PRIVATE(DLocalWatcher)
};

END_IO_NAMESPACE

#endif // DLOCALWATCHER_H
