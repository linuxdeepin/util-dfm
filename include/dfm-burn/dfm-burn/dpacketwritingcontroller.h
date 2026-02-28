// SPDX-FileCopyrightText: 2024 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DPACKETWRITINGCONTROLLER_H
#define DPACKETWRITINGCONTROLLER_H

#include <dfm-burn/dburn_global.h>

#include <QObject>

DFM_BURN_BEGIN_NS

class DPacketWritingControllerPrivate;
class DPacketWritingController : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(DPacketWritingController)

public:
    explicit DPacketWritingController(const QString &dev, const QString &workingPath,
                                      QObject *parent = nullptr);
    ~DPacketWritingController() override;

    QString device() const;
    QString localWorkingDirectory() const;
    QString lastError() const;
    bool isOpen() const;

    bool open();
    void close();
    bool put(const QString &fileName);
    bool mv(const QString &srcName, const QString &destName);
    bool rm(const QString &fileName);

private:
    QScopedPointer<DPacketWritingControllerPrivate> dptr;
};

DFM_BURN_END_NS

#endif   // DPACKETWRITINGCONTROLLER_H
