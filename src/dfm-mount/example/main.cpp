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
//    qDebug() << mng->startMonitorWatch();
    QObject::connect(mng, &DFMDeviceManager::blockDeviceAdded, [mng](DFMDevice *dev){
        qDebug() << "new device added..." << dev->getProperty(Property::BlockIDLabel).toString();
//        qDebug() << "mount new device async...";
//        dev->mountAsync({});
//        QObject::connect(dev, &DFMDevice::mounted, mng, [](const QUrl &mpt){
//            qDebug() << "device mounted at: " << mpt;
//        });
    });

    QTimer::singleShot(10000, mng, [mng]{
        mng->stopMonitorWatch();
    });

    qDebug() << "=================================================================>";
    qDebug() << mng->devices();
    qDebug() << "<=================================================================";
    return app.exec();
}
