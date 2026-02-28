// SPDX-FileCopyrightText: 2022 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef OPTICALDISCMANAGER_H
#define OPTICALDISCMANAGER_H

#include <dfm-burn/dburn_global.h>

#include <QObject>
#include <QHash>
#include <QUrl>

DFM_BURN_BEGIN_NS

class DOpticalDiscInfo;
class DOpticalDiscManagerPrivate;
class DOpticalDiscManager : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(DOpticalDiscManager)

public:
    static DOpticalDiscInfo *createOpticalInfo(const QString &dev);

public:
    explicit DOpticalDiscManager(const QString &dev, QObject *parent = nullptr);
    virtual ~DOpticalDiscManager() override;

    bool setStageFile(const QString &diskPath, const QString &isoPath = "/");
    bool commit(const BurnOptions &opts, int speed = 0, const QString &volId = "ISOIMAGE");
    bool erase();
    bool checkmedia(double *qgood, double *qslow, double *qbad);
    bool writeISO(const QString &isoPath, int speed = 0);
    bool dumpISO(const QString &isoPath);
    QString lastError() const;

Q_SIGNALS:
    void jobStatusChanged(JobStatus status, int progress, QString speed, QStringList message);

private:
    QScopedPointer<DOpticalDiscManagerPrivate> dptr;
};

DFM_BURN_END_NS

#endif   // OPTICALDISCMANAGER_H
