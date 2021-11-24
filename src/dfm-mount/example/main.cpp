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
    qDebug() << "start monitor: " << mng->startMonitorWatch();

    auto monitor = mng->getRegisteredMonitor(DeviceType::ProtocolDevice).objectCast<DFMProtocolMonitor>();
    QObject::connect(monitor.data(), &DFMProtocolMonitor::deviceAdded, [monitor](const QString &id){
        qDebug() << "#### device added: " << id;
        auto dev = monitor->createDeviceById(id);
        qDebug() << dev->mount() << __PRETTY_FUNCTION__;
        qDebug() << dev->mountPoint() << __LINE__ << __FUNCTION__;
        qDebug() << dev->displayName() << __LINE__ << __FUNCTION__;
        qDebug() << dev->mount() << Utils::errorMessage(dev->lastError());
    });
    QObject::connect(monitor.data(), &DFMProtocolMonitor::mountAdded, [](const QString &id){
        qDebug() << "#### mount added: " << id;
    });
    QObject::connect(monitor.data(), &DFMProtocolMonitor::deviceRemoved, [](const QString &id){
        qDebug() << "#### device removed: " << id;
    });
    QObject::connect(monitor.data(), &DFMProtocolMonitor::mountRemoved, [](const QString &id){
        qDebug() << "#### mount removed: " << id;
    });
    return app.exec();
}
