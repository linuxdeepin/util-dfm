// SPDX-FileCopyrightText: 2020 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DMOUNTUTILS_H
#define DMOUNTUTILS_H

#include <dfm-mount/base/dmount_global.h>

#include <QVariant>
#include <QCoreApplication>
#include <QThread>

typedef struct _GVariant GVariant;
typedef struct _GError GError;

DFM_MOUNT_BEGIN_NS

#define warningIfNotInMain()                                                                                               \
    {                                                                                                                      \
        if (qApp->thread() != QThread::currentThread())                                                                    \
            qWarning() << "<" << __PRETTY_FUNCTION__ << ">\n"                                                              \
                       << "\t:( this function DOES NOT promise thread safe! please use it CAUTION or use *Async instead."; \
    }

class Utils
{
public:
    static QVariant castFromGVariant(GVariant *val);

    static GVariant *castFromQVariant(const QVariant &val);

    static GVariant *castFromQVariantMap(const QVariantMap &val);

    static GVariant *castFromQStringList(const QStringList &val);

    static GVariant *castFromList(const QList<QVariant> &val);

    static QString getNameByProperty(Property type);

    static Property getPropertyByName(const QString &name, const QString &iface);

    static PartitionType getPartitionTypeByGuid(const QString &guid);

    static DeviceError castFromGError(const GError *const err);

    static DeviceError castFromJobOperation(const QString &op);

    static QString errorMessage(DeviceError err);

    static QString errorMessage(MonitorError err);

    static OperationErrorInfo genOperateErrorInfo(DeviceError err, const QString &errMsg = "");

    /*!
     * \brief gcharToQString
     * \param tmp: target char *, which will be free in function, use it caution
     * \return
     */
    static QString gcharToQString(char *tmp);

    /*!
     * \brief gcharvToQStringList
     * \param tmp: target char *, which will be free in function, use it caution
     * \return
     */
    static QStringList gcharvToQStringList(char **tmp);
    static QVariant gvariantToQVariant(GVariant *value);

    template<typename FromClass, typename ToClass>
    static inline ToClass *castClassFromTo(FromClass *p)
    {
        auto pPointer = dynamic_cast<ToClass *>(p);
        return pPointer;
    }

    static QString currentUser();
};

DFM_MOUNT_END_NS
#endif
