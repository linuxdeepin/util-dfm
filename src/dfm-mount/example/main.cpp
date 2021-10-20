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

    DFMDeviceManager *mng = DFMDeviceManager::instance();
    qDebug() << mng->startMonitorWatch();
    QObject::connect(mng, &DFMDeviceManager::deviceAdded, [](DFMDevice *dev){
        qDebug() << "new device added..." << dev->getProperty(Property::BlockIDLabel).toString();
        auto *blkDev = dynamic_cast<DFMBlockDevice *>(dev);
        if (blkDev) {
            qDebug() << "\n";
            qDebug() << "mountpoints: " << blkDev->mountPoints();
            qDebug() << "device: " << blkDev->device();
            qDebug() << "drive: " << blkDev->drive();
            qDebug() << "removable: " << blkDev->removable();
            qDebug() << "optical: " << blkDev->optical();
            qDebug() << "opticalBlank: " << blkDev->opticalBlank();
            qDebug() << "mediaCompatibility: " << blkDev->mediaCompatibility();
            qDebug() << "canPowerOff: " << blkDev->canPowerOff();
            qDebug() << "ejectable: " << blkDev->ejectable();
            qDebug() << "isEncrypted: " << blkDev->isEncrypted();
            qDebug() << "hasFileSystem: " << blkDev->hasFileSystem();
            qDebug() << "hintIgnore: " << blkDev->hintIgnore();
            qDebug() << "path: " << blkDev->path();
            qDebug() << "\n";
        }
    });
    QObject::connect(mng, &DFMDeviceManager::mounted, [](DFMDevice *dev, const QString &mountPoint){
        qDebug() << dev->getProperty(Property::BlockIDLabel) << "mounted at: " << mountPoint;
    });
    QObject::connect(mng, &DFMDeviceManager::unmounted, [](DFMDevice *dev){
        qDebug() << dev->getProperty(Property::BlockIDLabel) << "unmounted";
    });

//    QTimer::singleShot(10000, mng, [mng]{
//        mng->stopMonitorWatch();
//    });

    qDebug() << "=================================================================>";
    qDebug() << mng->devices();
    qDebug() << "<=================================================================";
    return app.exec();
}
