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
