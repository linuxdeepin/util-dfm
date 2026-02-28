// SPDX-FileCopyrightText: 2020 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DOPTICALDISCINFO_H
#define DOPTICALDISCINFO_H

#include <dfm-burn/dburn_global.h>

#include <QObject>
#include <QString>
#include <QSharedDataPointer>

DFM_BURN_BEGIN_NS

class DOpticalDiscInfoPrivate;

class DOpticalDiscInfo
{
    friend class DOpticalDiscInfoPrivate;
    friend class DOpticalDiscManager;

public:
    DOpticalDiscInfo(const DOpticalDiscInfo &info);
    DOpticalDiscInfo &operator=(const DOpticalDiscInfo &info);
    ~DOpticalDiscInfo();

    bool blank() const;
    QString device() const;
    QString volumeName() const;
    quint64 usedSize() const;
    quint64 availableSize() const;
    quint64 totalSize() const;
    quint64 dataBlocks() const;
    MediaType mediaType() const;
    QStringList writeSpeed() const;

protected:
    DOpticalDiscInfo();
    DOpticalDiscInfo(const QString &dev);

protected:
    QSharedDataPointer<DOpticalDiscInfoPrivate> d_ptr;
};

DFM_BURN_END_NS

#endif   // DOPTICALDISCINFO_H
