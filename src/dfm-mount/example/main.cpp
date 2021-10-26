#include "dfmblockdevice.h"
#include "dfmblockmonitor.h"
#include "dfmdevicemanager.h"
#include "base/dfmmountutils.h"

#include <QVariantMap>
#include <QDebug>
#include <QApplication>
#include <QTimer>
#include <QThread>
#include <QEventLoop>
#include <QtConcurrent>

#include <glib.h>

DFM_MOUNT_USE_NS
int main(int argc, char **argv) {
    QApplication app(argc, argv);

    DFMDeviceManager *mng = DFMDeviceManager::instance();
    qDebug() << "start monitor: " << mng->startMonitorWatch();
    QObject::connect(mng, &DFMDeviceManager::propertyChanged, [mng](const QString &id, const QMap<Property, QVariant> &changes){
        qDebug() << "\n\t " << id << "property changed";
        auto iter = changes.cbegin();
        while (iter != changes.cend()) {
            qDebug() << "\t" << Utils::getNameByProperty(iter.key()) << ": " << iter.value();
            iter++;
        }
        qDebug() << "\t################################################\n\n";
    });

    QSharedPointer<DFMBlockDevice> blkdev;
    auto monitor = mng->getRegisteredMonitor(DeviceType::BlockDevice);
    auto dev = monitor->createDeviceById("/org/freedesktop/UDisks2/block_devices/sdb4");
    blkdev = qobject_cast<QSharedPointer<DFMBlockDevice>>(dev);
    blkdev->ejectAsync({}, [](bool result, DeviceError err){
        qDebug() << "eject finished: " << result << static_cast<int>(err);
    });
    auto sdb2 = monitor->createDeviceById("/org/freedesktop/UDisks2/block_devices/sdb");
    sdb2->renameAsync("sdb", {}, [](bool result, DeviceError err) {
        qDebug() << "rename: " << result << static_cast<int>(err);
    });

    return app.exec();
}
