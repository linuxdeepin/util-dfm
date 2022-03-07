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
#include "opticaldiscmanager.h"
#include "opticaldiscinfo.h"
#include "private/opticaldiscmanager_p.h"
#include "private/xorrisoengine.h"
#include "private/udfburnengine.h"

#include <QDebug>
#include <QUrl>
#include <QPointer>

DFM_BURN_USE_NS

OpticalDiscManager::OpticalDiscManager(const QString &dev, QObject *parent)
    : QObject(parent), dptr(new OpticalDiscManagerPrivate)
{
    dptr->curDev = dev;
}

OpticalDiscManager::~OpticalDiscManager()
{
}

bool OpticalDiscManager::setStageFile(const QString &diskPath, const QString &isoPath)
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
bool OpticalDiscManager::commit(const BurnOptions &opts, int speed, const QString &volId)
{
    bool ret { false };

    if (opts.testFlag(BurnOption::kUDF102Supported)) {
        QScopedPointer<UDFBurnEngine> udfEngine { new UDFBurnEngine };
        connect(udfEngine.data(), &UDFBurnEngine::jobStatusChanged, this,
                [this, ptr = QPointer(udfEngine.data())](JobStatus status, int progress) {
                    if (ptr) {
                        if (status == JobStatus::kFailed)
                            emit jobStatusChanged(status, progress, {}, ptr->lastErrorMessage());
                        else
                            emit jobStatusChanged(status, progress, {}, {});
                    }
                },
                Qt::DirectConnection);
        ret = udfEngine->doBurn(dptr->curDev, dptr->files, volId);
    } else {
        QScopedPointer<XorrisoEngine> xorrisoEngine { new XorrisoEngine };
        connect(xorrisoEngine.data(), &XorrisoEngine::jobStatusChanged, this,
                [this, ptr = QPointer(xorrisoEngine.data())](JobStatus status, int progress, QString speed) {
                    if (ptr)
                        emit jobStatusChanged(status, progress, speed, ptr->takeInfoMessages());
                },
                Qt::DirectConnection);

        if (!xorrisoEngine->acquireDevice(dptr->curDev)) {
            dptr->errorMsg = "Cannot acquire device";
            return ret;
        }

        using XJolietSupport = XorrisoEngine::JolietSupport;
        using XRockRageSupport = XorrisoEngine::RockRageSupport;
        using XKeepAppendable = XorrisoEngine::KeepAppendable;
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

bool OpticalDiscManager::erase()
{
    bool ret { false };
    QScopedPointer<XorrisoEngine> engine { new XorrisoEngine };
    connect(engine.data(), &XorrisoEngine::jobStatusChanged, this,
            [this, ptr = QPointer(engine.data())](JobStatus status, int progress, QString speed) {
                if (ptr)
                    emit jobStatusChanged(status, progress, speed, ptr->takeInfoMessages());
            },
            Qt::DirectConnection);

    if (!engine->acquireDevice(dptr->curDev)) {
        dptr->errorMsg = "Cannot acquire device";
        return ret;
    }

    ret = engine->doErase();

    engine->releaseDevice();
    return ret;
}

bool OpticalDiscManager::checkmedia(double *qgood, double *qslow, double *qbad)
{
    bool ret { false };
    quint64 blocks { 0 };

    {
        QScopedPointer<OpticalDiscInfo> info { OpticalDiscManager::createOpticalInfo(dptr->curDev) };
        if (!info)
            return ret;
        blocks = info->dataBlocks();
    }

    QScopedPointer<XorrisoEngine> engine { new XorrisoEngine };
    connect(engine.data(), &XorrisoEngine::jobStatusChanged, this,
            [this, ptr = QPointer(engine.data())](JobStatus status, int progress, QString speed) {
                if (ptr)
                    emit jobStatusChanged(status, progress, speed, ptr->takeInfoMessages());
            },
            Qt::DirectConnection);
    if (!engine->acquireDevice(dptr->curDev)) {
        dptr->errorMsg = "Cannot acquire device";
        return ret;
    }

    ret = engine->doCheckmedia(blocks, qgood, qslow, qbad);

    engine->releaseDevice();

    return ret;
}

bool OpticalDiscManager::writeISO(const QString &isoPath, int speed)
{
    bool ret { false };
    QScopedPointer<XorrisoEngine> engine { new XorrisoEngine };
    connect(engine.data(), &XorrisoEngine::jobStatusChanged, this,
            [this, ptr = QPointer(engine.data())](JobStatus status, int progress, QString speed) {
                if (ptr)
                    emit jobStatusChanged(status, progress, speed, ptr->takeInfoMessages());
            },
            Qt::DirectConnection);

    if (!engine->acquireDevice(dptr->curDev)) {
        dptr->errorMsg = "Cannot acquire device";
        return ret;
    }

    if (QUrl(isoPath).isEmpty() || !QUrl(isoPath).isValid()) {
        dptr->errorMsg = QString("[dfm-burn]: Invalid path: %1 ").arg(isoPath);
        return ret;
    }

    ret = engine->doWriteISO(isoPath, speed);

    engine->releaseDevice();

    return ret;
}

QString OpticalDiscManager::lastError() const
{
    return dptr->errorMsg;
}

OpticalDiscInfo *OpticalDiscManager::createOpticalInfo(const QString &dev)
{
    auto info = new OpticalDiscInfo(dev);
    if (info->device().isEmpty())
        return nullptr;

    return info;
}
