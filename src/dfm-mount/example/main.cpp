#include "dfmblockdevice.h"
#include "dfmblockmonitor.h"
#include <QVariantMap>
#include <QDebug>
#include <QApplication>
#include <QTimer>

DFM_MOUNT_USE_NS
int main(int argc, char **argv) {
    QApplication app(argc, argv);

    DFMBlockMonitor monitor;
    qDebug() << monitor.startMonitor();
    QObject::connect(&monitor, &DFMBlockMonitor::deviceAdded, [](DFMDevice *dev){
        qDebug() << "mount device: " << dev->mount({});
        qDebug() << dev->getProperty(Property::BlockIDLabel).toString() << ": "
                 << dev->getProperty(Property::BlockIDType).toString();
    });

//    QTimer::singleShot(10000, &monitor, [&monitor]{
//        monitor.stopMonitor();
//    });
    return app.exec();
}
