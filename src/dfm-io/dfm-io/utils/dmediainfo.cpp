/*
 * Copyright (C) 2022 Uniontech Software Technology Co., Ltd.
 *
 * Author:     lanxuesong<lanxuesong@uniontech.com>
 *
 * Maintainer: lanxuesong<lanxuesong@uniontech.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "dmediainfo.h"

#include <MediaInfo/MediaInfo.h>

#include <QTimer>
#include <QQueue>
#include <QMutex>
#include <QDebug>

#include <thread>

const size_t MediaInfoStateFinished = 10000;   // read finished and no error

Q_GLOBAL_STATIC(QQueue<QSharedPointer<MediaInfoLib::MediaInfo>>, queueDestoryMediaInfo)

BEGIN_IO_NAMESPACE
class DMediaInfoPrivate : public QSharedData
{
public:
    DMediaInfoPrivate(DMediaInfo *qq, const QString &fileName)
        : q(qq)
    {
        isWorking.store(false);
        isStopState.store(false);

        this->fileName = fileName;
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
        if (isWorking.load())
            return;
        isWorking.store(true);

        mediaInfo->Option(__T("Thread"), __T("1"));   // open file in thread
        mediaInfo->Option(__T("Inform"), __T("Text"));
        if (mediaInfo->Open(fileName.toStdWString()) == 0) {   // 可能耗时
            std::thread thread([this]() {
                while (1) {
                    if (isStopState.load())
                        break;

                    if (mediaInfo) {
                        if (mediaInfo->State_Get() == MediaInfoStateFinished) {
                            callback();
                            break;
                        }
                        std::chrono::milliseconds dura(200);
                        std::this_thread::sleep_for(dura);
                    }
                }
            });
            thread.detach();
        }
        isWorking.store(false);
    }

    QString value(const QString &key, MediaInfoLib::stream_t type)
    {
        QString info = QString::fromStdWString(mediaInfo->Get(type, 0, key.toStdWString()));
        return info;
    }

public:
    std::atomic_bool isWorking;
    std::atomic_bool isStopState;
    QString fileName;
    QSharedPointer<MediaInfoLib::MediaInfo> mediaInfo { nullptr };
    DMediaInfo *q { nullptr };
    DMediaInfo::FinishedCallback callback = nullptr;
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
    d->callback = callback;
    d->start();
}

void DMediaInfo::stopReadInfo()
{
    d->isStopState.store(true);
    if (d->mediaInfo)
        d->mediaInfo->Close();
}
