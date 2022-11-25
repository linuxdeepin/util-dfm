// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DFILEFUTURE_P_H
#define DFILEFUTURE_P_H

#include <QUrl>

#include "dfmio_global.h"
#include "error/error.h"

BEGIN_IO_NAMESPACE
class DFileFuture;
class DFuturePrivate
{
public:
    explicit DFuturePrivate(DFileFuture *q);
    ~DFuturePrivate();

    DFileFuture *q = nullptr;
    DFMIOError error;
};
END_IO_NAMESPACE

#endif   // DFILEFUTURE_P_H
