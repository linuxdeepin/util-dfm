// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QTest>

// 前置声明测试类
class tst_DfmIO;
class tst_DFileSorter;

int main(int argc, char *argv[])
{
    int status = 0;

    // 运行各测试类
    extern int run_tst_DfmIO(int argc, char *argv[]);
    extern int run_tst_DFileSorter(int argc, char *argv[]);

    status |= run_tst_DfmIO(argc, argv);
    status |= run_tst_DFileSorter(argc, argv);

    return status;
}
