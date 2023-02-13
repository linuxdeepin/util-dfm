// SPDX-FileCopyrightText: 2020 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DWATCHER_P_H
#define DWATCHER_P_H

#include "dfmio_global.h"

#include "core/dfileinfo.h"
#include "core/dwatcher.h"

#include <QUrl>

BEGIN_IO_NAMESPACE

class DWatcher;

class DWatcherPrivate
{
public:
    explicit DWatcherPrivate(DWatcher *q);
    virtual ~DWatcherPrivate();

public:
    DWatcher *q = nullptr;
    QUrl uri;

    DWatcher::RunningFunc runningFunc = nullptr;
    DWatcher::StartFunc startFunc = nullptr;
    DWatcher::StopFunc stopFunc = nullptr;
    DWatcher::SetWatchTypeFunc setWatchTypeFunc = nullptr;
    DWatcher::WatchTypeFunc watchTypeFunc = nullptr;
    DWatcher::LastErrorFunc lastErrorFunc = nullptr;

    int timeRate = 200;
};

END_IO_NAMESPACE

#endif   // DWATCHER_P_H
