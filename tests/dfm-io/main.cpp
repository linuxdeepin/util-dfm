// SPDX-FileCopyrightText: 2020 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include <sanitizer/asan_interface.h>

#include <QDebug>
#include <QApplication>
#include <QProcess>

int main(int argc, char *argv[])
{
    QApplication app(argc,argv);

    qInfo() << "start test cases ..............";

    ::testing::InitGoogleTest(&argc, argv);

    int ret = RUN_ALL_TESTS();

    qInfo() << "end test cases ..............";

#ifdef ENABLE_TSAN_TOOL
    __sanitizer_set_report_path("../../asan_util_dfm.log");
#endif

    return ret;
}
