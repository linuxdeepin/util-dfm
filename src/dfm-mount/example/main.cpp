#include "dfmblockdevice.h"
#include "dfmblockmonitor.h"
#include "dfmdevicemanager.h"
#include "base/dfmmountutils.h"

#include <QVariantMap>
#include <QDebug>
#include <QApplication>
#include <QTimer>

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

    auto monitor = mng->getRegisteredMonitor(DeviceType::BlockDevice).objectCast<DFMBlockMonitor>();
    auto devicePaths = monitor->resolveDeviceFromDrive("/org/freedesktop/UDisks2/drives/USB_SanDisk_3_2e2Gen1_0101d1edc092e4d140f3228dcdc865e2cd4571a035a163e50463efec5218f0012d10000000000000000000009b350d6e00867700a3558107b528d843");
    for (const auto &path: devicePaths) {
        auto blkdev = monitor->createDeviceById(path).objectCast<DFMBlockDevice>();
        if (!blkdev) continue;
        blkdev->mount();
        if (blkdev->lastError() != DeviceError::NoError) {
            qDebug() << Utils::errorMessage(blkdev->lastError()) << path;
        }
    }

    QTimer::singleShot(10000, mng, [&devicePaths, monitor]{
        for (const auto &path: devicePaths) {
            auto blkdev = monitor->createDeviceById(path).objectCast<DFMBlockDevice>();
            if (!blkdev) continue;
            blkdev->unmount();
            if (blkdev->lastError() != DeviceError::NoError) {
                qDebug() << Utils::errorMessage(blkdev->lastError()) << path;
            }
        }
    });

    auto ret = monitor->resolveDeviceNode("/dev/sdb", {});
    qDebug() << ret;

    return app.exec();
}
