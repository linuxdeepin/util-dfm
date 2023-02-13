// SPDX-FileCopyrightText: 2020 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DIOFACTORY_H
#define DIOFACTORY_H

#include "dfmio_global.h"
#include "denumerator.h"
#include "error/error.h"
#include "core/dfileinfo.h"

#include <QSharedPointer>

#include <functional>

BEGIN_IO_NAMESPACE

class DIOFactoryPrivate;
class DFileInfo;
class DFile;
class DEnumerator;
class DWatcher;
class DOperator;

class DIOFactory
{
public:
    using CreateFileInfoFunc = std::function<QSharedPointer<DFileInfo>(const char *, const DFMIO::DFileInfo::FileQueryInfoFlags)>;
    using CreateFileFunc = std::function<QSharedPointer<DFile>()>;
    using CreateEnumeratorFunc = std::function<QSharedPointer<DEnumerator>(const QStringList &nameFilters, DEnumerator::DirFilters filters, DEnumerator::IteratorFlags flags)>;
    using CreateWatcherFunc = std::function<QSharedPointer<DWatcher>()>;
    using CreateOperatorFunc = std::function<QSharedPointer<DOperator>()>;

    DIOFactory(const QUrl &uri);
    virtual ~DIOFactory();

    void setUri(const QUrl &uri);
    QUrl uri() const;

    DFM_VIRTUAL QSharedPointer<DFileInfo> createFileInfo(const char *attributes = "*",
                                                         const DFMIO::DFileInfo::FileQueryInfoFlags flag = DFMIO::DFileInfo::FileQueryInfoFlags::kTypeNone) const;
    DFM_VIRTUAL QSharedPointer<DFile> createFile() const;
    DFM_VIRTUAL QSharedPointer<DEnumerator> createEnumerator(const QStringList &nameFilters = QStringList(),
                                                             DEnumerator::DirFilters filters = DEnumerator::DirFilter::kNoFilter,
                                                             DEnumerator::IteratorFlags flags = DEnumerator::IteratorFlag::kNoIteratorFlags) const;
    DFM_VIRTUAL QSharedPointer<DWatcher> createWatcher() const;
    DFM_VIRTUAL QSharedPointer<DOperator> createOperator() const;

    /**
     * @brief 注册接口
     * @param
     * @return
     */
    void registerCreateFileInfo(const CreateFileInfoFunc &func);
    void registerCreateFile(const CreateFileFunc &func);
    void registerCreateEnumerator(const CreateEnumeratorFunc &func);
    void registerCreateWatcher(const CreateWatcherFunc &func);
    void registerCreateOperator(const CreateOperatorFunc &func);

    DFMIOError lastError() const;

protected:
    QScopedPointer<DIOFactoryPrivate> d;

private:
    Q_DISABLE_COPY(DIOFactory)
};

END_IO_NAMESPACE

#endif   // DIOFACTORY_H
