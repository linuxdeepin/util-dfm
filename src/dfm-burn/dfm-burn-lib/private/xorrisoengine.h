/*
 * Copyright (C) 2022 Uniontech Software Technology Co., Ltd.
 *
 * Author:     zhangsheng<zhangsheng@uniontech.com>
 *
 * Maintainer: max-lv<lvwujun@uniontech.com>
 *             lanxuesong<lanxuesong@uniontech.com>
 *             xushitong<xushitong@uniontech.com>
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
#ifndef XORRISOENGINE_H
#define XORRISOENGINE_H

#include "dfmburn_global.h"

#include <QObject>

#include <xorriso.h>

DFM_BURN_BEGIN_NS

class XorrisoEngine : public QObject
{
    Q_OBJECT
public:
    explicit XorrisoEngine(QObject *parent = nullptr);
    ~XorrisoEngine();

    bool acquireDevice(QString dev);
    void releaseDevice();
    void clearResult();
    MediaType mediaTypeProperty() const;
    void mediaStorageProperty(quint64 *usedSize, quint64 *availSize, quint64 *blocks) const;
    bool mediaFormattedProperty() const;
    QString mediaVolIdProperty() const;
    QStringList mediaSpeedProperty() const;
    QStringList takeInfoMessages();
    QString getCurSpeed() const;

public slots:
    void messageReceived(int type, char *text);

signals:
    /**
     * \brief Indicates a change of current job status.
     *
     * \param status Current job status.
     * \param progress Job progress in percentage if status is running.
     *        Type of stalled work if status is stalled (1 = closing session, 0 = others).
     */
    void jobStatusChanged(JobStatus status, int progress);

private:
    XorrisO *xorriso;
    QString curDev;
    QStringList xorrisomsg;
    QString curspeed;
    quint64 curDatablocks;
};

DFM_BURN_END_NS

#endif   // XORRISOENGINE_H
