#include "dfmblockdevice.h"
#include "dfmblockmonitor.h"
#include "dfmdevicemanager.h"


#include <QVariantMap>
#include <QDebug>
#include <QApplication>
#include <QTimer>

DFM_MOUNT_USE_NS
int main(int argc, char **argv) {
    QApplication app(argc, argv);

//    DFMBlockMonitor monitor;
//    qDebug() << monitor.startMonitor();
//    QObject::connect(&monitor, &DFMBlockMonitor::deviceAdded, [](DFMDevice *dev){
//        qDebug() << "mount device: " << dev->mount({});
//        qDebug() << dev->getProperty(Property::BlockIDLabel).toString() << ": "
//                 << dev->getProperty(Property::BlockIDType).toString();
//    });
    DFMDeviceManager *mng = DFMDeviceManager::instance();
    qDebug() << mng->startMonitorWatch();
    QObject::connect(mng, &DFMDeviceManager::blockDeviceAdded, []{
        qDebug() << "new device added...";
    });

    qDebug() << "=================================================================>";
    qDebug() << mng->devices();
    qDebug() << "<=================================================================";
//    QTimer::singleShot(10000, &monitor, [&monitor]{
//        monitor.stopMonitor();
//    });
    return app.exec();
}
