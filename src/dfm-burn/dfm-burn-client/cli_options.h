// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef CLI_OPTIONS_H
#define CLI_OPTIONS_H

#include <QStringList>
#include <dfm-burn/dburn_global.h>

DFM_BURN_BEGIN_NS

/**
 * @brief Supported burn subcommands
 */
enum class BurnCommand {
    None,
    Info,
    Burn,
    WriteISO,
    DumpISO,
    Erase,
    Check,
    PwOpen,
    PwClose,
    PwPut,
    PwMv,
    PwRm
};

/**
 * @brief Parsed CLI configuration
 *
 * Fields are populated based on the active subcommand;
 * unrelated fields are left at their defaults.
 */
struct BurnCliConfig
{
    BurnCommand command = BurnCommand::None;
    QString device;
    bool jsonOutput = false;

    // burn options
    QStringList stageFiles;
    QString volumeId = "ISOIMAGE";
    int speed = 0;
    BurnOptions burnOptions;

    // write-iso / dump-iso paths
    QString isoPath;
    QString outputPath;

    // packet writing
    QString workingPath;
    QString pwSrcName;
    QString pwDestName;
    QString pwFileName;
};

/**
 * @brief Command-line option parser with subcommand routing
 *
 * Supports git-style subcommands: info, burn, write-iso, dump-iso,
 * erase, check, pw <open|close|put|mv|rm>.
 */
class CliOptions
{
public:
    CliOptions() = default;
    ~CliOptions() = default;

    /**
     * @brief Parse command-line arguments into config
     * @return true on success, false on error (message already printed)
     */
    bool parse(int argc, char *argv[], BurnCliConfig &config);

    void printHelp() const;

private:
    void printCommandHelp(BurnCommand cmd) const;

    // ── Per-subcommand parsers ──
    bool parseInfoArgs(const QStringList &args, BurnCliConfig &config) const;
    bool parseBurnArgs(const QStringList &args, BurnCliConfig &config) const;
    bool parseWriteIsoArgs(const QStringList &args, BurnCliConfig &config) const;
    bool parseDumpIsoArgs(const QStringList &args, BurnCliConfig &config) const;
    bool parseEraseArgs(const QStringList &args, BurnCliConfig &config) const;
    bool parseCheckArgs(const QStringList &args, BurnCliConfig &config) const;
    bool parsePwArgs(const QString &pwAction, const QStringList &args, BurnCliConfig &config) const;
};

DFM_BURN_END_NS

#endif   // CLI_OPTIONS_H
