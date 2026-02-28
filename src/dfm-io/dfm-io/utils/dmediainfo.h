// SPDX-FileCopyrightText: 2022 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DMEDIAINFO_H
#define DMEDIAINFO_H

#include <dfm-io/dfmio_global.h>
#include <dfm-io/dfileinfo.h>

#include <QObject>
#include <QString>
#include <QScopedPointer>

#include <functional>

BEGIN_IO_NAMESPACE

class DMediaInfoPrivate;
class DMediaInfo : public QObject
{
public:
    using FinishedCallback = std::function<void()>;

    explicit DMediaInfo(const QString &fileName);
    ~DMediaInfo();

    QString value(const QString &key, DFileInfo::MediaType meidiaType = DFileInfo::MediaType::kGeneral);

    void startReadInfo(FinishedCallback callback);
    void stopReadInfo();

private:
    QScopedPointer<DMediaInfoPrivate> d;
};

END_IO_NAMESPACE

#endif   // DMEDIAINFO_H
