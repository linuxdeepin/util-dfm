// SPDX-FileCopyrightText: 2022 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dmediainfo.h"

#include <MediaInfo/MediaInfo.h>

#include <QTimer>
#include <QQueue>
#include <QMutex>
#include <QPointer>
#include <QDebug>

#include <thread>

const size_t MediaInfoStateFinished = 10000;   // read finished and no error

Q_GLOBAL_STATIC(QQueue<QSharedPointer<MediaInfoLib::MediaInfo>>, queueDestoryMediaInfo)

BEGIN_IO_NAMESPACE
class DMediaInfoPrivate : public QObject
{
public:
    DMediaInfoPrivate(DMediaInfo *qq, const QString &fileName)
        : q(qq)
    {
        this->fileName = fileName;
        isStopState.store(false);
        mediaInfo.reset(new MediaInfoLib::MediaInfo());
    }

    ~DMediaInfoPrivate()
    {
        if (mediaInfo) {
            // 由于当远程文件夹下存在大量图片文件时，析构mediainfo对象耗时会很长，造成文管卡
            // 所以将对象添加到队列中，开启线程去释放对象
            static QMutex lock;
            {
                QMutexLocker locker(&lock);
                queueDestoryMediaInfo->enqueue(mediaInfo);
            }

            static bool isRunning = false;
            if (!isRunning) {
                isRunning = true;
                std::thread thread(
                        []() {
                            while (!queueDestoryMediaInfo->isEmpty()) {
                                QMutexLocker locker(&lock);
                                queueDestoryMediaInfo->dequeue();
                            }
                            isRunning = false;
                        });
                thread.detach();
            }
        }
    }

    /**
     * @brief bug-35165, 将构造时读取media信息的方式改为独立的方法
     * 以免造成构造对象时直接卡住
     */
    void start()
    {
        if (isStopState.load())
            return;

        mediaInfo->Option(__T("Thread"));
        mediaInfo->Option(__T("Width"), __T("Text"));
        mediaInfo->Option(__T("Height"), __T("Text"));
        mediaInfo->Option(__T("Duration"), __T("Text"));
        mediaInfo->Open(fileName.toStdWString());

        QPointer<DMediaInfoPrivate> me = this;
        std::thread thread([me]() {
            while (1) {
                if (!me)
                    break;
                if (me->isStopState.load())
                    break;
                if (me->mediaInfo->State_Get() == MediaInfoStateFinished) {
                    me->callback();
                    break;
                }
                std::chrono::milliseconds dura(200);
                std::this_thread::sleep_for(dura);
            }
        });
        thread.detach();
    }

    QString value(const QString &key, MediaInfoLib::stream_t type)
    {
        QString info = QString::fromStdWString(mediaInfo->Get(type, 0, key.toStdWString()));
        return info;
    }

public:
    QString fileName;
    QSharedPointer<MediaInfoLib::MediaInfo> mediaInfo { nullptr };
    DMediaInfo *q { nullptr };
    DMediaInfo::FinishedCallback callback = nullptr;
    std::atomic_bool isStopState;
};
END_IO_NAMESPACE

USING_IO_NAMESPACE
DMediaInfo::DMediaInfo(const QString &fileName)
    : d(new DMediaInfoPrivate(this, fileName))
{
}

DMediaInfo::~DMediaInfo()
{
}

QString DMediaInfo::value(const QString &key, DFileInfo::MediaType meidiaType /* = General*/)
{
    return d->value(key, static_cast<MediaInfoLib::stream_t>(meidiaType));
}

void DMediaInfo::startReadInfo(FinishedCallback callback)
{
    d->isStopState.store(false);
    d->callback = callback;
    d->start();
}

void DMediaInfo::stopReadInfo()
{
    d->isStopState.store(true);
    if (d->mediaInfo)
        d->mediaInfo->Close();
}
