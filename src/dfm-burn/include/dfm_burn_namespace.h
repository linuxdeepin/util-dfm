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
#ifndef DFM_BURN_NAMESPACE_H
#define DFM_BURN_NAMESPACE_H

#include "dfm_burn.h"

BEGIN_BURN_NAMESPACE

namespace dfmburn {

/**
* @brief 光盘文件系统
*/
enum class FileSystem : unsigned int
{
    kNoFS,
    kISO9660,
    kUDF
};

/**
 * @brief 光盘类型
 */
enum class MediaType : unsigned int
{
    kNoMedia,
    kCD_ROM,
    kCD_R,
    kCD_RW,
    kDVD_ROM,
    kDVD_R,
    kDVD_RW,
    kDVD_PLUS_R,
    kDVD_PLUS_R_DL,
    kDVD_RAM,
    kDVD_PLUS_RW,
    kBD_ROM,
    kBD_R,
    kBD_RE
};

} // namespace dfmburn

END_BURN_NAMESPACE

#endif // DFM_BURN_NAMESPACE_H
