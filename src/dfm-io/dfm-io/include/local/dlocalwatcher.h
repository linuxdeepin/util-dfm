// SPDX-FileCopyrightText: 2020 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

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

    void setWatchType(WatchType type) DFM_OVERRIDE;
    WatchType watchType() const DFM_OVERRIDE;

    bool running() const DFM_OVERRIDE;
    bool start(int timeRate = 200) DFM_OVERRIDE;
    bool stop() DFM_OVERRIDE;

    DFMIOError lastError() const DFM_OVERRIDE;

private:
    QSharedPointer<DLocalWatcherPrivate> d = nullptr;
};

END_IO_NAMESPACE

#endif   // DLOCALWATCHER_H
