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

#include <glib.h>

DFM_MOUNT_USE_NS
int main(int argc, char **argv) {
    QApplication app(argc, argv);

    DFMDeviceManager *mng = DFMDeviceManager::instance();
    qDebug() << "start monitor: " << mng->startMonitorWatch();
    QObject::connect(mng, &DFMDeviceManager::deviceAdded, [mng](const QString &devPath, DeviceType type){
        qDebug() << "new device added..." << devPath;
        auto monitor = mng->getRegisteredMonitor(type);
        auto dev = (monitor->createDeviceById(devPath));
        if (dev) {
            auto *blkDev = dynamic_cast<DFMBlockDevice *>(dev.data());
            qDebug() << "---------------------------------------------------\n";
            qDebug() << "mountpoints:           " << blkDev->mountPoints();
            qDebug() << "device:                " << blkDev->device();
            qDebug() << "drive:                 " << blkDev->drive();
            qDebug() << "removable:             " << blkDev->removable();
            qDebug() << "optical:               " << blkDev->optical();
            qDebug() << "opticalBlank:          " << blkDev->opticalBlank();
            qDebug() << "mediaCompatibility:    " << blkDev->mediaCompatibility();
            qDebug() << "canPowerOff:           " << blkDev->canPowerOff();
            qDebug() << "ejectable:             " << blkDev->ejectable();
            qDebug() << "isEncrypted:           " << blkDev->isEncrypted();
            qDebug() << "hasFileSystem:         " << blkDev->hasFileSystem();
            qDebug() << "hintIgnore:            " << blkDev->hintIgnore();
            qDebug() << "path:                  " << blkDev->path();
            qDebug() << "---------------------------------------------------\n";
            qDebug() << "Mount device: " << dev->mount();
        }
    });
    QObject::connect(mng, &DFMDeviceManager::mounted, [mng](const QString &obj, const QString &mpt, DeviceType type){
        qDebug() << obj << "is mounted at" << mpt << "which type is" << static_cast<int>(type);
    });
    QObject::connect(mng, &DFMDeviceManager::unmounted, [mng](const QString &obj, DeviceType type){
        qDebug() << obj << "is unmounted, which type is" << static_cast<int>(type);
    });

    qDebug() << "\n=================================================================>";
    qDebug() << "test cast functions";
    QStringList lst {
        "str1",
        "str2"
    };
    QList<QVariant> varlst {
        "123", 123, 3.14
    };
    QByteArray barr("this is byte string");
    QVariantMap item {
        std::pair<QString, QVariant>("map item1", "test"),
        std::pair<QString, QVariant>("map item2", "test"),
    };
    QVariantMap vm {
        std::pair<QString, QVariant>("string", "test"),
        std::pair<QString, QVariant>("int", 123),
        std::pair<QString, QVariant>("strlst", lst),
        std::pair<QString, QVariant>("byteString", barr),
        std::pair<QString, QVariant>("bool", false),
        std::pair<QString, QVariant>("double", 3.141592654),
        std::pair<QString, QVariant>("map", item),
        std::pair<QString, QVariant>("lst", varlst)
    };

    GVariant *var = Utils::castFromQVariantMap(vm);
    char *value_str = g_variant_print(var, false);
    qDebug() << "\t\tcast to gvariant" << value_str;
    qDebug() << "________________";
    qDebug() << "\t\tcast from gvariant" << Utils::castFromGVariant(var);

//    var = Utils::castFromQVariantMap({});
//    qDebug() << "cast to gvariant from empty qvm: " << g_variant_print(var, false);
    qDebug() << "<=================================================================";

//    return 0;

    qDebug() << "\n=================================================================>";
    qDebug() << "test resolve device node";
    auto monitor = mng->getRegisteredMonitor(DeviceType::BlockDevice);
    auto blkMonitor = qobject_cast<QSharedPointer<DFMBlockMonitor>>(monitor);
    qDebug() << blkMonitor->resolveDevice({{"path", "/dev/sda1"}}, {});
    qDebug() << "<=================================================================";

    qDebug() << "\n=================================================================>";
    qDebug() << "test monitor.getdevices";
    qDebug() << blkMonitor->getDevices();
    qDebug() << "<=================================================================";

    // test createDevice
    qDebug() << "\n=================================================================>";
    qDebug() << "test monitor.resolvedevice";
    auto path = blkMonitor->resolveDevice({{"path", "/dev/sda1"}}, {});
    qDebug() << path;

    qDebug() << "test monitor.resolvedevicenode";
    path = blkMonitor->resolveDeviceNode("/dev/sda1", {});
    qDebug() << path;

    qDebug() << "test monitor.createDevice";
    auto blksdb1 = blkMonitor->createDeviceById(path.first());
    qDebug() << "sdb1's ilabel: " << blksdb1->getProperty(Property::BlockIDLabel);
    qDebug() << "<=================================================================";
    return app.exec();
}
