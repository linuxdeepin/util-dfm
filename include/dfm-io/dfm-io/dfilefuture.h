// SPDX-FileCopyrightText: 2022 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DFILEFUTURE_H
#define DFILEFUTURE_H

#include <dfm-io/dfmio_global.h>
#include <dfm-io/dfileinfo.h>
#include <dfm-io/error/error.h>

#include <QObject>
#include <QScopedPointer>

BEGIN_IO_NAMESPACE
class DFuturePrivate;
class DFileFuture : public QObject
{
    Q_OBJECT
public:
    explicit DFileFuture(QObject *parent = nullptr);
    ~DFileFuture();

    bool cancel();
    bool isFinished() const;
    bool hasError() const;
    int errorCode();
    QString errorMessage();

    // TODO(lanxs): setCancallable for cancel()
    void setError(DFMIOError error);

Q_SIGNALS:
    void finished();
    void infoAttribute(DFileInfo::AttributeID id, const QVariant &value);
    void infoAttribute(const QByteArray &key, const QVariant &value);
    void progressNotify(qint64 current, qint64 total);
    void infoExists(const bool exists);
    void infoPermissions(const DFile::Permissions permissions);
    void infoSize(const quint64 &size);
    void writeAsyncSize(const qint64 &size);
    void readData(const QByteArray &data);
    void infoMedia(const QUrl &url, const QMap<DFileInfo::AttributeExtendID, QVariant> &map);

private:
    QScopedPointer<DFuturePrivate> d;
};
END_IO_NAMESPACE

#endif   // DFILEFUTURE_H
