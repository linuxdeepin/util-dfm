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
    enum class WatchType : uint8_t {
        kAuto = 0x00,
        kDir = 0x01,
        kFile = 0x02,
    };

    using RunningFunc = std::function<bool()>;
    using StartFunc = std::function<bool(int)>;
    using StopFunc = std::function<bool()>;
    using SetWatchTypeFunc = std::function<void(WatchType)>;
    using WatchTypeFunc = std::function<WatchType()>;
    using LastErrorFunc = std::function<DFMIOError()>;

public:
    explicit DWatcher(const QUrl &uri, QObject *parent = nullptr);
    virtual ~DWatcher();

    QUrl uri() const;

    void setTimeRate(int msec);
    int timeRate() const;

    void setWatchAttributeIDList(const QList<DFileInfo::AttributeID> &ids);
    void addWatchAttributeID(const DFileInfo::AttributeID &id);
    QList<DFileInfo::AttributeID> watchAttributeIDList() const;

    DFM_VIRTUAL void setWatchType(WatchType type);
    DFM_VIRTUAL WatchType watchType() const;

    DFM_VIRTUAL bool running() const;
    DFM_VIRTUAL bool start(int timeRate = 200);
    DFM_VIRTUAL bool stop();

    DFM_VIRTUAL DFMIOError lastError() const;

    // register
    void registerRunning(const RunningFunc &func);
    void registerStart(const StartFunc &func);
    void registerStop(const StopFunc &func);
    void registerSetWatchType(const SetWatchTypeFunc &func);
    void registerWatchType(const WatchTypeFunc &func);
    void registerLastError(const LastErrorFunc &func);

Q_SIGNALS:
    void fileChanged(const QUrl &url);
    void fileDeleted(const QUrl &url);
    void fileAdded(const QUrl &url);
    void fileRenamed(const QUrl &fromUrl, const QUrl &toUrl);

private:
    QSharedPointer<DWatcherPrivate> d = nullptr;
};

END_IO_NAMESPACE

#endif   // DWATCHER_H
