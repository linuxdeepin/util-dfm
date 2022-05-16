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
        kNotOpen = 0x0000,
        kReadOnly = 0x0001,
        kWriteOnly = 0x0002,   // auto create
        kReadWrite = kReadOnly | kWriteOnly,   // auto create
        kAppend = 0x0004,
        kTruncate = 0x0008,
        kText = 0x0010,
        kUnbuffered = 0x0020,
        kNewOnly = 0x0040,
        kExistingOnly = 0x0080,

        kCustomStart = 0x0100,
    };
    Q_DECLARE_FLAGS(OpenFlags, OpenFlag)

    enum class CopyFlag : uint8_t {
        kNone = 0,   // No flags set.
        kOverwrite = 1,   // Overwrite any existing files.
        kBackup = 2,   // Make a backup of any existing files.
        kNoFollowSymlinks = 3,   // Don’t follow symlinks.
        kAllMetadata = 4,   // Copy all file metadata instead of just default set used for copy.
        kNoFallbackForMove = 5,   // Don’t use copy and delete fallback if native move not supported.
        kTargetDefaultPerms = 6,   // Leaves target file with default perms, instead of setting the source file perms.

        kUserFlag = 0x10
    };

    enum class SeekType : uint8_t {
        kBegin = 0x00,
        kCurrent = 0x01,
        kEnd = 0x02
    };

    enum class Permission : uint16_t {
        kNoPermission = 0x0000,

        kExeOther = 0x0001,
        kWriteOther = 0x0002,
        kReadOther = 0x0004,

        kExeGroup = 0x0010,
        kWriteGroup = 0x0020,
        kReadGroup = 0x0040,

        kExeUser = 0x0100,
        kWriteUser = 0x0200,
        kReadUser = 0x0400,

        kExeOwner = 0x1000,
        kWriteOwner = 0x2000,
        kReadOwner = 0x4000,
    };
    Q_DECLARE_FLAGS(Permissions, Permission)

    // callback, use function pointer
    using ReadCallbackFunc = void (*)(qint64, void *);
    using ReadQCallbackFunc = void (*)(QByteArray, void *);
    using ReadAllCallbackFunc = void (*)(QByteArray, void *);

    using WriteCallbackFunc = void (*)(qint64, void *);
    using WriteAllCallbackFunc = void (*)(qint64, void *);
    using WriteQCallbackFunc = void (*)(qint64, void *);

    // register function, use std::function
    // interface
    using OpenFunc = std::function<bool(OpenFlags)>;
    using CloseFunc = std::function<bool()>;
    // read
    using ReadFunc = std::function<qint64(char *, qint64)>;
    using ReadQFunc = std::function<QByteArray(qint64)>;
    using ReadAllFunc = std::function<QByteArray()>;
    using ReadFuncAsync = std::function<void(char *, qint64, int, ReadCallbackFunc, void *)>;
    using ReadQFuncAsync = std::function<void(qint64, int, ReadQCallbackFunc, void *)>;
    using ReadAllFuncAsync = std::function<void(int, ReadAllCallbackFunc, void *)>;

    // write
    using WriteFunc = std::function<qint64(const char *, qint64)>;
    using WriteAllFunc = std::function<qint64(const char *)>;
    using WriteQFunc = std::function<qint64(const QByteArray &)>;
    using WriteFuncAsync = std::function<void(const char *, qint64, int, WriteCallbackFunc, void *)>;
    using WriteAllFuncAsync = std::function<void(const char *, int, WriteAllCallbackFunc, void *)>;
    using WriteQFuncAsync = std::function<void(const QByteArray &, int, WriteQCallbackFunc, void *)>;

    using SeekFunc = std::function<bool(qint64, SeekType)>;
    using PosFunc = std::function<qint64()>;
    using FlushFunc = std::function<bool()>;
    using SizeFunc = std::function<qint64()>;
    using ExistsFunc = std::function<bool()>;

    using PermissionFunc = std::function<Permissions()>;
    using SetPermissionFunc = std::function<bool(Permissions)>;
    using LastErrorFunc = std::function<DFMIOError()>;
    using SetErrorFunc = std::function<void(DFMIOError)>;

public:
    DFile(const QUrl &uri);
    virtual ~DFile();

    DFM_VIRTUAL bool open(OpenFlags mode);
    DFM_VIRTUAL bool close();

    DFM_VIRTUAL qint64 read(char *data, qint64 maxSize);
    DFM_VIRTUAL QByteArray read(qint64 maxSize);
    DFM_VIRTUAL QByteArray readAll();
    // async
    DFM_VIRTUAL void readAsync(char *data, qint64 maxSize, int ioPriority = 0, ReadCallbackFunc func = nullptr, void *userData = nullptr);
    DFM_VIRTUAL void readQAsync(qint64 maxSize, int ioPriority = 0, ReadQCallbackFunc func = nullptr, void *userData = nullptr);
    DFM_VIRTUAL void readAllAsync(int ioPriority = 0, ReadAllCallbackFunc func = nullptr, void *userData = nullptr);

    DFM_VIRTUAL qint64 write(const char *data, qint64 len);
    DFM_VIRTUAL qint64 write(const char *data);
    DFM_VIRTUAL qint64 write(const QByteArray &byteArray);
    // async
    DFM_VIRTUAL void writeAsync(const char *data, qint64 len, int ioPriority = 0, WriteCallbackFunc func = nullptr, void *userData = nullptr);
    DFM_VIRTUAL void writeAllAsync(const char *data, int ioPriority = 0, WriteAllCallbackFunc func = nullptr, void *userData = nullptr);
    DFM_VIRTUAL void writeQAsync(const QByteArray &byteArray, int ioPriority = 0, WriteQCallbackFunc func = nullptr, void *userData = nullptr);

    DFM_VIRTUAL bool seek(qint64 pos, SeekType type = SeekType::kBegin);
    DFM_VIRTUAL qint64 pos();
    DFM_VIRTUAL bool flush();
    DFM_VIRTUAL qint64 size();
    DFM_VIRTUAL bool exists();

    DFM_VIRTUAL Permissions permissions() const;
    DFM_VIRTUAL bool setPermissions(Permissions permission);

    DFM_VIRTUAL DFMIOError lastError() const;

    // register
    void registerOpen(const OpenFunc &func);
    void registerClose(const CloseFunc &func);
    void registerRead(const ReadFunc &func);
    void registerReadQ(const ReadQFunc &func);
    void registerReadAll(const ReadAllFunc &func);
    void registerReadAsync(const ReadFuncAsync &func);
    void registerReadQAsync(const ReadQFuncAsync &func);
    void registerReadAllAsync(const ReadAllFuncAsync &func);

    void registerWrite(const WriteFunc &func);
    void registerWriteAll(const WriteAllFunc &func);
    void registerWriteQ(const WriteQFunc &func);
    void registerWriteAsync(const WriteFuncAsync &func);
    void registerWriteAllAsync(const WriteAllFuncAsync &func);
    void registerWriteQAsync(const WriteQFuncAsync &func);

    void registerSeek(const SeekFunc &func);
    void registerPos(const PosFunc &func);
    void registerFlush(const FlushFunc &func);
    void registerSize(const SizeFunc &func);
    void registerExists(const ExistsFunc &func);
    void registerPermissions(const PermissionFunc &func);
    void registerSetPermissions(const SetPermissionFunc &func);
    void registerLastError(const LastErrorFunc &func);
    void registerSetError(const SetErrorFunc &func);

    QUrl uri() const;
    bool isOpen();

private:
    QSharedPointer<DFilePrivate> d = nullptr;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(DFile::OpenFlags);
Q_DECLARE_OPERATORS_FOR_FLAGS(DFile::Permissions);

END_IO_NAMESPACE

#endif   // DFILE_H
