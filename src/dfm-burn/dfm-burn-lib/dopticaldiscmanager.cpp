// SPDX-FileCopyrightText: 2022 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <dfm-burn/dopticaldiscmanager.h>
#include <dfm-burn/dopticaldiscinfo.h>

#include "private/dopticaldiscmanager_p.h"
#include "private/dxorrisoengine.h"
#include "private/dudfburnengine.h"
#include "private/dsm3hash.h"

#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
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

static const QString kExtractCacheSubDir = "discburn";

static QString extractCacheDir(const QString &dev)
{
    QString safeDev = dev;
    safeDev.replace("/", "_");
    // Remove leading underscore caused by the leading "/" in device path
    if (safeDev.startsWith("_"))
        safeDev.remove(0, 1);
    return QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation)
            + "/" + kExtractCacheSubDir + "/extract_" + safeDev;
}

// xorriso extracts files with read-only permissions (dr-xr-xr-x),
// so we must chmod recursively before removal.
static void cleanupExtractCache(QDir dir)
{
    if (!dir.exists())
        return;
    QDirIterator it(dir.absolutePath(), QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot,
                     QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QString path = it.next();
        QFile::setPermissions(path, QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner
                                     | QFile::ReadGroup | QFile::ExeGroup
                                     | QFile::ReadOther | QFile::ExeOther);
    }
    dir.removeRecursively();
}

bool DOpticalDiscManager::generateChecksumManifest(const QString &savePath)
{
    QString srcPath = dptr->files.first;
    if (srcPath.isEmpty()) {
        dptr->errorMsg = "No staged file for checksum generation";
        return false;
    }

    QFileInfo srcInfo(srcPath);
    if (!srcInfo.exists()) {
        dptr->errorMsg = QString("Source path does not exist: %1").arg(srcPath);
        return false;
    }

    QString isoBase = dptr->files.second;
    if (isoBase.isEmpty())
        isoBase = "/";

    // Normalize isoBase: ensure it ends with /
    if (!isoBase.endsWith("/"))
        isoBase += "/";

    QJsonObject filesObj;

    if (srcInfo.isDir()) {
        QDirIterator it(srcPath, QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            QString filePath = it.next();
            QString relPath = QDir(srcPath).relativeFilePath(filePath);
            QString isoRelPath = isoBase + relPath;

            QString hash = DSM3Hash::sm3File(filePath);
            if (hash.isEmpty()) {
                dptr->errorMsg = QString("Failed to compute SM3 for: %1").arg(filePath);
                return false;
            }
            filesObj[isoRelPath] = hash;
        }
    } else {
        QString hash = DSM3Hash::sm3File(srcPath);
        if (hash.isEmpty()) {
            dptr->errorMsg = QString("Failed to compute SM3 for: %1").arg(srcPath);
            return false;
        }
        filesObj[isoBase + srcInfo.fileName()] = hash;
    }

    if (filesObj.isEmpty()) {
        dptr->errorMsg = "No files found to checksum";
        return false;
    }

    QJsonObject root;
    root["algorithm"] = "sm3";
    root["device"] = dptr->curDev;
    root["files"] = filesObj;

    QFile outFile(savePath);
    if (!outFile.open(QIODevice::WriteOnly)) {
        dptr->errorMsg = QString("Cannot write manifest to: %1").arg(savePath);
        return false;
    }

    outFile.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    return true;
}

bool DOpticalDiscManager::verifyChecksum(const QString &manifestPath)
{
    QFile mf(manifestPath);
    if (!mf.open(QIODevice::ReadOnly)) {
        dptr->errorMsg = QString("Cannot read manifest: %1").arg(manifestPath);
        return false;
    }

    QJsonParseError parseErr;
    QJsonObject root = QJsonDocument::fromJson(mf.readAll(), &parseErr).object();
    if (root.isEmpty()) {
        dptr->errorMsg = QString("Invalid manifest JSON: %1").arg(parseErr.errorString());
        return false;
    }

    QJsonObject filesObj = root["files"].toObject();
    if (filesObj.isEmpty()) {
        dptr->errorMsg = "Manifest contains no file entries";
        return false;
    }

    // Prepare temp directory for per-file extraction
    QString cacheBase = extractCacheDir(dptr->curDev);
    QDir cacheDir(cacheBase);
    if (cacheDir.exists())
        cleanupExtractCache(cacheDir);
    if (!cacheDir.mkpath(".")) {
        dptr->errorMsg = QString("Cannot create cache directory: %1").arg(cacheBase);
        return false;
    }

    // Acquire device and enable osirrox once for all per-file extractions
    QScopedPointer<DXorrisoEngine> engine { new DXorrisoEngine };
    connect(engine.data(), &DXorrisoEngine::jobStatusChanged, this,
            [this, ptr = QPointer(engine.data())](JobStatus status, int progress, QString speed) {
                if (ptr)
                    Q_EMIT jobStatusChanged(status, progress, speed, ptr->takeInfoMessages());
            },
            Qt::DirectConnection);

    if (!engine->acquireDevice(dptr->curDev)) {
        dptr->errorMsg = "Cannot acquire device for verification";
        cleanupExtractCache(cacheDir);
        return false;
    }

    if (!engine->doOsirroxOn()) {
        dptr->errorMsg = QString("Failed to enable osirrox: %1").arg(engine->takeInfoMessages().join(" "));
        engine->releaseDevice();
        cleanupExtractCache(cacheDir);
        return false;
    }

    // Extract and verify each file individually:
    // extract → compute SM3 → delete → next file
    // This avoids extracting the entire disc (multi-session friendly)
    // and minimizes disk usage (only one file at a time).
    QStringList isoPaths = filesObj.keys();
    int total = isoPaths.size();
    int checked = 0;

    for (const auto &isoPath : isoPaths) {
        QString localTmp = cacheBase + isoPath;

        // Ensure parent directory exists
        QDir().mkpath(QFileInfo(localTmp).absolutePath());

        if (!engine->doExtract(localTmp, isoPath)) {
            dptr->errorMsg = QString("Failed to extract '%1' from disc: %2")
                                     .arg(isoPath, engine->takeInfoMessages().join(" "));
            engine->releaseDevice();
            cleanupExtractCache(cacheDir);
            return false;
        }

        QString expectedHash = filesObj[isoPath].toString();
        QString actualHash = DSM3Hash::sm3File(localTmp);

        // Delete immediately to free disk space and avoid permission issues on cleanup
        QFile::remove(localTmp);

        ++checked;
        int pct = (total > 0) ? (100 * checked / total) : 0;
        Q_EMIT jobStatusChanged(JobStatus::kRunning, pct, {}, { isoPath });

        if (actualHash.isEmpty()) {
            dptr->errorMsg = QString("Checksum mismatch: file not found on disc: %1").arg(isoPath);
            engine->releaseDevice();
            cleanupExtractCache(cacheDir);
            return false;
        }

        if (actualHash != expectedHash) {
            dptr->errorMsg = QString("Checksum mismatch: %1 (expected %2, got %3)")
                                     .arg(isoPath, expectedHash, actualHash);
            engine->releaseDevice();
            cleanupExtractCache(cacheDir);
            return false;
        }
    }

    engine->releaseDevice();

    Q_EMIT jobStatusChanged(JobStatus::kFinished, 100, {}, {});

    // Clean up cache directory structure
    cleanupExtractCache(cacheDir);

    return true;
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
