/*
 * Copyright (C) 2020 ~ 2021 Uniontech Software Technology Co., Ltd.
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
#ifndef DWATCHER_H
#define DWATCHER_H

#include "error.h"
#include "core/dfileinfo.h"

#include <QObject>
#include <QSharedPointer>

#include <functional>

BEGIN_IO_NAMESPACE

class DFileInfo;
class DWatcherPrivate;

class DWatcher : public QObject
{
    Q_OBJECT
public:
    using StartFunc = std::function<bool(int)>;
    using StopFunc = std::function<bool()>;

public:
    explicit DWatcher(const QUrl &uri, QObject *parent = nullptr);
    virtual ~DWatcher();

    QUrl uri() const;
    void setTimeRate(int msec);
    int timeRate() const;
    void setWatchAttributeIDList(const QList<DFileInfo::AttributeID> &ids);
    void addWatchAttributeID(const DFileInfo::AttributeID &id);
    QList<DFileInfo::AttributeID> watchAttributeIDList() const;

    bool running() const;

    DFM_VIRTUAL bool start(int timeRate);
    DFM_VIRTUAL bool stop();

    // register
    void registerStart(const StartFunc &func);
    void registerStop(const StopFunc &func);

    DFMIOError lastError() const;

Q_SIGNALS:
    void fileChanged(const QUrl &uri, const DFileInfo &fileInfo);
    void fileDeleted(const QUrl &uri, const DFileInfo &fileInfo);
    void fileAdded(const QUrl &uri, const DFileInfo &fileInfo);

private:
    QSharedPointer<DWatcherPrivate> d_ptr;
    Q_DECLARE_PRIVATE(DWatcher)
};

END_IO_NAMESPACE

#endif // DWATCHER_H
