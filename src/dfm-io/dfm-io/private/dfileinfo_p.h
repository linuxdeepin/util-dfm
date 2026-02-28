// SPDX-FileCopyrightText: 2020 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DFILEINFO_P_H
#define DFILEINFO_P_H

#include "utils/dmediainfo.h"

#include <dfm-io/dfileinfo.h>
#include <dfm-io/dfmio_global.h>

#include <QUrl>
#include <QVariant>
#include <QSharedData>
#include <QPointer>

#include <gio/gio.h>

#include <unordered_map>
#include <string>

BEGIN_IO_NAMESPACE

class DFileInfoPrivate : public QObject, public QSharedData
{
public:
    typedef struct
    {
        DFileInfo::InitQuerierAsyncCallback callback;
        gpointer userData;
        QPointer<DFileInfoPrivate> me;
    } QueryInfoAsyncOp;
    typedef struct
    {
        QPointer<DFileInfoPrivate> me;
        DFileFuture *future = nullptr;
    } QueryInfoAsyncOp2;

    explicit DFileInfoPrivate(DFileInfo *qq);
    DFileInfoPrivate(const DFileInfoPrivate &other);
    DFileInfoPrivate &operator=(const DFileInfoPrivate &other);
    virtual ~DFileInfoPrivate() override;
    void initNormal();

    void attributeExtend(DFileInfo::MediaType type, QList<DFileInfo::AttributeExtendID> ids, DFileInfo::AttributeExtendFuncCallback callback = nullptr);
    [[nodiscard]] DFileFuture *attributeExtend(DFileInfo::MediaType type, QList<DFileInfo::AttributeExtendID> ids, int ioPriority, QObject *parent = nullptr);
    bool cancelAttributeExtend();
    bool cancelAttributes();
    void attributeExtendCallback();

    void setErrorFromGError(GError *gerror);
    bool queryInfoSync();
    void queryInfoAsync(int ioPriority = 0, DFileInfo::InitQuerierAsyncCallback func = nullptr, void *userData = nullptr);
    QVariant attributesBySelf(DFileInfo::AttributeID id);
    QVariant attributesFromUrl(DFileInfo::AttributeID id);
    void checkAndResetCancel();

    [[nodiscard]] DFileFuture *initQuerierAsync(int ioPriority, QObject *parent = nullptr) const;
    [[nodiscard]] QFuture<void> refreshAsync();

    void cacheAttributes();
    DFile::Permissions permissions() const;
    bool exists() const;

    static void queryInfoAsyncCallback(GObject *sourceObject, GAsyncResult *res, gpointer userData);
    static void queryInfoAsyncCallback2(GObject *sourceObject, GAsyncResult *res, gpointer userData);
    static void freeQueryInfoAsyncOp(QueryInfoAsyncOp *op);
    static void freeQueryInfoAsyncOp2(QueryInfoAsyncOp2 *op);

public:
    DFileInfo *q { nullptr };

    QUrl uri = QUrl();
    char *attributes { nullptr };
    DFileInfo::FileQueryInfoFlags flag = DFileInfo::FileQueryInfoFlags::kTypeNone;

    QSharedPointer<DFMIO::DMediaInfo> mediaInfo { nullptr };
    QList<DFileInfo::AttributeExtendID> extendIDs;
    QPointer<DFileFuture> future;  // 使用 QPointer 自动追踪对象生命周期，防止访问已删除的对象
    DFileInfo::MediaType mediaType = DFileInfo::MediaType::kGeneral;
    DFileInfo::AttributeExtendFuncCallback attributeExtendFuncCallback { nullptr };

    QList<DFileInfo::AttributeID> attributesRealizationSelf;
    QList<DFileInfo::AttributeID> attributesNoBlockIO;
    GFile *gfile { nullptr };
    GFileInfo *gfileinfo { nullptr };
    std::atomic_bool initFinished { false };
    std::atomic_bool infoReseted { false };
    std::atomic_bool isQuquerying { false };
    GCancellable *gcancellable { nullptr };

    QFuture<void> futureRefresh;
    std::atomic_bool stoped { false };
    std::atomic_bool fileExists { false };
    QMap<DFileInfo::AttributeID, QVariant> caches;
    std::atomic_bool cacheing { false };
    std::atomic_bool refreshing { false };
    QMutex mutex;

    DFMIOError error;
};

END_IO_NAMESPACE

#endif   // DFILEINFO_P_H
