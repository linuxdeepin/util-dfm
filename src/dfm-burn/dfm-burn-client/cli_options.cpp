// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "cli_options.h"

#include <QFileInfo>
#include <iostream>

DFM_BURN_USE_NS

using namespace std;

// ── Main parse entry ───────────────────────────────────────────

bool CliOptions::parse(int argc, char *argv[], BurnCliConfig &config)
{
    if (argc < 2) {
        printHelp();
        return false;
    }

    // Collect remaining arguments (skip program name)
    QStringList args;
    for (int i = 2; i < argc; ++i)
        args.append(QString::fromLocal8Bit(argv[i]));

    QString cmd = QString::fromLocal8Bit(argv[1]);

    if (cmd == "--help" || cmd == "-h") {
        printHelp();
        return false;
    }

    // ── Route to subcommand parser ──
    if (cmd == "info")
        return parseInfoArgs(args, config);
    if (cmd == "burn")
        return parseBurnArgs(args, config);
    if (cmd == "write-iso")
        return parseWriteIsoArgs(args, config);
    if (cmd == "dump-iso")
        return parseDumpIsoArgs(args, config);
    if (cmd == "erase")
        return parseEraseArgs(args, config);
    if (cmd == "check")
        return parseCheckArgs(args, config);
    if (cmd == "checksum") {
        if (args.isEmpty()) {
            cerr << "Error: 'checksum' requires a subcommand (gen, verify)" << endl;
            cerr << "Run 'dfm-burner checksum --help' for details." << endl;
            return false;
        }
        QString checksumAction = args.takeFirst();
        if (checksumAction == "gen")
            return parseChecksumGenArgs(args, config);
        if (checksumAction == "verify")
            return parseChecksumVerifyArgs(args, config);
        if (checksumAction == "--help" || checksumAction == "-h") {
            printCommandHelp(BurnCommand::ChecksumGen);
            return false;
        }
        cerr << "Error: Unknown checksum action '" << checksumAction.toStdString() << "'" << endl;
        cerr << "Valid actions: gen, verify" << endl;
        return false;
    }
    if (cmd == "pw") {
        if (args.isEmpty()) {
            cerr << "Error: 'pw' requires a subcommand (open, close, put, mv, rm)" << endl;
            cerr << "Run 'dfm-burner pw --help' for details." << endl;
            return false;
        }
        QString pwAction = args.takeFirst();
        return parsePwArgs(pwAction, args, config);
    }

    cerr << "Error: Unknown command '" << cmd.toStdString() << "'" << endl;
    printHelp();
    return false;
}

// ── Help output ────────────────────────────────────────────────

void CliOptions::printHelp() const
{
    cout << "Usage: dfm-burner <command> [options] [arguments]" << endl;
    cout << endl;
    cout << "Optical disc burning tool for Deepin File Manager." << endl;
    cout << "Use 'dfm-burner info /dev/sr0' to check disc status before burning." << endl;
    cout << endl;
    cout << "Commands:" << endl;
    cout << "  info                Show disc info (type, capacity, free space)" << endl;
    cout << "  burn                Burn files or folders to disc" << endl;
    cout << "  write-iso           Write an .iso image to disc" << endl;
    cout << "  dump-iso            Read disc and save as .iso image" << endl;
    cout << "  erase               Erase a rewritable disc (CD-RW, DVD-RW, BD-RE)" << endl;
    cout << "  check               Scan disc for read errors" << endl;
    cout << "  checksum            SM3 checksum verification (gen / verify)" << endl;
    cout << "  pw                  Incremental file operations (UDF packet writing)" << endl;
    cout << endl;
    cout << "Tips:" << endl;
    cout << "  - Find your device:  ls /dev/sr*" << endl;
    cout << "  - Get started:       dfm-burner info /dev/sr0" << endl;
    cout << "  - Quick burn:        dfm-burner burn /dev/sr0 ./my-folder" << endl;
    cout << endl;
    cout << "Run 'dfm-burner <command> --help' for details." << endl;
}

void CliOptions::printCommandHelp(BurnCommand cmd) const
{
    switch (cmd) {
    case BurnCommand::Info:
        cout << "Usage: dfm-burner info [--json] <device>" << endl;
        cout << endl;
        cout << "Show disc type, capacity, used/free space, and write speeds." << endl;
        cout << "Run this first to check what disc is in the drive before burning." << endl;
        cout << endl;
        cout << "Arguments:" << endl;
        cout << "  device              Device path (e.g., /dev/sr0). Use 'ls /dev/sr*' to find it." << endl;
        cout << endl;
        cout << "Options:" << endl;
        cout << "  --json, -j          Output in JSON format (for scripting)" << endl;
        cout << endl;
        cout << "Examples:" << endl;
        cout << "  dfm-burner info /dev/sr0          # Check disc status" << endl;
        cout << "  dfm-burner info --json /dev/sr0   # Machine-readable output" << endl;
        break;

    case BurnCommand::Burn:
        cout << "Usage: dfm-burner burn [options] <device> <file_or_dir> [<file_or_dir>...]" << endl;
        cout << endl;
        cout << "Burn files and directories to an optical disc." << endl;
        cout << "Use 'dfm-burner info /dev/sr0' first to check disc type and free space." << endl;
        cout << endl;
        cout << "Arguments:" << endl;
        cout << "  device              Device path (e.g., /dev/sr0). Use 'ls /dev/sr*' to find it." << endl;
        cout << "  file_or_dir         One or more files or folders to burn." << endl;
        cout << endl;
        cout << "Options:" << endl;
        cout << "  --volume-id=<name>  Set disc label shown in file manager (default: ISOIMAGE)" << endl;
        cout << "  --speed=<number>    Write speed in CD/DVD multiplier. 0 = auto pick (default)" << endl;
        cout << endl;
        cout << "Filesystem (pick one, default is iso9660):" << endl;
        cout << "  --iso9660           Basic format, works on virtually all systems" << endl;
        cout << "  --joliet            Like iso9660 but supports long filenames and CJK" << endl;
        cout << "                        characters, recommended for Windows compatibility" << endl;
        cout << "  --rockridge         Like iso9660 but preserves Linux permissions and" << endl;
        cout << "                        symbolic links, recommended for Linux-only use" << endl;
        cout << "  --udf               Modern format, supports files > 2GB and Unicode" << endl;
        cout << "                        filenames. Best for large files or mixed-OS use." << endl;
        cout << "                        Combine with --appendable for multi-session." << endl;
        cout << endl;
        cout << "Behavior:" << endl;
        cout << "  --appendable        After burning, leave disc open so you can add more" << endl;
        cout << "                        data in a later session (multi-session disc)" << endl;
        cout << "  --verify            Read back all data after burning to detect errors." << endl;
        cout << "                        Slower but ensures data integrity." << endl;
        cout << endl;
        cout << "Examples:" << endl;
        cout << "  # Quick burn with defaults" << endl;
        cout << "  dfm-burner burn /dev/sr0 ./my-folder" << endl;
        cout << endl;
        cout << "  # Burn with a custom label" << endl;
        cout << "  dfm-burner burn --volume-id=Photos_2026 /dev/sr0 ./photos" << endl;
        cout << endl;
        cout << "  # Burn a 4GB file (needs UDF)" << endl;
        cout << "  dfm-burner burn --udf /dev/sr0 ./large-video.mkv" << endl;
        cout << endl;
        cout << "  # Multi-session: burn now, add more files later" << endl;
        cout << "  dfm-burner burn --udf --appendable /dev/sr0 ./batch1" << endl;
        cout << endl;
        cout << "  # Cross-platform: works on Windows, Linux, macOS" << endl;
        cout << "  dfm-burner burn --joliet --rockridge --verify /dev/sr0 ./data" << endl;
        break;

    case BurnCommand::WriteISO:
        cout << "Usage: dfm-burner write-iso [--speed=<number>] <device> <iso_path>" << endl;
        cout << endl;
        cout << "Write an .iso image file to disc (1:1 copy, disc-at-once)." << endl;
        cout << endl;
        cout << "Arguments:" << endl;
        cout << "  device              Device path (e.g., /dev/sr0). Use 'ls /dev/sr*' to find it." << endl;
        cout << "  iso_path            Path to the .iso file to burn." << endl;
        cout << endl;
        cout << "Options:" << endl;
        cout << "  --speed=<number>    Write speed. 0 = auto pick (default)" << endl;
        cout << endl;
        cout << "Examples:" << endl;
        cout << "  dfm-burner write-iso /dev/sr0 ./ubuntu-22.04.iso" << endl;
        cout << "  dfm-burner write-iso --speed=8 /dev/sr0 ./debian-live.iso" << endl;
        break;

    case BurnCommand::DumpISO:
        cout << "Usage: dfm-burner dump-iso <device> <output_path>" << endl;
        cout << endl;
        cout << "Read disc content and save it as an .iso image file (for backup or redistribution)." << endl;
        cout << endl;
        cout << "Arguments:" << endl;
        cout << "  device              Device path (e.g., /dev/sr0). Use 'ls /dev/sr*' to find it." << endl;
        cout << "  output_path         Where to save the .iso file." << endl;
        cout << endl;
        cout << "Examples:" << endl;
        cout << "  dfm-burner dump-iso /dev/sr0 ./backup.iso" << endl;
        break;

    case BurnCommand::Erase:
        cout << "Usage: dfm-burner erase <device>" << endl;
        cout << endl;
        cout << "Erase all data on a rewritable disc." << endl;
        cout << "Only works on rewritable media: CD-RW, DVD-RW, DVD+RW, BD-RE." << endl;
        cout << "CD-R and DVD+R are write-once and cannot be erased." << endl;
        cout << endl;
        cout << "Arguments:" << endl;
        cout << "  device              Device path (e.g., /dev/sr0). Use 'ls /dev/sr*' to find it." << endl;
        cout << endl;
        cout << "Examples:" << endl;
        cout << "  dfm-burner erase /dev/sr0" << endl;
        break;

    case BurnCommand::Check:
        cout << "Usage: dfm-burner check [--json] <device>" << endl;
        cout << endl;
        cout << "Read the entire disc to detect read errors and report quality statistics." << endl;
        cout << "Useful for verifying old backups or checking disc health." << endl;
        cout << endl;
        cout << "Arguments:" << endl;
        cout << "  device              Device path (e.g., /dev/sr0). Use 'ls /dev/sr*' to find it." << endl;
        cout << endl;
        cout << "Options:" << endl;
        cout << "  --json, -j          Output in JSON format (for scripting)" << endl;
        cout << endl;
        cout << "Examples:" << endl;
        cout << "  dfm-burner check /dev/sr0" << endl;
        cout << "  dfm-burner check --json /dev/sr0" << endl;
        break;

    case BurnCommand::ChecksumGen:
        cout << "Usage: dfm-burner checksum gen --output=<path> <source_dir>" << endl;
        cout << endl;
        cout << "Generate an SM3 checksum manifest for files before burning." << endl;
        cout << "The manifest records the SM3 hash of every file, which can later be" << endl;
        cout << "used to verify disc integrity with 'dfm-burner checksum verify'." << endl;
        cout << endl;
        cout << "Arguments:" << endl;
        cout << "  source_dir          Directory containing files to be burned." << endl;
        cout << endl;
        cout << "Options:" << endl;
        cout << "  --output=<path>     Where to save the manifest JSON (required)." << endl;
        cout << endl;
        cout << "Examples:" << endl;
        cout << "  dfm-burner checksum gen --output=manifest.json ./my-data" << endl;
        break;

    case BurnCommand::ChecksumVerify:
        cout << "Usage: dfm-burner checksum verify <device> <manifest.json>" << endl;
        cout << endl;
        cout << "Verify burned disc integrity by comparing SM3 checksums." << endl;
        cout << "Extracts all files from the disc and compares their SM3 hashes" << endl;
        cout << "against the manifest generated by 'dfm-burner checksum gen'." << endl;
        cout << endl;
        cout << "Arguments:" << endl;
        cout << "  device              Device path (e.g., /dev/sr0). Disc must still be in drive." << endl;
        cout << "  manifest.json       Path to the manifest file created during 'gen'." << endl;
        cout << endl;
        cout << "Examples:" << endl;
        cout << "  dfm-burner checksum verify /dev/sr0 manifest.json" << endl;
        break;

    case BurnCommand::PwOpen:
    case BurnCommand::PwClose:
    case BurnCommand::PwPut:
    case BurnCommand::PwMv:
    case BurnCommand::PwRm:
        cout << "Usage: dfm-burner pw <action> <device> <working_path> [extra_args...]" << endl;
        cout << endl;
        cout << "Packet writing (UDF) lets you use a rewritable disc like a USB drive:" << endl;
        cout << "add, rename, or remove individual files without rewriting the whole disc." << endl;
        cout << endl;
        cout << "Arguments:" << endl;
        cout << "  device              Device path (e.g., /dev/sr0). Use 'ls /dev/sr*' to find it." << endl;
        cout << "  working_path        Local directory for temporary staging files." << endl;
        cout << "                        Must be on the same filesystem as the files you put." << endl;
        cout << endl;
        cout << "Actions:" << endl;
        cout << "  open    Open disc for packet writing (must be called first)" << endl;
        cout << "  close   Finish packet writing and finalize disc" << endl;
        cout << "  put     Copy a file from your computer onto the disc" << endl;
        cout << "  mv      Rename a file already on the disc" << endl;
        cout << "  rm      Delete a file from the disc" << endl;
        cout << endl;
        cout << "Workflow:" << endl;
        cout << "  1. dfm-burner pw open  /dev/sr0 /tmp/pw-work" << endl;
        cout << "  2. dfm-burner pw put   /dev/sr0 /tmp/pw-work /home/user/doc.txt" << endl;
        cout << "  3. dfm-burner pw put   /dev/sr0 /tmp/pw-work /home/user/photo.jpg" << endl;
        cout << "  4. dfm-burner pw close /dev/sr0 /tmp/pw-work" << endl;
        break;

    default:
        printHelp();
        break;
    }
}

// ── Subcommand parsers ─────────────────────────────────────────

bool CliOptions::parseInfoArgs(const QStringList &args, BurnCliConfig &config) const
{
    config.command = BurnCommand::Info;

    for (const auto &arg : args) {
        if (arg == "--help" || arg == "-h") {
            printCommandHelp(BurnCommand::Info);
            return false;
        }
        if (arg == "--json" || arg == "-j") {
            config.jsonOutput = true;
        } else if (!arg.startsWith('-') && config.device.isEmpty()) {
            config.device = arg;
        } else {
            cerr << "Error: Unknown argument '" << arg.toStdString() << "'" << endl;
            return false;
        }
    }

    if (config.device.isEmpty()) {
        cerr << "Error: Device path is required." << endl;
        cerr << "Run 'dfm-burner info --help' for usage." << endl;
        return false;
    }
    return true;
}

bool CliOptions::parseBurnArgs(const QStringList &args, BurnCliConfig &config) const
{
    config.command = BurnCommand::Burn;
    bool hasFsOption = false;

    for (const auto &arg : args) {
        if (arg == "--help" || arg == "-h") {
            printCommandHelp(BurnCommand::Burn);
            return false;
        }
        if (arg.startsWith("--volume-id=")) {
            config.volumeId = arg.mid(12);
            if (config.volumeId.isEmpty()) {
                cerr << "Error: --volume-id requires a non-empty value." << endl;
                return false;
            }
        } else if (arg.startsWith("--speed=")) {
            bool ok = false;
            config.speed = arg.mid(8).toInt(&ok);
            if (!ok || config.speed < 0) {
                cerr << "Error: Invalid speed value '" << arg.mid(8).toStdString() << "'" << endl;
                return false;
            }
        } else if (arg == "--iso9660") {
            config.burnOptions |= BurnOption::kISO9660Only;
            hasFsOption = true;
        } else if (arg == "--joliet") {
            config.burnOptions |= BurnOption::kJolietSupport;
            hasFsOption = true;
        } else if (arg == "--rockridge") {
            config.burnOptions |= BurnOption::kRockRidgeSupport;
            hasFsOption = true;
        } else if (arg == "--udf") {
            config.burnOptions |= BurnOption::kUDF102Supported;
            hasFsOption = true;
        } else if (arg == "--appendable") {
            config.burnOptions |= BurnOption::kKeepAppendable;
        } else if (arg == "--verify") {
            config.burnOptions |= BurnOption::kVerifyDatas;
        } else if (!arg.startsWith('-')) {
            if (config.device.isEmpty())
                config.device = arg;
            else
                config.stageFiles << arg;
        } else {
            cerr << "Error: Unknown option '" << arg.toStdString() << "'" << endl;
            return false;
        }
    }

    // Default filesystem is ISO9660
    if (!hasFsOption)
        config.burnOptions |= BurnOption::kISO9660Only;

    if (config.device.isEmpty()) {
        cerr << "Error: Device path is required." << endl;
        cerr << "Run 'dfm-burner burn --help' for usage." << endl;
        return false;
    }
    if (config.stageFiles.isEmpty()) {
        cerr << "Error: At least one file or directory is required." << endl;
        cerr << "Run 'dfm-burner burn --help' for usage." << endl;
        return false;
    }

    // Validate stage files exist
    for (const auto &f : config.stageFiles) {
        if (!QFileInfo::exists(f)) {
            cerr << "Error: File not found: " << f.toStdString() << endl;
            return false;
        }
    }

    return true;
}

bool CliOptions::parseWriteIsoArgs(const QStringList &args, BurnCliConfig &config) const
{
    config.command = BurnCommand::WriteISO;

    for (const auto &arg : args) {
        if (arg == "--help" || arg == "-h") {
            printCommandHelp(BurnCommand::WriteISO);
            return false;
        }
        if (arg.startsWith("--speed=")) {
            bool ok = false;
            config.speed = arg.mid(8).toInt(&ok);
            if (!ok || config.speed < 0) {
                cerr << "Error: Invalid speed value '" << arg.mid(8).toStdString() << "'" << endl;
                return false;
            }
        } else if (!arg.startsWith('-')) {
            if (config.device.isEmpty())
                config.device = arg;
            else if (config.isoPath.isEmpty())
                config.isoPath = arg;
            else {
                cerr << "Error: Unexpected argument '" << arg.toStdString() << "'" << endl;
                return false;
            }
        } else {
            cerr << "Error: Unknown option '" << arg.toStdString() << "'" << endl;
            return false;
        }
    }

    if (config.device.isEmpty() || config.isoPath.isEmpty()) {
        cerr << "Error: Device and ISO path are required." << endl;
        cerr << "Run 'dfm-burner write-iso --help' for usage." << endl;
        return false;
    }
    if (!QFileInfo::exists(config.isoPath)) {
        cerr << "Error: ISO file not found: " << config.isoPath.toStdString() << endl;
        return false;
    }
    return true;
}

bool CliOptions::parseDumpIsoArgs(const QStringList &args, BurnCliConfig &config) const
{
    config.command = BurnCommand::DumpISO;

    for (const auto &arg : args) {
        if (arg == "--help" || arg == "-h") {
            printCommandHelp(BurnCommand::DumpISO);
            return false;
        }
        if (!arg.startsWith('-')) {
            if (config.device.isEmpty())
                config.device = arg;
            else if (config.outputPath.isEmpty())
                config.outputPath = arg;
            else {
                cerr << "Error: Unexpected argument '" << arg.toStdString() << "'" << endl;
                return false;
            }
        } else {
            cerr << "Error: Unknown option '" << arg.toStdString() << "'" << endl;
            return false;
        }
    }

    if (config.device.isEmpty() || config.outputPath.isEmpty()) {
        cerr << "Error: Device and output path are required." << endl;
        cerr << "Run 'dfm-burner dump-iso --help' for usage." << endl;
        return false;
    }
    return true;
}

bool CliOptions::parseEraseArgs(const QStringList &args, BurnCliConfig &config) const
{
    config.command = BurnCommand::Erase;

    for (const auto &arg : args) {
        if (arg == "--help" || arg == "-h") {
            printCommandHelp(BurnCommand::Erase);
            return false;
        }
        if (!arg.startsWith('-') && config.device.isEmpty()) {
            config.device = arg;
        } else {
            cerr << "Error: Unexpected argument '" << arg.toStdString() << "'" << endl;
            return false;
        }
    }

    if (config.device.isEmpty()) {
        cerr << "Error: Device path is required." << endl;
        cerr << "Run 'dfm-burner erase --help' for usage." << endl;
        return false;
    }
    return true;
}

bool CliOptions::parseCheckArgs(const QStringList &args, BurnCliConfig &config) const
{
    config.command = BurnCommand::Check;

    for (const auto &arg : args) {
        if (arg == "--help" || arg == "-h") {
            printCommandHelp(BurnCommand::Check);
            return false;
        }
        if (arg == "--json" || arg == "-j") {
            config.jsonOutput = true;
        } else if (!arg.startsWith('-') && config.device.isEmpty()) {
            config.device = arg;
        } else {
            cerr << "Error: Unknown argument '" << arg.toStdString() << "'" << endl;
            return false;
        }
    }

    if (config.device.isEmpty()) {
        cerr << "Error: Device path is required." << endl;
        cerr << "Run 'dfm-burner check --help' for usage." << endl;
        return false;
    }
    return true;
}

bool CliOptions::parseChecksumGenArgs(const QStringList &args, BurnCliConfig &config) const
{
    config.command = BurnCommand::ChecksumGen;

    for (const auto &arg : args) {
        if (arg == "--help" || arg == "-h") {
            printCommandHelp(BurnCommand::ChecksumGen);
            return false;
        }
        if (arg.startsWith("--output=")) {
            config.manifestPath = arg.mid(9);
            if (config.manifestPath.isEmpty()) {
                cerr << "Error: --output requires a non-empty value." << endl;
                return false;
            }
        } else if (!arg.startsWith('-')) {
            if (config.stageFiles.isEmpty())
                config.stageFiles << arg;
            else {
                cerr << "Error: Unexpected argument '" << arg.toStdString() << "'" << endl;
                return false;
            }
        } else {
            cerr << "Error: Unknown option '" << arg.toStdString() << "'" << endl;
            return false;
        }
    }

    if (config.manifestPath.isEmpty()) {
        cerr << "Error: --output=<path> is required." << endl;
        cerr << "Run 'dfm-burner checksum gen --help' for usage." << endl;
        return false;
    }
    if (config.stageFiles.isEmpty()) {
        cerr << "Error: Source directory is required." << endl;
        cerr << "Run 'dfm-burner checksum gen --help' for usage." << endl;
        return false;
    }
    if (!QFileInfo::exists(config.stageFiles.first())) {
        cerr << "Error: Source not found: " << config.stageFiles.first().toStdString() << endl;
        return false;
    }

    return true;
}

bool CliOptions::parseChecksumVerifyArgs(const QStringList &args, BurnCliConfig &config) const
{
    config.command = BurnCommand::ChecksumVerify;

    for (const auto &arg : args) {
        if (arg == "--help" || arg == "-h") {
            printCommandHelp(BurnCommand::ChecksumVerify);
            return false;
        }
        if (!arg.startsWith('-')) {
            if (config.device.isEmpty())
                config.device = arg;
            else if (config.manifestPath.isEmpty())
                config.manifestPath = arg;
            else {
                cerr << "Error: Unexpected argument '" << arg.toStdString() << "'" << endl;
                return false;
            }
        } else {
            cerr << "Error: Unknown option '" << arg.toStdString() << "'" << endl;
            return false;
        }
    }

    if (config.device.isEmpty()) {
        cerr << "Error: Device path is required." << endl;
        cerr << "Run 'dfm-burner checksum verify --help' for usage." << endl;
        return false;
    }
    if (config.manifestPath.isEmpty()) {
        cerr << "Error: Manifest path is required." << endl;
        cerr << "Run 'dfm-burner checksum verify --help' for usage." << endl;
        return false;
    }
    if (!QFileInfo::exists(config.manifestPath)) {
        cerr << "Error: Manifest not found: " << config.manifestPath.toStdString() << endl;
        return false;
    }

    return true;
}

bool CliOptions::parsePwArgs(const QString &pwAction, const QStringList &args, BurnCliConfig &config) const
{
    // Map action to command
    if (pwAction == "--help" || pwAction == "-h") {
        printCommandHelp(BurnCommand::PwOpen);
        return false;
    }

    if (pwAction == "open") {
        config.command = BurnCommand::PwOpen;
    } else if (pwAction == "close") {
        config.command = BurnCommand::PwClose;
    } else if (pwAction == "put") {
        config.command = BurnCommand::PwPut;
    } else if (pwAction == "mv") {
        config.command = BurnCommand::PwMv;
    } else if (pwAction == "rm") {
        config.command = BurnCommand::PwRm;
    } else {
        cerr << "Error: Unknown pw action '" << pwAction.toStdString() << "'" << endl;
        cerr << "Valid actions: open, close, put, mv, rm" << endl;
        return false;
    }

    // Parse positional arguments
    for (const auto &arg : args) {
        if (arg.startsWith('-')) {
            cerr << "Error: Unknown option '" << arg.toStdString() << "'" << endl;
            return false;
        }
        if (config.device.isEmpty())
            config.device = arg;
        else if (config.workingPath.isEmpty())
            config.workingPath = arg;
        else if (config.command == BurnCommand::PwPut && config.pwFileName.isEmpty())
            config.pwFileName = arg;
        else if (config.command == BurnCommand::PwRm && config.pwFileName.isEmpty())
            config.pwFileName = arg;
        else if (config.command == BurnCommand::PwMv && config.pwSrcName.isEmpty())
            config.pwSrcName = arg;
        else if (config.command == BurnCommand::PwMv && config.pwDestName.isEmpty())
            config.pwDestName = arg;
        else {
            cerr << "Error: Unexpected argument '" << arg.toStdString() << "'" << endl;
            return false;
        }
    }

    // Validate required arguments
    if (config.device.isEmpty() || config.workingPath.isEmpty()) {
        cerr << "Error: Device and working path are required." << endl;
        cerr << "Run 'dfm-burner pw --help' for usage." << endl;
        return false;
    }

    if (config.command == BurnCommand::PwPut && config.pwFileName.isEmpty()) {
        cerr << "Error: File name is required for 'put' action." << endl;
        return false;
    }
    if (config.command == BurnCommand::PwRm && config.pwFileName.isEmpty()) {
        cerr << "Error: File name is required for 'rm' action." << endl;
        return false;
    }
    if (config.command == BurnCommand::PwMv && (config.pwSrcName.isEmpty() || config.pwDestName.isEmpty())) {
        cerr << "Error: Source and destination names are required for 'mv' action." << endl;
        return false;
    }

    return true;
}
