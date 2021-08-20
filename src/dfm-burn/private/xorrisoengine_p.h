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
#ifndef XORRISOENGINE_P_H
#define XORRISOENGINE_P_H

#include "private/abstractopticaldiscengine_p.h"

BEGIN_BURN_NAMESPACE

class XorrisoEnginePrivate;
class XorrisoEngine : public AbstractOpticalDiscEngine
{
    Q_DECLARE_PRIVATE(XorrisoEngine)
public:
    XorrisoEngine();

};

class XorrisoEnginePrivate : public AbstractOpticalDiscEnginePrivate
{
    Q_DECLARE_PUBLIC(XorrisoEngine)
public:
    inline XorrisoEnginePrivate() {}

};

END_BURN_NAMESPACE

#endif // XORRISOENGINE_P_H
