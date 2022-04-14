#include <QApplication>
#include <QDebug>

#include "dfmprotocoldevice.h"
#include "dfmdevicemanager.h"

using namespace dfmmount;
int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    DFMDeviceManager::instance()->startMonitorWatch();

    return app.exec();
}
