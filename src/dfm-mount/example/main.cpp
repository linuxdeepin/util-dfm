#include <QApplication>
#include <QDebug>

#include "dfmprotocoldevice.h"

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    // test mount network device
//    auto getChoice = [](const QString &msg, const QStringList &choices) {
//        qDebug() << msg;
//        qDebug() << choices;
//        return -1;
//    };
//    auto getPasswd = [](const QString &message, const QString &userDefault, const QString &domainDefault) {
//        qDebug() << message;
//        qDebug() << userDefault << domainDefault;
//        return dfmmount::MountPassInfo();
//    };
//    auto result = [](bool ok, dfmmount::DeviceError err, const QString &mntPath) {
//        qDebug() << "ok? " << ok;
//        qDebug() << static_cast<int>(err);
//        qDebug() << mntPath;
//    };

//    dfmmount::DFMProtocolDevice::mountNetworkDevice("sftp://10.8.10.214", getPasswd, getChoice, result);
    return app.exec();
}
