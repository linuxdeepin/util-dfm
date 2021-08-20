/*
 * Copyright (C) 2020 ~ 2021 Uniontech Software Technology Co., Ltd.
 *
 * Author:     lanxuesong<lanxuesong@uniontech.com>
 *
 * Maintainer: max-lv<lvwujun@uniontech.com>
 *             xushitong<xushitong@uniontech.com>
 *             zhangsheng<zhangsheng@uniontech.com>
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

#ifndef DDAVIOFACTORY_P_H
#define DDAVIOFACTORY_P_H

#include "dfmio_global.h"

#include "core/diofactory_p.h"
#include "core/dfileinfo.h"
#include "core/dfile.h"
#include "core/denumerator.h"
#include "core/dwatcher.h"
#include "core/doperator.h"

#include <QSharedPointer>

BEGIN_IO_NAMESPACE

class DDavIOFactory;

class DDavIOFactoryPrivate : public DIOFactoryPrivate
{
public:
    explicit DDavIOFactoryPrivate(DDavIOFactory *q);
    ~DDavIOFactoryPrivate() override;

    virtual QSharedPointer<DFileInfo> doCreateFileInfo() const override;
    virtual QSharedPointer<DFile> doCreateFile() const override;
    virtual QSharedPointer<DEnumerator> doCreateEnumerator() const override;
    virtual QSharedPointer<DWatcher> doCreateWatcher() const override;
    virtual QSharedPointer<DOperator> doCreateOperator() const override;
};

END_IO_NAMESPACE

#endif // DDAVIOFACTORY_P_H
