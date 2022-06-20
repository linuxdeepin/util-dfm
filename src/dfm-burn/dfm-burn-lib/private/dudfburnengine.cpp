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
#include "dudfburnengine.h"

#include <QDebug>
#include <QDir>
#include <QRegularExpression>
#include <QStandardPaths>

DFM_BURN_USE_NS

namespace {
typedef struct
{
    long totalSize;
    long wroteSize;
    double progress;
} ProgressInfo;

// this is defined for callback register
std::function<void(const ProgressInfo *)> udfProgressCbBinder;
extern "C" void udProgressCbProxy(const ProgressInfo *info)
{
    udfProgressCbBinder(info);
}

extern "C" {
typedef void (*progress_cb)(const ProgressInfo *);
typedef void (*uburn_init)();
typedef int (*uburn_do_burn)(const char *dev, const char *file, const char *lable);
typedef void (*uburn_regi_cb)(progress_cb cb);
typedef char **(*uburn_get_errors)(int *);
typedef void (*uburn_show_verbose)();
typedef void (*uburn_redirect_output)(int redir_stdout, int redir_stderr);

static uburn_init ub_init = nullptr;
static uburn_do_burn ub_do_burn = nullptr;
static uburn_regi_cb ub_regi_cb = nullptr;
static uburn_get_errors ub_get_errors = nullptr;
static uburn_show_verbose ub_show_verbose = nullptr;
static uburn_redirect_output ub_redirect_output = nullptr;
}

}   // namespace

DUDFBurnEngine::DUDFBurnEngine(QObject *parent)
    : QObject(parent)
{
    lib.setFileName("udfburn");
    if (lib.isLoaded())
        return;
    libLoaded = lib.load();
    if (!libLoaded) {
        qWarning() << "[dfm-burn] Cannot load lib: " << lib.fileName();
        return;
    }
    qDebug() << lib.fileName();
    ub_init = reinterpret_cast<uburn_init>(lib.resolve("burn_init"));
    funcsLoaded &= (ub_init != nullptr);

    ub_do_burn = reinterpret_cast<uburn_do_burn>(lib.resolve("burn_burn_to_disc"));
    funcsLoaded &= (ub_do_burn != nullptr);

    ub_regi_cb = reinterpret_cast<uburn_regi_cb>(lib.resolve("burn_register_progress_callback"));
    funcsLoaded &= (ub_regi_cb != nullptr);

    ub_show_verbose = reinterpret_cast<uburn_show_verbose>(lib.resolve("burn_show_verbose_information"));
    funcsLoaded &= (ub_show_verbose != nullptr);

    ub_redirect_output = reinterpret_cast<uburn_redirect_output>(lib.resolve("burn_redirect_output"));
    funcsLoaded &= (ub_redirect_output != nullptr);

    ub_get_errors = reinterpret_cast<uburn_get_errors>(lib.resolve("burn_get_last_errors"));
    funcsLoaded &= (ub_get_errors != nullptr);

    qInfo() << "[dfm-burn] udf load lib " << (libLoaded ? "success" : "failed");
    qInfo() << "[dfm-burn] udf load func " << (funcsLoaded ? "success" : "failed");
}

DUDFBurnEngine::~DUDFBurnEngine()
{
    if (libLoaded)
        lib.unload();
}

bool DUDFBurnEngine::doBurn(const QString &dev, const QPair<QString, QString> files, QString volId)
{
    if (!canSafeUse())
        return false;

    Q_EMIT jobStatusChanged(JobStatus::kStalled, 0);

    udfProgressCbBinder = [this](const ProgressInfo *info) {
        Q_EMIT jobStatusChanged(JobStatus::kRunning, static_cast<int>(info->progress));
    };
    ub_init();
    ub_regi_cb(udProgressCbProxy);
    ub_show_verbose();
    ub_redirect_output(1, 0);

    int ret = ub_do_burn(dev.toStdString().c_str(), files.first.toStdString().c_str(),
                         volId.toStdString().c_str());

    // burn failed
    if (ret != 0) {
        int err_count = 0;
        char **errors = ub_get_errors(&err_count);
        if (errors != nullptr && err_count > 0) {
            QStringList errMsg;
            for (int i = err_count - 1; i >= 0; i--)
                errMsg.append(errors[i]);
            message = errMsg;
        }
        message.append(readErrorsFromLog());
        Q_EMIT jobStatusChanged(JobStatus::kFailed, 100);
        return false;
    }

    Q_EMIT jobStatusChanged(JobStatus::kFinished, 100);

    return true;
}

QStringList DUDFBurnEngine::lastErrorMessage() const
{
    return message;
}

bool DUDFBurnEngine::canSafeUse() const
{
    return libLoaded && funcsLoaded;
}

QStringList DUDFBurnEngine::readErrorsFromLog() const
{
    auto &&homePaths { QStandardPaths::standardLocations(QStandardPaths::HomeLocation) };
    if (homePaths.isEmpty())
        return {};

    auto &&logPath { homePaths.at(0) + "/.cache/deepin/discburn/uburn/" };
    QDir logDir(logPath);
    if (!logDir.exists())
        return {};

    auto &&burns { logDir.entryList(QDir::AllEntries | QDir::NoDotAndDotDot, QDir::SortFlag::Time) };
    if (burns.count() == 0)
        return {};

    auto &&lastBurn { burns.first() };
    auto &&logFilePath { logPath + lastBurn + "/log" };
    QFile logFile(logFilePath);
    if (!logFile.exists())
        return {};

    QStringList ret;
    logFile.open(QIODevice::ReadOnly | QIODevice::Text);
    while (!logFile.atEnd()) {
        QString &&line(logFile.readLine());
        if (line.startsWith("Warning") || line.startsWith("Error")) {
            line.remove(QRegularExpression("/home/.*/.cache/deepin/discburn/_dev_sr[0-9]*/"));
            ret << line;
        }
    }
    logFile.close();
    return ret;
}
