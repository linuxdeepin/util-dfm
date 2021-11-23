#include "dfmblockdevice.h"
#include "dfmblockmonitor.h"
#include "dfmdevicemanager.h"
#include "base/dfmmountutils.h"
#include "dfmprotocolmonitor.h"
#include "dfmblockmonitor.h"

#include <QVariantMap>
#include <QDebug>
#include <QApplication>
#include <QTimer>
#include <QtConcurrent>

DFM_MOUNT_USE_NS
int main(int argc, char **argv) {
    QApplication app(argc, argv);

    DFMDeviceManager *mng = DFMDeviceManager::instance();
    auto monitor = mng->getRegisteredMonitor(DeviceType::ProtocolDevice).objectCast<DFMProtocolMonitor>();
    monitor->getDevices();
    qDebug() << "start monitor: " << mng->startMonitorWatch();

//    QObject::connect(mng, &DFMDeviceManager::deviceAdded, [mng](const QString &id, DeviceType type){
//        auto monitor = mng->getRegisteredMonitor(type);
//        if (!monitor)
//            return;
//        auto blkMnt = monitor.dynamicCast<DFMBlockMonitor>();
//        auto blk = blkMnt->createDeviceById(id);
//        QtConcurrent::run([blk]{
//            while (1)
//                blk->mountPoint();
//        });
//    });

//    QObject::connect(mng, &DFMDeviceManager::propertyChanged, [mng](const QString &id, const QMap<Property, QVariant> &changes){
//        qDebug() << "\n\t " << id << "property changed";
//        auto iter = changes.cbegin();
//        while (iter != changes.cend()) {
//            qDebug() << "\t" << Utils::getNameByProperty(iter.key()) << ": " << iter.value();
//            iter++;
//        }
//        qDebug() << "\t################################################\n\n";
//    });

    return app.exec();
}
