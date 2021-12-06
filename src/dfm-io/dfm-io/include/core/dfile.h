/*
 * Copyright (C) 2020 ~ 2021 Uniontech Software Technology Co., Ltd.
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
#ifndef DFILE_H
#define DFILE_H

#include "dfmio_global.h"

#include "error/error.h"

#include <QUrl>
#include <QSharedPointer>

#include <functional>

BEGIN_IO_NAMESPACE

class DFilePrivate;

class DFile
{
public:
    enum class OpenFlag : uint16_t {
        NotOpen = 0x0000,
        ReadOnly = 0x0001,
        WriteOnly = 0x0002,   // auto create
        ReadWrite = ReadOnly | WriteOnly,   // auto create
        Append = 0x0004,
        Truncate = 0x0008,
        Text = 0x0010,
        Unbuffered = 0x0020,
        NewOnly = 0x0040,
        ExistingOnly = 0x0080,

        CustomStart = 0x0100,
    };
    Q_DECLARE_FLAGS(OpenFlags, OpenFlag)

    enum class CopyFlag : uint8_t {
        copyNone,   // No flags set.
        copyOverwrite,   // Overwrite any existing files.
        copyBackup,   // Make a backup of any existing files.
        copyNofollowSymlinks,   // Don’t follow symlinks.
        copyAllMetadata,   // Copy all file metadata instead of just default set used for copy.
        copyNoFallbackForMove,   // Don’t use copy and delete fallback if native move not supported.
        copyTargetDefaultPerms,   // Leaves target file with default perms, instead of setting the source file perms.
    };

    enum class DFMSeekType : uint8_t {
        BEGIN = 0,
        CURRENT = 1,
        END = 2
    };

    enum class Permission : uint16_t {
        NoPermission = 0x0000,

        ExeOther = 0x0001,
        WriteOther = 0x0002,
        ReadOther = 0x0004,

        ExeGroup = 0x0010,
        WriteGroup = 0x0020,
        ReadGroup = 0x0040,

        ExeUser = 0x0100,
        WriteUser = 0x0200,
        ReadUser = 0x0400,

        ExeOwner = 0x1000,
        WriteOwner = 0x2000,
        ReadOwner = 0x4000,
    };

    // interface
    using OpenFunc = std::function<bool(OpenFlags)>;
    using CloseFunc = std::function<bool()>;
    // read
    using ReadFunc = std::function<qint64(char *, qint64)>;
    using ReadQFunc = std::function<QByteArray(qint64)>;
    using ReadAllFunc = std::function<QByteArray()>;
    // write
    using WriteFunc = std::function<qint64(const char *, qint64)>;
    using WriteAllFunc = std::function<qint64(const char *)>;
    using WriteQFunc = std::function<qint64(const QByteArray &)>;

    using SeekFunc = std::function<bool(qint64, DFMSeekType)>;
    using PosFunc = std::function<qint64()>;
    using FlushFunc = std::function<bool()>;
    using SizeFunc = std::function<qint64()>;
    using ExistsFunc = std::function<bool()>;

    using PermissionFunc = std::function<uint16_t(Permission)>;
    using SetPermissionFunc = std::function<bool(uint16_t)>;
    using LastErrorFunc = std::function<DFMIOError()>;

public:
    DFile(const QUrl &uri);
    virtual ~DFile();

    DFM_VIRTUAL bool open(OpenFlags mode);
    DFM_VIRTUAL bool close();

    DFM_VIRTUAL qint64 read(char *data, qint64 maxSize);
    DFM_VIRTUAL QByteArray read(qint64 maxSize);
    DFM_VIRTUAL QByteArray readAll();

    DFM_VIRTUAL qint64 write(const char *data, qint64 len);
    DFM_VIRTUAL qint64 write(const char *data);
    DFM_VIRTUAL qint64 write(const QByteArray &byteArray);

    DFM_VIRTUAL bool seek(qint64 pos, DFMSeekType type = DFMSeekType::BEGIN);
    DFM_VIRTUAL qint64 pos();
    DFM_VIRTUAL bool flush();
    DFM_VIRTUAL qint64 size();
    DFM_VIRTUAL bool exists();

    DFM_VIRTUAL uint16_t permissions(Permission permission = Permission::NoPermission);
    DFM_VIRTUAL bool setPermissions(const uint16_t mode);

    DFM_VIRTUAL DFMIOError lastError() const;

    // register
    void registerOpen(const OpenFunc &func);
    void registerClose(const CloseFunc &func);
    void registerRead(const ReadFunc &func);
    void registerReadQ(const ReadQFunc &func);
    void registerReadAll(const ReadAllFunc &func);
    void registerWrite(const WriteFunc &func);
    void registerWriteAll(const WriteAllFunc &func);
    void registerWriteQ(const WriteQFunc &func);
    void registerSeek(const SeekFunc &func);
    void registerPos(const PosFunc &func);
    void registerFlush(const FlushFunc &func);
    void registerSize(const SizeFunc &func);
    void registerExists(const ExistsFunc &func);
    void registerPermissions(const PermissionFunc &func);
    void registerSetPermissions(const SetPermissionFunc &func);
    void registerLastError(const LastErrorFunc &func);

    QUrl uri() const;
    bool isOpen();

private:
    QSharedPointer<DFilePrivate> d = nullptr;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(DFile::OpenFlags);

END_IO_NAMESPACE

#endif   // DFILE_H
