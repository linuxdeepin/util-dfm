// SPDX-FileCopyrightText: 2020 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TRASHHELPER_H
#define TRASHHELPER_H

#include <dfm-io/dfmio_global.h>

#include <QObject>
#include <QMap>

BEGIN_IO_NAMESPACE
class TrashHelper
{
public:
    struct DeleteTimeInfo
    {
        qint64 startTime { 0 };
        qint64 endTime { 0 };
    };

public:
    TrashHelper();
    ~TrashHelper();

    void setDeleteInfos(const QMap<QUrl, QSharedPointer<TrashHelper::DeleteTimeInfo>> &infos);
    bool getTrashUrls(QList<QUrl> *trashUrls, QString *errorMsg = nullptr);

private:
    QMap<QUrl, QSharedPointer<TrashHelper::DeleteTimeInfo>> deleteInfos;
};

END_IO_NAMESPACE
#endif   // TRASHHELPER_H
