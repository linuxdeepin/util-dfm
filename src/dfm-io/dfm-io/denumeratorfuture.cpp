// SPDX-FileCopyrightText: 2021 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <dfm-io/denumeratorfuture.h>
#include <dfm-io/dfileinfo.h>
#include <dfm-io/dfmio_utils.h>

USING_IO_NAMESPACE
DEnumeratorFuture::DEnumeratorFuture(const QSharedPointer<DEnumerator> &enumerator, QObject *parent)
    : QObject(parent), enumerator(enumerator)
{
}

DEnumeratorFuture::~DEnumeratorFuture()
{
}

void DEnumeratorFuture::startAsyncIterator()
{
    enumerator->startAsyncIterator();
}

bool DEnumeratorFuture::isFinished()
{
    return enumerator->isAsyncOver();
}

DFMIOError DEnumeratorFuture::lastError() const
{
    return enumerator->lastError();
}

qint64 DEnumeratorFuture::fileCount()
{
    if (enumerator->isAsyncOver())
        return 0;
    auto infos = enumerator->fileInfoList();
    QList<QUrl> children;
    for (const auto &info : infos) {
        auto url = DFMUtils::bindUrlTransform(info->uri());
        if (!children.contains(url))
            children.append(url);
    }

    return children.count();
}

void DEnumeratorFuture::onAsyncIteratorOver()
{
    Q_EMIT asyncIteratorOver();
}
