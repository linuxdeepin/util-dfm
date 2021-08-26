#include "include/dfmabstractdevice.h"
#include "include/dfmblockdevice.h"
#include "include/dfmblockmonitor.h"
#include <QVariantMap>
#include <QDebug>
#include <QApplication>

DFM_MOUNT_USE_NS
int main(int argc, char **argv) {
    QApplication app(argc, argv);
    DFMAbstractDevice dev(nullptr);
    dev.mount(QVariantMap());
    DFMBlockDevice dev2("path", nullptr);
    dev2.mount(QVariantMap());

    DFMBlockMonitor monitor;
    qDebug() << monitor.startMonitor();
    QObject::connect(&monitor, &DFMBlockMonitor::deviceAdded, [](DFMAbstractDevice *dev){
        qDebug() << dev->path();
        qDebug() << "yoho!!!!!!!!!!!!!!!!!!!!!!!!1";
    });
    return app.exec();
}
