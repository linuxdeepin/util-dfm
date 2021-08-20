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
#ifndef DFMIO_GLOBAL_H
#define DFMIO_GLOBAL_H

//  namespace
#define DFMIO dfmio
#define BEGIN_IO_NAMESPACE namespace DFMIO {
#define USING_IO_NAMESPACE using namespace DFMIO;
#define END_IO_NAMESPACE }

// fake virtual and override
// remind developers that they can override member functions
#define DFM_VIRTUAL
#define DFM_OVERRIDE

#endif // DFMIO_GLOBAL_H
