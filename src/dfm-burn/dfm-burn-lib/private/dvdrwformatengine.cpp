// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dvdrwformatengine.h"

#include <QDebug>
#include <QProcess>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QDir>

DFM_BURN_USE_NS

DVDRwFormatEngine::DVDRwFormatEngine(QObject *parent)
    : QObject(parent)
{
}

DVDRwFormatEngine::~DVDRwFormatEngine()
{
}

bool DVDRwFormatEngine::doErase(const QString &dev, MediaType type)
{
    phase = 0;
    lastRawProgress = 0;

    // 1. 校验设备路径
    QString cleanDev = QDir::cleanPath(dev);
    if (!cleanDev.startsWith("/dev/sr")) {
        qWarning() << "[dfm-burn] Invalid device path:" << dev;
        Q_EMIT jobStatusChanged(JobStatus::kFailed, -1);
        return false;
    }

    // 2. 查找 dvd+rw-format 二进制
    QString binary = QStandardPaths::findExecutable("dvd+rw-format");
    if (binary.isEmpty()) {
        qWarning() << "[dfm-burn] dvd+rw-format not found";
        Q_EMIT jobStatusChanged(JobStatus::kFailed, -1);
        return false;
    }

    // 3. 根据介质类型选择命令参数
    QStringList args;
    if (type == MediaType::kDVD_PLUS_RW) {
        args << "-force"
             << "-gui"
             << cleanDev;
    } else if (type == MediaType::kDVD_RW) {
        args << "-blank=full"
             << "-gui"
             << cleanDev;
    } else {
        qWarning() << "[dfm-burn] Unsupported media type for dvd+rw-format:" << static_cast<int>(type);
        Q_EMIT jobStatusChanged(JobStatus::kFailed, -1);
        return false;
    }

    Q_EMIT jobStatusChanged(JobStatus::kRunning, 0);

    // dvd+rw-format 进度输出机制分析（源码 dvd+rw-format.cpp）：
    //
    // 1. fork() 后子进程 setsid() 执行 SCSI 操作，父进程安装 SIGALRM
    //    定时轮询共享内存中的 progress 值（0~65536），用退格符覆盖式
    //    输出百分比到 stderr。
    //
    // 2. -gui 标志的 gui_action 赋值在 fork 之后的子进程中（727 行），
    //    父进程的 gui_action 始终为 NULL（129 行），所以 -gui 对父进程
    //    的进度输出格式无影响——父进程始终用 \b 退格覆盖方式。
    //
    // 3. DVD+RW -force 路径有 3 个 wait_for_unit(progress) 调用
    //    （FORMAT UNIT → STOP DE-ICING → CLOSE SESSION），
    //    每个 SCSI 操作的进度指示器独立从 0 开始，
    //    导致原始输出出现 3 个 0→100% 周期。
    //    DVD-RW -blank 路径只有 1 个 wait_for_unit，单周期。
    //
    // 4. 进度数据通过 stderr 管道传输。QProcess::readyReadStandardOutput
    //    信号在 QEventLoop + MergedChannels 下可能不能及时触发（尤其在
    //    DVD-RW 单阶段输出稀疏时）。改用 waitForReadyRead 主动轮询
    //    管道——其内部调用 processEvents 处理 Qt 信号，进度可实时传递。

    int totalPhases = (type == MediaType::kDVD_PLUS_RW) ? 3 : 1;

    QProcess proc;
    proc.setProcessChannelMode(QProcess::MergedChannels);

    QByteArray buffer;

    proc.start(binary, args);
    if (!proc.waitForStarted()) {
        qWarning() << "[dfm-burn] Failed to start dvd+rw-format";
        Q_EMIT jobStatusChanged(JobStatus::kFailed, -1);
        return false;
    }

    // 主动轮询管道，不依赖 readyReadStandardOutput 信号。
    // waitForReadyRead 内部调用 processEvents，emit 的信号能被上层接收。
    const int pollInterval { 200 };   // ms

    while (proc.state() == QProcess::Running) {
        proc.waitForReadyRead(pollInterval);

        if (proc.bytesAvailable() > 0) {
            buffer += proc.readAllStandardOutput();
            parseProgress(buffer, totalPhases);
            if (buffer.size() > 1024)
                buffer = buffer.right(512);
        }
    }

    // 读取进程退出后管道中的剩余数据
    if (proc.bytesAvailable() > 0) {
        buffer += proc.readAllStandardOutput();
        parseProgress(buffer, totalPhases);
    }

    // 检查退出结果
    if (proc.exitStatus() == QProcess::CrashExit) {
        qWarning() << "[dfm-burn] dvd+rw-format crashed";
        Q_EMIT jobStatusChanged(JobStatus::kFailed, -1);
        return false;
    }

    int exitCode = proc.exitCode();
    if (exitCode == 0) {
        Q_EMIT jobStatusChanged(JobStatus::kFinished, 100);
        return true;
    } else {
        qWarning() << "[dfm-burn] dvd+rw-format exited with code" << exitCode;
        Q_EMIT jobStatusChanged(JobStatus::kFailed, -1);
        return false;
    }
}

void DVDRwFormatEngine::parseProgress(const QByteArray &data, int totalPhases)
{
    // dvd+rw-format 父进程用退格符覆盖式输出进度，格式如：
    //   0.0%\b\b\b\b0.1%\b|\b/\b\b0.2% ...
    // 也有部分行带换行符（子进程的诊断输出、父进程结束时的 \n）。
    // 在整个缓冲区中搜索最后一个 XX.X% 模式即可获取当前进度。
    static const QRegularExpression re("([0-9]+(?:\\.[0-9]+)?)%");
    auto matches = re.globalMatch(QString::fromLocal8Bit(data));
    QString lastPct;
    while (matches.hasNext())
        lastPct = matches.next().captured(1);
    if (lastPct.isEmpty())
        return;

    int rawProgress = static_cast<int>(lastPct.toDouble());

    // 检测相位切换：进度显著回退说明进入了下一个 SCSI 操作
    if (rawProgress < lastRawProgress - 5) {
        phase++;
        if (phase >= totalPhases)
            phase = totalPhases - 1;
    }
    lastRawProgress = rawProgress;

    // 加权计算总体进度：(phase * 100 + rawProgress) / totalPhases
    int overall = (phase * 100 + rawProgress) / totalPhases;
    if (overall < 100)
        Q_EMIT jobStatusChanged(JobStatus::kRunning, overall);
}
