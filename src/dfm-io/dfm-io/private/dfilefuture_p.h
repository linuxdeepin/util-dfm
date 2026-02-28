// SPDX-FileCopyrightText: 2022 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DFILEFUTURE_P_H
#define DFILEFUTURE_P_H

#include <QUrl>

#include <dfm-io/dfmio_global.h>
#include <dfm-io/error/error.h>

BEGIN_IO_NAMESPACE
class DFileFuture;
class DFuturePrivate
{
public:
    explicit DFuturePrivate(DFileFuture *qq);
    ~DFuturePrivate();

    DFileFuture *q = nullptr;
    DFMIOError error;
};
END_IO_NAMESPACE

#endif   // DFILEFUTURE_P_H
