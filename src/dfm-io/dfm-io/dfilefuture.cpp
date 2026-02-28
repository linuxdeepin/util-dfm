// SPDX-FileCopyrightText: 2022 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "private/dfilefuture_p.h"

#include <dfm-io/dfilefuture.h>

BEGIN_IO_NAMESPACE
DFuturePrivate::DFuturePrivate(DFileFuture *qq)
    : q(qq)
{
}

DFuturePrivate::~DFuturePrivate()
{
}

DFileFuture::DFileFuture(QObject *parent)
    : QObject(parent)
{
}

DFileFuture::~DFileFuture()
{
}

bool DFileFuture::cancel()
{
    // TODO: impl me!
    return false;
}

bool DFileFuture::isFinished() const
{
    // TODO: impl me!
    return false;
}

bool DFileFuture::hasError() const
{
    return d->error.code() != DFM_IO_ERROR_NONE;
}

int DFileFuture::errorCode()
{
    return d->error.code();
}

QString DFileFuture::errorMessage()
{
    return d->error.errorMsg();
}

void DFileFuture::setError(DFMIOError error)
{
    d->error = error;
}

END_IO_NAMESPACE
