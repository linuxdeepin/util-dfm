// SPDX-FileCopyrightText: 2020 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DLOCALENUMERATOR_H
#define DLOCALENUMERATOR_H

#include "core/denumerator.h"

#include <QUrl>

BEGIN_IO_NAMESPACE

class DLocalEnumeratorPrivate;

class DLocalEnumerator : public DEnumerator
{
public:
    explicit DLocalEnumerator(const QUrl &uri, const QStringList &nameFilters = QStringList(), DirFilters filters = DirFilter::kNoFilter, IteratorFlags flags = IteratorFlag::kNoIteratorFlags);
    ~DLocalEnumerator();

    bool init() DFM_OVERRIDE;
    void initAsync(int ioPriority = 0, InitCallbackFunc func = nullptr, void *userData = nullptr) DFM_OVERRIDE;
    bool cancel() DFM_OVERRIDE;
    bool hasNext() const DFM_OVERRIDE;
    QUrl next() const DFM_OVERRIDE;
    QSharedPointer<DFileInfo> fileInfo() const DFM_OVERRIDE;
    quint64 fileCount() DFM_OVERRIDE;
    DFMIOError lastError() const DFM_OVERRIDE;

    QList<QSharedPointer<DFileInfo>> fileInfoList();

private:
    QSharedPointer<DLocalEnumeratorPrivate> d = nullptr;
};

END_IO_NAMESPACE

#endif   // DLOCALENUMERATOR_H
