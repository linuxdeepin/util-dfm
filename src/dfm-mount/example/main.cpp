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
    return app.exec();
}
