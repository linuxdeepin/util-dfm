#include "dfmblockdevice.h"
#include "dfmblockmonitor.h"
#include "dfmdevicemanager.h"
#include "base/dfmmountutils.h"

#include <QVariantMap>
#include <QDebug>
#include <QApplication>
#include <QTime>

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
    QTime t; t.start();
    auto encryptedBlk = monitor->createDeviceById("/org/freedesktop/UDisks2/block_devices/sdb4").objectCast<DFMBlockDevice>();
    qDebug() << "1: " << t.elapsed();
    QString clearTextDev;
    bool success = encryptedBlk->unlock("123", clearTextDev, {});
    qDebug() << "unlock success? " << success;
    if (success) {
        t.restart();
        auto clearBlk = monitor->createDeviceById(clearTextDev).objectCast<DFMBlockDevice>();
        qDebug() << "2: " << t.elapsed();
        clearBlk->mount();
        qDebug() << "has error: " << Utils::errorMessage(clearBlk->lastError());
        qDebug() << clearBlk->sizeUsage() << clearBlk->sizeFree();
//        clearBlk->unmount();
//        qDebug() << "has error: " << Utils::errorMessage(clearBlk->lastError());
//        encryptedBlk->lock();
//        qDebug() << "has error: " << Utils::errorMessage(encryptedBlk->lastError());
    }
    return app.exec();
}
