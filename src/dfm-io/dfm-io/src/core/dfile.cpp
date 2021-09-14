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
#include "core/dfile_p.h"

#include <gio/gio.h>

#include <QDebug>

USING_IO_NAMESPACE


DFilePrivate::DFilePrivate(DFile *q)
    : q(q)
{

}

DFilePrivate::~DFilePrivate()
{

}

DFile::DFile(const QUrl &uri)
    : d(new DFilePrivate(this))
{
    d->uri = uri;
}

DFile::~DFile()
{
}

DFM_VIRTUAL bool DFile::open(DFile::OpenFlag mode)
{
    if (!d->openFunc)
        return false;

    d->isOpen = d->openFunc(mode);

    return d->isOpen;
}

DFM_VIRTUAL bool DFile::close()
{
    if (!d->closeFunc)
        return false;

    if (d->isOpen) {
        if (d->closeFunc())
            d->isOpen = false;
        else
            return false;
    }

    return true;
}

DFM_VIRTUAL qint64 DFile::read(char *data, qint64 maxSize)
{
    if (!d->readFunc)
        return -1;

    if (!d->isOpen) {
        qWarning() << "Need open file first!";
        return -1;
    }

    return d->readFunc(data, maxSize);
}

QByteArray DFile::read(qint64 maxSize)
{
    if (!d->readQFunc)
        return QByteArray();

    if (!d->isOpen) {
        qWarning() << "Need open file first!";
        return QByteArray();
    }

    return d->readQFunc(maxSize);
}

QByteArray DFile::readAll()
{
    if (!d->readAllFunc)
        return QByteArray();

    if (!d->isOpen) {
        qWarning() << "Need open file first!";
        return QByteArray();
    }

    return d->readAllFunc();
}

DFM_VIRTUAL qint64 DFile::write(const char *data, qint64 len)
{
    if (!d->writeFunc)
        return -1;

    if (!d->isOpen) {
        qWarning() << "Need open file first!";
        return -1;
    }

    return d->writeFunc(data, len);
}

qint64 DFile::write(const char *data)
{
    if (!d->writeAllFunc)
        return -1;

    if (!d->isOpen) {
        qWarning() << "Need open file first!";
        return -1;
    }

    return d->writeAllFunc(data);
}

qint64 DFile::write(const QByteArray &byteArray)
{
    if (!d->writeQFunc)
        return -1;

    if (!d->isOpen) {
        qWarning() << "Need open file first!";
        return -1;
    }

    return d->writeQFunc(byteArray);
}

DFM_VIRTUAL bool DFile::seek(qint64 pos, DFMSeekType type)
{
    if (!d->seekFunc)
        return -1;

    if (!d->isOpen) {
        qWarning() << "Need open file first!";
        return -1;
    }

    return d->seekFunc(pos, type);
}

DFM_VIRTUAL qint64 DFile::pos()
{
    if (!d->posFunc)
        return -1;

    if (!d->isOpen) {
        qWarning() << "Need open file first!";
        return -1;
    }

    return d->posFunc();
}

bool DFile::flush()
{
    if (!d->flushFunc)
        return -1;

    if (!d->isOpen) {
        qWarning() << "Need open file first!";
        return -1;
    }

    return d->flushFunc();
}

qint64 DFile::size()
{
    if (!d->sizeFunc)
        return -1;

    if (!d->isOpen) {
        qWarning() << "Need open file first!";
        return -1;
    }

    return d->sizeFunc();
}

bool DFile::exists()
{
    if (!d->existsFunc)
        return -1;

    if (!d->isOpen) {
        qWarning() << "Need open file first!";
        return -1;
    }

    return d->existsFunc();
}

void DFile::registerOpen(const OpenFunc &func)
{
    d->openFunc = func;
}

void DFile::registerClose(const CloseFunc &func)
{
    d->closeFunc = func;
}

void DFile::registerRead(const ReadFunc &func)
{
    d->readFunc = func;
}

void DFile::registerReadQ(const DFile::ReadQFunc &func)
{
    d->readQFunc = func;
}

void DFile::registerReadAll(const DFile::ReadAllFunc &func)
{
    d->readAllFunc = func;
}

void DFile::registerWrite(const WriteFunc &func)
{
    d->writeFunc = func;
}

void DFile::registerWriteAll(const DFile::WriteAllFunc &func)
{
    d->writeAllFunc = func;
}

void DFile::registerWriteQ(const DFile::WriteQFunc &func)
{
    d->writeQFunc = func;
}

void DFile::registerSeek(const SeekFunc &func)
{
    d->seekFunc = func;
}

void DFile::registerPos(const DFile::PosFunc &func)
{
    d->posFunc = func;
}

void DFile::registerFlush(const DFile::FlushFunc &func)
{
    d->flushFunc = func;
}

void DFile::registerSize(const DFile::SizeFunc &func)
{
    d->sizeFunc = func;
}

void DFile::registerExists(const DFile::ExistsFunc &func)
{
    d->existsFunc = func;
}

QUrl DFile::uri() const
{
    return d->uri;
}

DFMIOError DFile::lastError() const
{
    if (!d)
        return DFMIOError();

    return d->error;
}

bool DFile::copyFile(const QString &sourcePath, const QString &destPath, DFile::CopyFlag flag, void *cancellabel, std::function<void (int, int, void *)> progressCallback, void *progressCallbackData, void **error)
{
    GFile *fileSource = g_file_new_for_path(sourcePath.toLocal8Bit().data());
    GFile *fileDest = g_file_new_for_path(destPath.toLocal8Bit().data());

    GError *gerror = nullptr;

    bool success = g_file_copy(fileSource, fileDest, GFileCopyFlags(flag), nullptr, nullptr, nullptr, &gerror);

    g_object_unref(fileSource);
    g_object_unref(fileDest);

    if (gerror)
        g_error_free(gerror);

    return success;
}
