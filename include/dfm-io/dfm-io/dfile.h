// SPDX-FileCopyrightText: 2020 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DFILE_H
#define DFILE_H

#include <dfm-io/dfmio_global.h>
#include <dfm-io/error/error.h>

#include <QUrl>
#include <QSharedPointer>

#include <functional>

BEGIN_IO_NAMESPACE

class DFileFuture;
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
        kBackup = (1 << 1),   // Make a backup of any existing files.
        kNoFollowSymlinks = (1 << 2),   // Don’t follow symlinks.
        kAllMetadata = (1 << 3),   // Copy all file metadata instead of just default set used for copy.
        kNoFallbackForMove = (1 << 4),   // Don’t use copy and delete fallback if native move not supported.
        kTargetDefaultPerms = (1 << 5),   // Leaves target file with default perms, instead of setting the source file perms.

        kUserFlag = 0x40
    };
    Q_DECLARE_FLAGS(CopyFlags, CopyFlag)

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

public:
    explicit DFile(const QUrl &uri);
    explicit DFile(const QString &path);
    ~DFile();
    QUrl uri() const;
    bool isOpen() const;
    qint64 size() const;
    bool exists() const;
    qint64 pos() const;
    Permissions permissions() const;
    DFMIOError lastError() const;

    bool open(OpenFlags mode);
    bool close();
    bool cancel();
    bool seek(qint64 pos, SeekType type = SeekType::kBegin) const;
    bool flush();
    bool setPermissions(Permissions permission);

    // read and write
    qint64 read(char *data, qint64 maxSize);
    QByteArray read(qint64 maxSize);
    QByteArray readAll();
    qint64 write(const char *data, qint64 len);
    qint64 write(const char *data);
    qint64 write(const QByteArray &byteArray);

    // async callback
    void readAsync(char *data, qint64 maxSize, int ioPriority = 0, ReadCallbackFunc func = nullptr, void *userData = nullptr);
    void readQAsync(qint64 maxSize, int ioPriority = 0, ReadQCallbackFunc func = nullptr, void *userData = nullptr);
    void readAllAsync(int ioPriority = 0, ReadAllCallbackFunc func = nullptr, void *userData = nullptr);
    void writeAsync(const char *data, qint64 maxSize, int ioPriority = 0, WriteCallbackFunc func = nullptr, void *userData = nullptr);
    void writeAllAsync(const char *data, int ioPriority = 0, WriteAllCallbackFunc func = nullptr, void *userData = nullptr);
    void writeQAsync(const QByteArray &byteArray, int ioPriority = 0, WriteQCallbackFunc func = nullptr, void *userData = nullptr);

    // future callback
    [[nodiscard]] DFileFuture *openAsync(OpenFlags mode, int ioPriority, QObject *parent = nullptr);
    [[nodiscard]] DFileFuture *closeAsync(int ioPriority, QObject *parent = nullptr);
    [[nodiscard]] DFileFuture *readAsync(quint64 maxSize, int ioPriority, QObject *parent = nullptr);
    [[nodiscard]] DFileFuture *readAllAsync(int ioPriority, QObject *parent = nullptr);
    [[nodiscard]] DFileFuture *writeAsync(const QByteArray &data, qint64 len, int ioPriority, QObject *parent = nullptr);
    [[nodiscard]] DFileFuture *writeAsync(const QByteArray &data, int ioPriority, QObject *parent = nullptr);
    [[nodiscard]] DFileFuture *flushAsync(int ioPriority, QObject *parent = nullptr);
    [[nodiscard]] DFileFuture *sizeAsync(int ioPriority, QObject *parent = nullptr);
    [[nodiscard]] DFileFuture *existsAsync(int ioPriority, QObject *parent = nullptr);
    [[nodiscard]] DFileFuture *permissionsAsync(int ioPriority, QObject *parent = nullptr);
    [[nodiscard]] DFileFuture *setPermissionsAsync(Permissions permission, int ioPriority, QObject *parent = nullptr);

private:
    QScopedPointer<DFilePrivate> d;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(DFile::OpenFlags);
Q_DECLARE_OPERATORS_FOR_FLAGS(DFile::Permissions);
Q_DECLARE_OPERATORS_FOR_FLAGS(DFile::CopyFlags);

END_IO_NAMESPACE
Q_DECLARE_METATYPE(dfmio::DFile::Permissions);
#endif   // DFILE_H
