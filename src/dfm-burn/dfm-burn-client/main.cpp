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

#include <dopticaldiscmanager.h>
#include <dopticaldiscinfo.h>

DFM_BURN_USE_NS

// TODO(zhangs): follow code is test code

//static void erase(const QString &dev)
//{
//    DOpticalDiscManager manager(dev);
//    QObject::connect(&manager, &DOpticalDiscManager::jobStatusChanged, [](JobStatus status, int progress, QString speed, QStringList message) {
//        qDebug() << int(status) << progress << speed << message;
//    });
//    manager.erase();
//}

//static void showInfo(const QString &dev)
//{
//    QScopedPointer<DOpticalDiscInfo> info { DOpticalDiscManager::createOpticalInfo(dev) };
//    qDebug() << info->device();
//    qDebug() << int(info->mediaType());
//    qDebug() << info->writeSpeed();
//    qDebug() << info->volumeName();
//}

//static void commit()
//{
//    DOpticalDiscManager manager("/dev/sr0");
//    QObject::connect(&manager, &DOpticalDiscManager::jobStatusChanged, [](JobStatus status, int progress, QString speed, QStringList message) {
//        qDebug() << int(status) << progress << speed << message;
//    });
//    manager.setStageFile("/home/zhangs/.cache/deepin/discburn/_dev_sr0");
//    BurnOptions opts;
//    opts |= BurnOption::kJolietAndRockRidge;
//    opts |= BurnOption::kKeepAppendable;
//    manager.commit(opts, 0, "123");
//}

//static void commitUDF()
//{
//    DOpticalDiscManager manager("/dev/sr0");
//    QObject::connect(&manager, &DOpticalDiscManager::jobStatusChanged, [](JobStatus status, int progress, QString speed, QStringList message) {
//        qDebug() << int(status) << progress << speed << message;
//    });
//    manager.setStageFile("/home/zhangs/Downloads/254111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111.gz");
//    BurnOptions opts;
//    opts |= BurnOption::kUDF102Supported;
//    opts |= BurnOption::kKeepAppendable;
//    manager.commit(opts, 0, "abc123");
//}

//static void writeISO()
//{
//    DOpticalDiscManager manager("/dev/sr0");
//    QObject::connect(&manager, &DOpticalDiscManager::jobStatusChanged, [](JobStatus status, int progress, QString speed, QStringList message) {
//        qDebug() << int(status) << progress << speed << message;
//    });
//    manager.writeISO("/home/zhangs/Downloads/deb/20200413_214350.iso");
//}

//static void check()
//{
//    DOpticalDiscManager manager("/dev/sr0");
//    QObject::connect(&manager, &DOpticalDiscManager::jobStatusChanged, [](JobStatus status, int progress, QString speed, QStringList message) {
//        qInfo() << int(status) << progress << speed << message;
//    });
//    double gud, slo, bad;
//    manager.checkmedia(&gud, &slo, &bad);
//    bool check { true };
//    bool checkRet { !(check && (bad > (2 + 1e-6))) };
//    qDebug() << "check ret" << checkRet;
//}

//static void dumpISO()
//{
//    DOpticalDiscManager manager("/dev/sr0");
//    QObject::connect(&manager, &DOpticalDiscManager::jobStatusChanged, [](JobStatus status, int progress, QString speed, QStringList message) {
//        qInfo() << int(status) << progress << speed << message;
//    });
//    manager.dumpISO("/home/zhangs/tmp/aabb.iso");
//}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    // showInfo("/dev/sr0");
    // erase("/dev/sr0");
    // writeISO();
    // commit();
    // commitUDF();
    // check();
    // dumpISO();
    return a.exec();
}
