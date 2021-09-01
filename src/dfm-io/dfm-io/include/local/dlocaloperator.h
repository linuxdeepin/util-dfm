/*
 * Copyright (C) 2020 ~ 2021 Uniontech Software Technology Co., Ltd.
 *
 * Author:     dengkeyun<dengkeyun@uniontech.com>
 *
 * Maintainer: max-lv<lvwujun@uniontech.com>
 *             xushitong<xushitong@uniontech.com>
 *             zhangsheng<zhangsheng@uniontech.com>
 *             lanxuesong<lanxuesong@uniontech.com>
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

#ifndef DLOCALOPERATOR_H
#define DLOCALOPERATOR_H

#include "dfmio_global.h"

#include "core/doperator.h"

BEGIN_IO_NAMESPACE

class DLocalOperatorPrivate;

class DLocalOperator : public DOperator
{
public:
    explicit DLocalOperator(const QUrl &uri);
    ~DLocalOperator();

    bool renameFile(const QString &newName) DFM_OVERRIDE;
    bool copyFile(const QUrl &destUri, CopyFlag flag) DFM_OVERRIDE;
    bool moveFile(const QUrl &destUri, CopyFlag flag) DFM_OVERRIDE;

    bool trashFile() DFM_OVERRIDE;
    bool deleteFile() DFM_OVERRIDE;
    bool restoreFile() DFM_OVERRIDE;

    bool touchFile() DFM_OVERRIDE;
    bool makeDirectory() DFM_OVERRIDE;
    bool createLink(const QUrl &link) DFM_OVERRIDE;
    bool setFileInfo(const DFileInfo &fileInfo) DFM_OVERRIDE;

private:
    QSharedPointer<DLocalOperatorPrivate> d_ptr;
    Q_DECLARE_PRIVATE(DLocalOperator)
};

END_IO_NAMESPACE

#endif // DLOCALOPERATOR_H
