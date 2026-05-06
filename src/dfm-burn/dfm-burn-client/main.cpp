// SPDX-FileCopyrightText: 2022 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QCoreApplication>
#include <QFileInfo>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <functional>
#include <iomanip>
#include <iostream>

#include <dfm-burn/dopticaldiscmanager.h>
#include <dfm-burn/dopticaldiscinfo.h>
#include <dfm-burn/dpacketwritingcontroller.h>

#include "cli_options.h"
#include "burn_utils.h"

DFM_BURN_USE_NS

using namespace std;

// ── Helpers ────────────────────────────────────────────────────

/**
 * @brief Run an async operation that reports progress via jobStatusChanged.
 *
 * @param config       Parsed CLI config (must have .device set)
 * @param app          QCoreApplication reference (for event loop)
 * @param startOp      Lambda that starts the operation; returns false on immediate failure
 * @param opName       Human-readable operation name for progress display
 * @return Exit code (0 = success, 1 = failure)
 */
static int runAsyncJob(const BurnCliConfig &config, QCoreApplication &app,
                       const function<bool(DOpticalDiscManager &)> &startOp,
                       const QString &opName)
{
    DOpticalDiscManager manager(config.device);
    int exitCode = 1;

    QObject::connect(&manager, &DOpticalDiscManager::jobStatusChanged,
                     [&exitCode, &app, &opName](JobStatus status, int progress,
                                                const QString &speed, const QStringList &message) {
                         if (status == JobStatus::kRunning) {
                             cout << "\r" << opName.toStdString() << "... " << progress << "%";
                             if (!speed.isEmpty())
                                 cout << " (" << speed.toStdString() << ")";
                             cout << flush;
                         } else if (status == JobStatus::kFinished) {
                             cout << "\r" << opName.toStdString() << "... Done.                    " << endl;
                             exitCode = 0;
                             app.quit();
                         } else if (status == JobStatus::kFailed) {
                             cerr << endl
                                  << "Operation failed: " << message.join(" ").toStdString() << endl;
                             exitCode = 1;
                             app.quit();
                         } else if (status == JobStatus::kStalled) {
                             cout << "\r" << opName.toStdString() << "... Stalled (waiting for drive)." << flush;
                         }
                     });

    if (!startOp(manager))
        return 1;

    return app.exec();
}

// ── Command: info (sync) ──────────────────────────────────────

static int handleInfo(const BurnCliConfig &config)
{
    QScopedPointer<DOpticalDiscInfo> info(DOpticalDiscManager::createOpticalInfo(config.device));
    if (!info) {
        cerr << "Error: Failed to get disc info for " << config.device.toStdString() << endl;
        return 1;
    }

    if (info->mediaType() == MediaType::kNoMedia) {
        cerr << "Error: No disc detected in " << config.device.toStdString() << endl;
        cerr << "  Insert a disc and try again." << endl;
        return 1;
    }

    if (config.jsonOutput) {
        QJsonObject obj;
        obj["device"] = info->device();
        obj["mediaType"] = mediaTypeName(info->mediaType());
        obj["blank"] = info->blank();
        obj["volumeName"] = info->volumeName();
        obj["usedSize"] = static_cast<qint64>(info->usedSize());
        obj["availableSize"] = static_cast<qint64>(info->availableSize());
        obj["totalSize"] = static_cast<qint64>(info->totalSize());
        obj["dataBlocks"] = static_cast<qint64>(info->dataBlocks());
        QJsonArray speeds;
        for (const auto &s : info->writeSpeed())
            speeds.append(s);
        obj["writeSpeeds"] = speeds;
        cout << QJsonDocument(obj).toJson(QJsonDocument::Indented).toStdString();
    } else {
        cout << "Device:       " << info->device().toStdString() << endl;
        cout << "Media Type:   " << mediaTypeName(info->mediaType()).toStdString()
             << (isRewritable(info->mediaType()) ? " (rewritable)" : " (write-once)") << endl;
        cout << "Blank:        " << (info->blank() ? "Yes" : "No") << endl;
        cout << "Volume Name:  " << (info->volumeName().isEmpty() ? "(empty)" : info->volumeName().toStdString()) << endl;
        cout << "Used:         " << formatSize(info->usedSize()).toStdString() << endl;
        cout << "Available:    " << formatSize(info->availableSize()).toStdString() << endl;
        cout << "Total:        " << formatSize(info->totalSize()).toStdString() << endl;
        cout << "Data Blocks:  " << info->dataBlocks() << endl;

        QStringList speeds = info->writeSpeed();
        cout << "Write Speeds: " << (speeds.isEmpty() ? "(none)" : speeds.join(", ").toStdString()) << endl;

        // Actionable hints
        cout << endl;
        if (info->blank()) {
            cout << ">> Disc is blank and ready to burn." << endl;
            cout << "   Quick start: dfm-burner burn " << info->device().toStdString() << " ./your-folder" << endl;
        } else if (info->availableSize() > 0) {
            cout << ">> Disc has free space (" << formatSize(info->availableSize()).toStdString() << ")." << endl;
            cout << "   To add data: dfm-burner burn --appendable " << info->device().toStdString() << " ./your-folder" << endl;
            cout << "   To start fresh, erase first: dfm-burner erase " << info->device().toStdString() << endl;
        } else {
            cout << ">> Disc is full. To reuse, erase first: dfm-burner erase " << info->device().toStdString() << endl;
        }
    }

    return 0;
}

// ── Command: burn (async) ──────────────────────────────────────

static int handleBurn(const BurnCliConfig &config, QCoreApplication &app)
{
    auto startOp = [&config](DOpticalDiscManager &manager) -> bool {
        for (const auto &file : config.stageFiles) {
            if (!manager.setStageFile(file)) {
                cerr << "Error: Could not add '" << file.toStdString() << "' to burn queue." << endl;
                cerr << "  " << manager.lastError().toStdString() << endl;
                return false;
            }
        }

        cout << "Device:   " << config.device.toStdString() << endl;
        cout << "Volume:   " << config.volumeId.toStdString() << endl;
        cout << "Mode:     " << burnOptionsSummary(config.burnOptions).toStdString() << endl;
        cout << "Files:    " << config.stageFiles.join(", ").toStdString() << endl;
        cout << endl;

        if (!manager.commit(config.burnOptions, config.speed, config.volumeId)) {
            cerr << "Error: Failed to start burn operation." << endl;
            cerr << "  " << manager.lastError().toStdString() << endl;
            return false;
        }
        return true;
    };

    return runAsyncJob(config, app, startOp, "Burning");
}

// ── Command: write-iso (async) ─────────────────────────────────

static int handleWriteISO(const BurnCliConfig &config, QCoreApplication &app)
{
    QFileInfo isoInfo(config.isoPath);
    cout << "Device:   " << config.device.toStdString() << endl;
    cout << "ISO:      " << config.isoPath.toStdString() << endl;
    cout << "Size:     " << formatSize(isoInfo.size()).toStdString() << endl;
    cout << endl;

    auto startOp = [&config](DOpticalDiscManager &manager) -> bool {
        if (!manager.writeISO(config.isoPath, config.speed)) {
            cerr << "Error: " << manager.lastError().toStdString() << endl;
            return false;
        }
        return true;
    };

    return runAsyncJob(config, app, startOp, "Writing ISO");
}

// ── Command: dump-iso (async) ──────────────────────────────────

static int handleDumpISO(const BurnCliConfig &config, QCoreApplication &app)
{
    cout << "Device:   " << config.device.toStdString() << endl;
    cout << "Output:   " << config.outputPath.toStdString() << endl;
    cout << endl;

    auto startOp = [&config](DOpticalDiscManager &manager) -> bool {
        if (!manager.dumpISO(config.outputPath)) {
            cerr << "Error: " << manager.lastError().toStdString() << endl;
            return false;
        }
        return true;
    };

    return runAsyncJob(config, app, startOp, "Dumping");
}

// ── Command: erase (async) ─────────────────────────────────────

static int handleErase(const BurnCliConfig &config, QCoreApplication &app)
{
    cout << "Device: " << config.device.toStdString() << endl;
    cout << endl;

    auto startOp = [](DOpticalDiscManager &manager) -> bool {
        if (!manager.erase()) {
            cerr << "Error: " << manager.lastError().toStdString() << endl;
            return false;
        }
        return true;
    };

    return runAsyncJob(config, app, startOp, "Erasing");
}

// ── Command: check (async, custom handler) ─────────────────────

static int handleCheck(const BurnCliConfig &config, QCoreApplication &app)
{
    DOpticalDiscManager manager(config.device);
    int exitCode = 1;
    double good = 0, slow = 0, bad = 0;

    QObject::connect(&manager, &DOpticalDiscManager::jobStatusChanged,
                     [&exitCode, &app, &config, &good, &slow, &bad](
                             JobStatus status, int progress,
                             const QString &speed, const QStringList &message) {
                         if (status == JobStatus::kRunning) {
                             cout << "\rChecking... " << progress << "%" << flush;
                         } else if (status == JobStatus::kFinished) {
                             cout << endl;
                             bool passed = !(bad > (2 + 1e-6));

                             if (config.jsonOutput) {
                                 QJsonObject obj;
                                 obj["good"] = good;
                                 obj["slow"] = slow;
                                 obj["bad"] = bad;
                                 obj["passed"] = passed;
                                 cout << QJsonDocument(obj).toJson(QJsonDocument::Indented).toStdString();
                             } else {
                                 cout << fixed << setprecision(1);
                                 cout << "Quality:" << endl;
                                 cout << "  Good: " << good << "%" << endl;
                                 cout << "  Slow: " << slow << "%" << endl;
                                 cout << "  Bad:  " << bad << "%" << endl;
                                 cout << endl;
                                 cout << "Result: " << (passed ? "PASS" : "FAIL") << endl;
                             }

                             exitCode = passed ? 0 : 1;
                             app.quit();
                         } else if (status == JobStatus::kFailed) {
                             cerr << endl
                                  << "Check failed: " << message.join(" ").toStdString() << endl;
                             exitCode = 1;
                             app.quit();
                         }
                     });

    if (!manager.checkmedia(&good, &slow, &bad)) {
        cerr << "Error: " << manager.lastError().toStdString() << endl;
        return 1;
    }

    return app.exec();
}

// ── Packet Writing Commands (sync) ─────────────────────────────

static int handlePwOpen(const BurnCliConfig &config)
{
    DPacketWritingController controller(config.device, config.workingPath);

    if (!controller.open()) {
        cerr << "Error: Failed to open packet writing session." << endl;
        cerr << "  " << controller.lastError().toStdString() << endl;
        return 1;
    }

    cout << "Packet writing session opened." << endl;
    cout << "  Device:  " << config.device.toStdString() << endl;
    cout << "  Working: " << config.workingPath.toStdString() << endl;
    return 0;
}

static int handlePwClose(const BurnCliConfig &config)
{
    DPacketWritingController controller(config.device, config.workingPath);
    controller.close();

    cout << "Packet writing session closed." << endl;
    return 0;
}

static int handlePwPut(const BurnCliConfig &config)
{
    DPacketWritingController controller(config.device, config.workingPath);

    if (!controller.put(config.pwFileName)) {
        cerr << "Error: Failed to add file." << endl;
        cerr << "  " << controller.lastError().toStdString() << endl;
        return 1;
    }

    cout << "Added: " << config.pwFileName.toStdString() << endl;
    return 0;
}

static int handlePwMv(const BurnCliConfig &config)
{
    DPacketWritingController controller(config.device, config.workingPath);

    if (!controller.mv(config.pwSrcName, config.pwDestName)) {
        cerr << "Error: Failed to rename." << endl;
        cerr << "  " << controller.lastError().toStdString() << endl;
        return 1;
    }

    cout << "Renamed: " << config.pwSrcName.toStdString()
         << " -> " << config.pwDestName.toStdString() << endl;
    return 0;
}

static int handlePwRm(const BurnCliConfig &config)
{
    DPacketWritingController controller(config.device, config.workingPath);

    if (!controller.rm(config.pwFileName)) {
        cerr << "Error: Failed to remove." << endl;
        cerr << "  " << controller.lastError().toStdString() << endl;
        return 1;
    }

    cout << "Removed: " << config.pwFileName.toStdString() << endl;
    return 0;
}

// ── Entry point ────────────────────────────────────────────────

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    app.setApplicationName("dfm-burner");
    app.setApplicationVersion("1.0.0");

    CliOptions cli;
    BurnCliConfig config;
    if (!cli.parse(argc, argv, config))
        return 1;

    switch (config.command) {
    case BurnCommand::Info:
        return handleInfo(config);
    case BurnCommand::Burn:
        return handleBurn(config, app);
    case BurnCommand::WriteISO:
        return handleWriteISO(config, app);
    case BurnCommand::DumpISO:
        return handleDumpISO(config, app);
    case BurnCommand::Erase:
        return handleErase(config, app);
    case BurnCommand::Check:
        return handleCheck(config, app);
    case BurnCommand::PwOpen:
        return handlePwOpen(config);
    case BurnCommand::PwClose:
        return handlePwClose(config);
    case BurnCommand::PwPut:
        return handlePwPut(config);
    case BurnCommand::PwMv:
        return handlePwMv(config);
    case BurnCommand::PwRm:
        return handlePwRm(config);
    default:
        return 1;
    }
}
