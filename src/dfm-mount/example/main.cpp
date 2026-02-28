// SPDX-FileCopyrightText: 2023 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QApplication>
#include <QDebug>

#include "dfm-mount/dmount.h"

using namespace dfmmount;
int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    DDeviceManager::instance()->startMonitorWatch();

    return app.exec();
}
