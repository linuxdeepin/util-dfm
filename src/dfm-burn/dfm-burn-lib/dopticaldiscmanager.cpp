// SPDX-FileCopyrightText: 2022 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <dfm-burn/dopticaldiscmanager.h>
#include <dfm-burn/dopticaldiscinfo.h>

#include "private/dopticaldiscmanager_p.h"
#include "private/dxorrisoengine.h"
#include "private/dudfburnengine.h"

#include <QDebug>
#include <QUrl>
#include <QPointer>
#include <QProcess>
#include <QRegularExpression>

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
        ret = udfEngine->doBurn(dptr->curDev, dptr->files, volId, opts);
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

    // Determine media type to dispatch the appropriate erasure tool.
    // DVD-RW requires growisofs for a complete erase; xorriso's "as_needed"
    // mode leaves residual filesystem structures on certain drive+media combos.
    MediaType mediaType { MediaType::kNoMedia };
    {
        QScopedPointer<DOpticalDiscInfo> info { DOpticalDiscManager::createOpticalInfo(dptr->curDev) };
        if (info)
            mediaType = info->mediaType();
    }

    if (mediaType == MediaType::kDVD_RW) {
        ret = eraseWithGrowisofs();
    } else {
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
    }

    return ret;
}

bool DOpticalDiscManager::eraseWithGrowisofs()
{
    Q_EMIT jobStatusChanged(JobStatus::kRunning, 0, {}, {});

    QProcess growisofs;
    growisofs.setProcessChannelMode(QProcess::MergedChannels);

    // growisofs -Z <device>=/dev/zero overwrites the entire disc with zeros,
    // performing a complete erase that reliably clears DVD-RW media.
    growisofs.start("growisofs", { "-Z", dptr->curDev + "=/dev/zero" });

    if (!growisofs.waitForStarted()) {
        dptr->errorMsg = "[dfm-burn] Failed to start growisofs for DVD-RW erase";
        Q_EMIT jobStatusChanged(JobStatus::kFailed, -1, {}, { dptr->errorMsg });
        return false;
    }

    // Parse progress output (e.g. "  10.0% done (223670/2236704 KiB)").
    QRegularExpression re(R"((\d+\.\d+)%\s*done)");
    while (growisofs.state() != QProcess::NotRunning) {
        if (!growisofs.waitForReadyRead(30000))
            break;
        QByteArray output = growisofs.readAllStandardOutput();
        auto match = re.match(QString::fromLocal8Bit(output));
        if (match.hasMatch()) {
            int percentage = static_cast<int>(match.captured(1).toDouble());
            Q_EMIT jobStatusChanged(JobStatus::kRunning, percentage, {}, {});
        }
    }

    growisofs.waitForFinished(-1);

    if (growisofs.exitCode() != 0) {
        QString errMsg = QString("[dfm-burn] growisofs DVD-RW erase failed: %1")
                                 .arg(QString::fromLocal8Bit(growisofs.readAllStandardOutput()).trimmed());
        dptr->errorMsg = errMsg;
        Q_EMIT jobStatusChanged(JobStatus::kFailed, -1, {}, { errMsg });
        return false;
    }

    Q_EMIT jobStatusChanged(JobStatus::kFinished, 0, {}, {});
    return true;
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
