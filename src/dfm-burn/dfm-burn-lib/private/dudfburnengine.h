// SPDX-FileCopyrightText: 2022 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef UDFBURNENGINE_H
#define UDFBURNENGINE_H

#include <dfm-burn/dburn_global.h>

#include <QObject>
#include <QLibrary>

DFM_BURN_BEGIN_NS

class DUDFBurnEngine : public QObject
{
    Q_OBJECT
public:
    explicit DUDFBurnEngine(QObject *parent = nullptr);
    virtual ~DUDFBurnEngine() override;

    bool doBurn(const QString &dev, const QPair<QString, QString> files, QString volId, const BurnOptions &opts);
    QStringList lastErrorMessage() const;

Q_SIGNALS:
    void jobStatusChanged(JobStatus status, int progress);

private:
    bool canSafeUse() const;
    QStringList readErrorsFromLog() const;

private:
    QLibrary lib;
    bool libLoaded { false };
    bool funcsLoaded { true };
    QStringList message;
};

DFM_BURN_END_NS

#endif   // UDFBURNENGINE_H
