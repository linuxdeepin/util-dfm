// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DVDRWFORMATENGINE_H
#define DVDRWFORMATENGINE_H

#include <dfm-burn/dburn_global.h>

#include <QObject>

DFM_BURN_BEGIN_NS

class DVDRwFormatEngine : public QObject
{
    Q_OBJECT

public:
    explicit DVDRwFormatEngine(QObject *parent = nullptr);
    ~DVDRwFormatEngine() override;

    bool doErase(const QString &dev, MediaType type);

Q_SIGNALS:
    void jobStatusChanged(JobStatus status, int progress);

private:
    void parseProgress(const QByteArray &data, int totalPhases);

private:
    int phase { 0 };
    int lastRawProgress { 0 };
};

DFM_BURN_END_NS

#endif   // DVDRWFORMATENGINE_H
