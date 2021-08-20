/*
 * Copyright (C) 2020 ~ 2021 Deepin Technology Co., Ltd.
 *
 * Author:     zhangsheng <zhangsheng@uniontech.com>
 *
 * Maintainer: zhangsheng <zhangsheng@uniontech.com>
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
#ifndef ENGINE_FACTORY_HPP
#define ENGINE_FACTORY_HPP

#include <dblockdevice.h>
#include <ddiskmanager.h>
#include <QSharedPointer>

#include "dfm_burn.h"
#include "private/xorrisoengine_p.h"

BEGIN_BURN_NAMESPACE

namespace engine_factory {
AbstractOpticalDiscEngine *engineFactory(const QString &dev)
{
    AbstractOpticalDiscEngine *engine = nullptr;
    QStringList &&paths = DDiskManager::resolveDeviceNode(dev, {});
    if (paths.isEmpty())
        return engine;
    const QString &udiskPath = paths.last();
    QSharedPointer<DBlockDevice> blk(DDiskManager::createBlockDevice(udiskPath));

    if (blk) {
        // 根据文件系统来创建不同的 engine
        const QString &fs = blk->idType();
        if (fs.compare("iso9660", Qt::CaseInsensitive) == 0)
            engine = new XorrisoEngine;
        else if (fs.compare("udf", Qt::CaseInsensitive) == 0)
            ; // TODO(zhangs): UDF engine
        else
            ; // TODO(zhangs): 自行解析文件系统
    }

    return engine;
}
} // namespace engine_factory

END_BURN_NAMESPACE

#endif // ENGINE_FACTORY_HPP
