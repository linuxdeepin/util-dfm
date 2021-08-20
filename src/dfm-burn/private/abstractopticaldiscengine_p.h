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
#ifndef ABSTRACTOPTICALDISCENGINE_H
#define ABSTRACTOPTICALDISCENGINE_H

#include <QObject>

#include "dfm_burn.h"

BEGIN_BURN_NAMESPACE

class AbstractOpticalDiscEnginePrivate;
class AbstractOpticalDiscEngine
{
public:
    virtual ~AbstractOpticalDiscEngine();

    static AbstractOpticalDiscEngine *create(const QString &dev);

protected:
    AbstractOpticalDiscEngine();
    AbstractOpticalDiscEngine(AbstractOpticalDiscEnginePrivate &);

    QScopedPointer<AbstractOpticalDiscEnginePrivate> d_ptr;

private:
    Q_DECLARE_PRIVATE(AbstractOpticalDiscEngine)
    Q_DISABLE_COPY(AbstractOpticalDiscEngine)
};

class AbstractOpticalDiscEnginePrivate
{
public:
    inline AbstractOpticalDiscEnginePrivate() {}
    inline virtual ~AbstractOpticalDiscEnginePrivate() {}

    AbstractOpticalDiscEngine *q_ptr;
    Q_DECLARE_PUBLIC(AbstractOpticalDiscEngine)
};

END_BURN_NAMESPACE

#endif // ABSTRACTOPTICALDISCENGINE_H
