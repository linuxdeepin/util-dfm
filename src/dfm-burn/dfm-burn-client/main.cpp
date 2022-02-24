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

// TODO(zhangs): follow code is test code

static void erase(const QString &dev)
{
    OpticalDiscManager manager(dev);
    QObject::connect(&manager, &OpticalDiscManager::jobStatusChanged, [](JobStatus status, int progress, QString speed, QStringList message) {
        qDebug() << int(status) << progress << speed << message;
    });
    manager.erase();
}

static void showInfo(const QString &dev)
{
    QScopedPointer<OpticalDiscInfo> info { OpticalDiscManager::createOpticalInfo(dev) };
    qDebug() << info->device();
    qDebug() << int(info->mediaType());
    qDebug() << info->writeSpeed();
    qDebug() << info->volumeName();
}

static void commit()
{
    OpticalDiscManager manager("/dev/sr0");
    QObject::connect(&manager, &OpticalDiscManager::jobStatusChanged, [](JobStatus status, int progress, QString speed, QStringList message) {
        qDebug() << int(status) << progress << speed << message;
    });
    manager.setStageFile("/home/zhangs/test/0");
    BurnOptions opts;
    opts |= BurnOption::kJolietAndRockRidge;
    opts |= BurnOption::kKeepAppendable;
    manager.commit(opts, 0, "123");
}

static void commitUDF()
{
    OpticalDiscManager manager("/dev/sr0");
    QObject::connect(&manager, &OpticalDiscManager::jobStatusChanged, [](JobStatus status, int progress, QString speed, QStringList message) {
        qDebug() << int(status) << progress << speed << message;
    });
    manager.setStageFile("/home/zhangs/test/1/git_pro");
    BurnOptions opts;
    opts |= BurnOption::kUDF102Supported;
    opts |= BurnOption::kKeepAppendable;
    manager.commit(opts, 0, "abc123");
}
static void writeISO()
{
    OpticalDiscManager manager("/dev/sr0");
    QObject::connect(&manager, &OpticalDiscManager::jobStatusChanged, [](JobStatus status, int progress, QString speed, QStringList message) {
        qDebug() << int(status) << progress << speed << message;
    });
    manager.writeISO("/home/zhangs/Downloads/deb/20200413_214350.iso");
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    // showInfo("/dev/sr0");
    // erase("/dev/sr0");
    // writeISO();
    // commit();
    // commitUDF();
    return a.exec();
}
