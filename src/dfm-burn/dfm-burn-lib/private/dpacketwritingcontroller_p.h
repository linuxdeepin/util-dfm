// SPDX-FileCopyrightText: 2024 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DPACKETWRITINGCONTROLLER_P_H
#define DPACKETWRITINGCONTROLLER_P_H

#include <dfm-burn/dburn_global.h>

DFM_BURN_BEGIN_NS

class DPacketWritingControllerPrivate
{
public:
    DPacketWritingControllerPrivate();
    bool initCurrentDir();
    void lcd(const QString &path);
    void cd(const QString &path);
    QString makeDiscRootPath();
    int64_t getmtime();

public:
    bool deviceOpended { false };
    QString errorMsg;
    QString deviceName;
    QString oldLocalWoringPath;
    QString localWorkingPath;
};

DFM_BURN_END_NS

#endif   // DPACKETWRITINGCONTROLLER_P_H
