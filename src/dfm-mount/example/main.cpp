#include <QApplication>
#include <QDebug>

#include "dprotocoldevice.h"
#include "ddevicemanager.h"

using namespace dfmmount;
int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    DDeviceManager::instance()->startMonitorWatch();

    return app.exec();
}
