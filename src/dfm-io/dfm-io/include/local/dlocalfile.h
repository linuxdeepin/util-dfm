// SPDX-FileCopyrightText: 2020 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DLOCALFILE_H
#define DLOCALFILE_H

#include "dfmio_global.h"

#include "core/dfile.h"

BEGIN_IO_NAMESPACE

class DLocalFilePrivate;

class DLocalFile : public DFile
{
public:
    explicit DLocalFile(const QUrl &uri);
    virtual ~DLocalFile();

    bool open(DFile::OpenFlags mode) DFM_OVERRIDE;
    bool close() DFM_OVERRIDE;

    qint64 read(char *data, qint64 maxSize) DFM_OVERRIDE;
    QByteArray read(qint64 maxSize) DFM_OVERRIDE;
    QByteArray readAll() DFM_OVERRIDE;
    // async
    void readAsync(char *data, qint64 maxSize, int ioPriority = 0, ReadCallbackFunc func = nullptr, void *userData = nullptr) DFM_OVERRIDE;
    void readQAsync(qint64 maxSize, int ioPriority = 0, ReadQCallbackFunc func = nullptr, void *userData = nullptr) DFM_OVERRIDE;
    void readAllAsync(int ioPriority = 0, ReadAllCallbackFunc func = nullptr, void *userData = nullptr) DFM_OVERRIDE;

    qint64 write(const char *data, qint64 len) DFM_OVERRIDE;
    qint64 write(const char *data) DFM_OVERRIDE;
    qint64 write(const QByteArray &byteArray) DFM_OVERRIDE;
    // async
    void writeAsync(const char *data, qint64 len, int ioPriority = 0, WriteCallbackFunc func = nullptr, void *userData = nullptr) DFM_OVERRIDE;
    void writeAllAsync(const char *data, int ioPriority = 0, WriteAllCallbackFunc func = nullptr, void *userData = nullptr) DFM_OVERRIDE;
    void writeQAsync(const QByteArray &byteArray, int ioPriority = 0, WriteQCallbackFunc func = nullptr, void *userData = nullptr) DFM_OVERRIDE;

    bool cancel() const DFM_OVERRIDE;
    bool seek(qint64 pos, DFile::SeekType type = SeekType::kBegin) const DFM_OVERRIDE;
    qint64 pos() const DFM_OVERRIDE;
    bool flush() DFM_OVERRIDE;
    qint64 size() const DFM_OVERRIDE;
    bool exists() const DFM_OVERRIDE;
    Permissions permissions() const DFM_OVERRIDE;
    bool setPermissions(Permissions permission) DFM_OVERRIDE;

    // future
    [[nodiscard]] DFileFuture *openAsync(OpenFlags mode, int ioPriority, QObject *parent = nullptr) DFM_OVERRIDE;
    [[nodiscard]] DFileFuture *closeAsync(int ioPriority, QObject *parent = nullptr) DFM_OVERRIDE;
    [[nodiscard]] DFileFuture *readAsync(qint64 maxSize, int ioPriority, QObject *parent = nullptr) DFM_OVERRIDE;
    [[nodiscard]] DFileFuture *readAllAsync(int ioPriority, QObject *parent = nullptr) DFM_OVERRIDE;
    [[nodiscard]] DFileFuture *writeAsync(const QByteArray &data, qint64 len, int ioPriority, QObject *parent = nullptr) DFM_OVERRIDE;
    [[nodiscard]] DFileFuture *writeAsync(const QByteArray &data, int ioPriority, QObject *parent = nullptr) DFM_OVERRIDE;
    [[nodiscard]] DFileFuture *flushAsync(int ioPriority, QObject *parent = nullptr) DFM_OVERRIDE;
    [[nodiscard]] DFileFuture *sizeAsync(int ioPriority, QObject *parent = nullptr) DFM_OVERRIDE;
    [[nodiscard]] DFileFuture *existsAsync(int ioPriority, QObject *parent = nullptr) DFM_OVERRIDE;
    [[nodiscard]] DFileFuture *permissionsAsync(int ioPriority, QObject *parent = nullptr) DFM_OVERRIDE;
    [[nodiscard]] DFileFuture *setPermissionsAsync(Permissions permission, int ioPriority, QObject *parent = nullptr) DFM_OVERRIDE;

    void setError(DFMIOError error);
    DFMIOError lastError() const DFM_OVERRIDE;

private:
    QSharedPointer<DLocalFilePrivate> d = nullptr;
};

END_IO_NAMESPACE

#endif   // DLOCALFILE_H
