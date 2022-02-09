/*
 * Copyright (C) 2022 Uniontech Software Technology Co., Ltd.
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

#include <QCoreApplication>
#include <QDebug>

#include <opticaldiscmanager.h>
#include <opticaldiscinfo.h>

DFM_BURN_USE_NS

static void showInfo(const QString &dev)
{
    QScopedPointer<OpticalDiscInfo> info { OpticalDiscManager::createOpticalInfo(dev) };
    qDebug() << info->device();
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    showInfo("/dev/sr0");
    return a.exec();
}
