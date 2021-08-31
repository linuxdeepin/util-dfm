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

#include <QDebug>

USING_IO_NAMESPACE


DFilePrivate::DFilePrivate(DFile *q)
    : q_ptr(q)
{

}

DFilePrivate::~DFilePrivate()
{

}

DFile::DFile(const QUrl &uri)
    : d_ptr(new DFilePrivate(this))
{
    Q_D(DFile);
    d->uri = uri;
}

DFile::~DFile()
{
    close();
}

DFM_VIRTUAL bool DFile::open(DFile::OpenFlag mode)
{
    Q_D(DFile);

    if (!d->openFunc)
        return false;

    d->isOpen = d->openFunc(mode);

    return d->isOpen;
}

DFM_VIRTUAL bool DFile::close()
{
    Q_D(DFile);

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
    Q_D(DFile);

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
    Q_D(DFile);

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
    Q_D(DFile);

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
    Q_D(DFile);

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
    Q_D(DFile);

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
    Q_D(DFile);

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
    Q_D(DFile);

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
    Q_D(DFile);

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
    Q_D(DFile);

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
    Q_D(DFile);

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
    Q_D(DFile);

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
    Q_D(DFile);
    d->openFunc = func;
}

void DFile::registerClose(const CloseFunc &func)
{
    Q_D(DFile);
    d->closeFunc = func;
}

void DFile::registerRead(const ReadFunc &func)
{
    Q_D(DFile);
    d->readFunc = func;
}

void DFile::registerReadQ(const DFile::ReadQFunc &func)
{
    Q_D(DFile);
    d->readQFunc = func;
}

void DFile::registerReadAll(const DFile::ReadAllFunc &func)
{
    Q_D(DFile);
    d->readAllFunc = func;
}

void DFile::registerWrite(const WriteFunc &func)
{
    Q_D(DFile);
    d->writeFunc = func;
}

void DFile::registerWriteAll(const DFile::WriteAllFunc &func)
{
    Q_D(DFile);
    d->writeAllFunc = func;
}

void DFile::registerWriteQ(const DFile::WriteQFunc &func)
{
    Q_D(DFile);
    d->writeQFunc = func;
}

void DFile::registerSeek(const SeekFunc &func)
{
    Q_D(DFile);
    d->seekFunc = func;
}

void DFile::registerPos(const DFile::PosFunc &func)
{
    Q_D(DFile);
    d->posFunc = func;
}

void DFile::registerFlush(const DFile::FlushFunc &func)
{
    Q_D(DFile);
    d->flushFunc = func;
}

void DFile::registerSize(const DFile::SizeFunc &func)
{
    Q_D(DFile);
    d->sizeFunc = func;
}

void DFile::registerExists(const DFile::ExistsFunc &func)
{
    Q_D(DFile);
    d->existsFunc = func;
}

QUrl DFile::uri() const
{
    Q_D(const DFile);
    return d->uri;
}

DFMIOError DFile::lastError() const
{
    Q_D(const DFile);

    if (!d)
        return DFMIOError();

    return d->error;
}
