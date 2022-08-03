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
#include "dopticaldiscmanager.h"
#include "dopticaldiscinfo.h"
#include "private/dopticaldiscmanager_p.h"
#include "private/dxorrisoengine.h"
#include "private/dudfburnengine.h"

#include <QDebug>
#include <QUrl>
#include <QPointer>

DFM_BURN_USE_NS

DOpticalDiscManager::DOpticalDiscManager(const QString &dev, QObject *parent)
    : QObject(parent), dptr(new DOpticalDiscManagerPrivate)
{
    dptr->curDev = dev;
}

DOpticalDiscManager::~DOpticalDiscManager()
{
}

bool DOpticalDiscManager::setStageFile(const QString &diskPath, const QString &isoPath)
{
    QUrl diskUrl { diskPath };
    QUrl isoUrl { isoPath };

    if (diskUrl.isEmpty() || !diskUrl.isValid()) {
        dptr->errorMsg = "Invalid disk path";
        return false;
    }
    if (isoUrl.isEmpty() || !isoUrl.isValid()) {
        dptr->errorMsg = "Invalid iso path";
        return false;
    }

    dptr->files.first = diskPath;
    dptr->files.second = isoPath;

    return true;
}

/*!
 * \brief DISOMaster::commit  Burn all staged files to the disc.
 * \param opts   burning options
 * \param speed  desired writing speed in kilobytes per second
 * \param volId  volume name of the disc
 * \return       true on success, false on failure
 */
bool DOpticalDiscManager::commit(const BurnOptions &opts, int speed, const QString &volId)
{
    bool ret { false };

    if (opts.testFlag(BurnOption::kUDF102Supported)) {
        QScopedPointer<DUDFBurnEngine> udfEngine { new DUDFBurnEngine };
        connect(udfEngine.data(), &DUDFBurnEngine::jobStatusChanged, this,
                [this, ptr = QPointer(udfEngine.data())](JobStatus status, int progress) {
                    if (ptr) {
                        if (status == JobStatus::kFailed)
                            Q_EMIT jobStatusChanged(status, progress, {}, ptr->lastErrorMessage());
                        else
                            Q_EMIT jobStatusChanged(status, progress, {}, {});
                    }
                },
                Qt::DirectConnection);
        ret = udfEngine->doBurn(dptr->curDev, dptr->files, volId);
    } else {
        QScopedPointer<DXorrisoEngine> xorrisoEngine { new DXorrisoEngine };
        connect(xorrisoEngine.data(), &DXorrisoEngine::jobStatusChanged, this,
                [this, ptr = QPointer(xorrisoEngine.data())](JobStatus status, int progress, QString speed) {
                    if (ptr)
                        Q_EMIT jobStatusChanged(status, progress, speed, ptr->takeInfoMessages());
                },
                Qt::DirectConnection);

        if (!xorrisoEngine->acquireDevice(dptr->curDev))
            qWarning() << "[dfm-burn] Cannot acquire device";

        using XJolietSupport = DXorrisoEngine::JolietSupport;
        using XRockRageSupport = DXorrisoEngine::RockRageSupport;
        using XKeepAppendable = DXorrisoEngine::KeepAppendable;
        XJolietSupport joliet = opts.testFlag(BurnOption::kJolietSupport)
                ? XJolietSupport::kTrue
                : XJolietSupport::kFalse;
        XRockRageSupport rockRage = opts.testFlag(BurnOption::kRockRidgeSupport)
                ? XRockRageSupport::kTrue
                : XRockRageSupport::kFalse;
        XKeepAppendable keepAppendable = opts.testFlag(BurnOption::kKeepAppendable)
                ? XKeepAppendable::kTrue
                : XKeepAppendable::kFalse;

        ret = xorrisoEngine->doBurn(dptr->files, speed, volId, joliet, rockRage, keepAppendable);
        xorrisoEngine->releaseDevice();
    }

    return ret;
}

bool DOpticalDiscManager::erase()
{
    bool ret { false };
    QScopedPointer<DXorrisoEngine> engine { new DXorrisoEngine };
    connect(engine.data(), &DXorrisoEngine::jobStatusChanged, this,
            [this, ptr = QPointer(engine.data())](JobStatus status, int progress, QString speed) {
                if (ptr)
                    Q_EMIT jobStatusChanged(status, progress, speed, ptr->takeInfoMessages());
            },
            Qt::DirectConnection);

    if (!engine->acquireDevice(dptr->curDev))
        qWarning() << "[dfm-burn] Cannot acquire device";

    ret = engine->doErase();

    engine->releaseDevice();
    return ret;
}

bool DOpticalDiscManager::checkmedia(double *qgood, double *qslow, double *qbad)
{
    bool ret { false };
    quint64 blocks { 0 };

    {
        QScopedPointer<DOpticalDiscInfo> info { DOpticalDiscManager::createOpticalInfo(dptr->curDev) };
        if (!info)
            return ret;
        blocks = info->dataBlocks();
    }

    QScopedPointer<DXorrisoEngine> engine { new DXorrisoEngine };
    connect(engine.data(), &DXorrisoEngine::jobStatusChanged, this,
            [this, ptr = QPointer(engine.data())](JobStatus status, int progress, QString speed) {
                if (ptr)
                    Q_EMIT jobStatusChanged(status, progress, speed, ptr->takeInfoMessages());
            },
            Qt::DirectConnection);

    if (!engine->acquireDevice(dptr->curDev))
        qWarning() << "[dfm-burn] Cannot acquire device";

    ret = engine->doCheckmedia(blocks, qgood, qslow, qbad);

    engine->releaseDevice();

    return ret;
}

bool DOpticalDiscManager::writeISO(const QString &isoPath, int speed)
{
    bool ret { false };
    QScopedPointer<DXorrisoEngine> engine { new DXorrisoEngine };
    connect(engine.data(), &DXorrisoEngine::jobStatusChanged, this,
            [this, ptr = QPointer(engine.data())](JobStatus status, int progress, QString speed) {
                if (ptr)
                    Q_EMIT jobStatusChanged(status, progress, speed, ptr->takeInfoMessages());
            },
            Qt::DirectConnection);

    if (!engine->acquireDevice(dptr->curDev))
        qWarning() << "[dfm-burn] Cannot acquire device";

    if (QUrl(isoPath).isEmpty() || !QUrl(isoPath).isValid()) {
        dptr->errorMsg = QString("[dfm-burn]: Invalid path: %1 ").arg(isoPath);
        return ret;
    }

    ret = engine->doWriteISO(isoPath, speed);

    engine->releaseDevice();

    return ret;
}

bool DOpticalDiscManager::dumpISO(const QString &isoPath)
{
    bool ret { false };
    quint64 blocks { 0 };

    {
        QScopedPointer<DOpticalDiscInfo> info { DOpticalDiscManager::createOpticalInfo(dptr->curDev) };
        if (!info)
            return ret;
        blocks = info->dataBlocks();
    }

    QScopedPointer<DXorrisoEngine> engine { new DXorrisoEngine };
    connect(engine.data(), &DXorrisoEngine::jobStatusChanged, this,
            [this, ptr = QPointer(engine.data())](JobStatus status, int progress, QString speed) {
                if (ptr)
                    emit jobStatusChanged(status, progress, speed, ptr->takeInfoMessages());
            },
            Qt::DirectConnection);

    if (!engine->acquireDevice(dptr->curDev))
        qWarning() << "[dfm-burn] Cannot acquire device";

    if (QUrl(isoPath).isEmpty() || !QUrl(isoPath).isValid()) {
        dptr->errorMsg = QString("[dfm-burn]: Invalid path: %1 ").arg(isoPath);
        return ret;
    }

    ret = engine->doDumpISO(blocks, isoPath);

    engine->releaseDevice();

    return ret;
}

QString DOpticalDiscManager::lastError() const
{
    return dptr->errorMsg;
}

DOpticalDiscInfo *DOpticalDiscManager::createOpticalInfo(const QString &dev)
{
    auto info = new DOpticalDiscInfo(dev);
    if (info && info->device().isEmpty()) {
        delete info;
        return nullptr;
    }

    return info;
}
