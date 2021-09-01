#include "base/dfmdevice.h"
#include "dfmblockdevice.h"
#include "dfmblockmonitor.h"
#include <QVariantMap>
#include <QDebug>
#include <QApplication>

DFM_MOUNT_USE_NS
int main(int argc, char **argv) {
    QApplication app(argc, argv);

    DFMBlockMonitor monitor;
    qDebug() << monitor.startMonitor();
    QObject::connect(&monitor, &DFMBlockMonitor::deviceAdded, [](DFMDevice *dev){
        qDebug() << dev->path();
        qDebug() << "yoho!!!!!!!!!!!!!!!!!!!!!!!!1";
    });
    return app.exec();
}
