// SPDX-FileCopyrightText: 2022 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QCoreApplication>
#include <QDebug>

#include <dfm-burn/dopticaldiscmanager.h>
#include <dfm-burn/dopticaldiscinfo.h>
#include <dfm-burn/dpacketwritingcontroller.h>

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

//static void pw()
//{
//    DPacketWritingController controller("/dev/sr0", "/home/zhangs");
//    controller.open();
//    controller.close();
//    qDebug() << "quit!!!";
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
    // pw();
    return a.exec();
}
