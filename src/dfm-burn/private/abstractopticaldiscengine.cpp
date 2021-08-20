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
#include "private/abstractopticaldiscengine_p.h"

#include "private/engine_factory.hpp"
#include "private/xorrisoengine_p.h"

BEGIN_BURN_NAMESPACE

/*****AbstractOpticalDiscEngine*/

AbstractOpticalDiscEngine::AbstractOpticalDiscEngine()
{
    d_ptr->q_ptr = this;
}

AbstractOpticalDiscEngine::AbstractOpticalDiscEngine(AbstractOpticalDiscEnginePrivate &dd)
    : d_ptr(&dd)
{
    d_ptr->q_ptr = this;
}

AbstractOpticalDiscEngine::~AbstractOpticalDiscEngine()
{

}

AbstractOpticalDiscEngine *AbstractOpticalDiscEngine::create(const QString &dev)
{
    return engine_factory::engineFactory(dev);
}

END_BURN_NAMESPACE
