/*
 * Copyright (C) 2020 ~ 2021 Uniontech Software Technology Co., Ltd.
 *
 * Author:     xushitong<xushitong@uniontech.com>
 *
 * Maintainer: max-lv<lvwujun@uniontech.com>
 *             lanxuesong<lanxuesong@uniontech.com>
 *             zhangsheng<zhangsheng@uniontech.com>
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
#ifndef DFMMOUNTUTILS_H
#define DFMMOUNTUTILS_H

#include "dfmmount_global.h"

#include <QVariant>
#include <QApplication>
#include <QThread>

typedef struct _GVariant GVariant;
DFM_MOUNT_BEGIN_NS

#define warningIfNotInMain() { \
    if (qApp->thread() != QThread::currentThread()) \
        qWarning() << "<" << __PRETTY_FUNCTION__ << ">\n"\
                   << "\t:( this function DOES NOT promise thread safe! please use it CAUTION or use *Async instead."; \
}

class Utils {
public:
static QVariant castFromGVariant(GVariant *val);

static GVariant *castFromQVariant(const QVariant &val);

static GVariant *castFromQVariantMap(const QVariantMap &val);

static GVariant *castFromQStringList(const QStringList &val);

static GVariant *castFromList(const QList<QVariant> &val);

static QString getNameByProperty(Property type);

static Property getPropertyByName(const QString &name);

template<typename FromClass, typename ToClass>
static inline ToClass *castClassFromTo(FromClass *p) {
    auto pPointer = dynamic_cast<ToClass *>(p);
//    if (!pPointer)
//        abort();
    return pPointer;
}
};

DFM_MOUNT_END_NS
#endif
