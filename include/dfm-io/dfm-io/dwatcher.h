// SPDX-FileCopyrightText: 2020 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DWATCHER_H
#define DWATCHER_H

#include <dfm-io/error/error.h>
#include <dfm-io/dfileinfo.h>

#include <QObject>
#include <QSharedPointer>

#include <functional>

BEGIN_IO_NAMESPACE

class DFileInfo;
class DWatcherPrivate;

class DWatcher : public QObject
{
    Q_OBJECT
public:
    enum class WatchType : uint8_t {
        kAuto = 0x00,
        kDir = 0x01,
        kFile = 0x02,
    };

public:
    explicit DWatcher(const QUrl &uri, QObject *parent = nullptr);
    virtual ~DWatcher() override;

    QUrl uri() const;

    void setTimeRate(int msec);
    int timeRate() const;

    void setWatchType(WatchType type);
    WatchType watchType() const;

    bool running() const;
    bool start(int timeRate = 200);
    bool stop();

    DFMIOError lastError() const;

Q_SIGNALS:
    void fileChanged(const QUrl &url);
    void fileDeleted(const QUrl &url);
    void fileAdded(const QUrl &url);
    void fileRenamed(const QUrl &fromUrl, const QUrl &toUrl);

private:
    QScopedPointer<DWatcherPrivate> d;
};

END_IO_NAMESPACE

#endif   // DWATCHER_H
