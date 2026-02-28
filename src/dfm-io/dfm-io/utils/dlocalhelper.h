// SPDX-FileCopyrightText: 2020 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DGIOHELPER_H
#define DGIOHELPER_H

#include <dfm-io/dfmio_global.h>
#include <dfm-io/dfileinfo.h>
#include <dfm-io/denumerator.h>

#include <gio/gio.h>

#include <QSharedPointer>

#include <fts.h>

BEGIN_IO_NAMESPACE

template<class C, typename Ret, typename... Ts>
std::function<Ret(Ts...)> bind_field(C *c, Ret (C::*m)(Ts...))
{
    return [=](auto &&...args) { return (c->*m)(std::forward<decltype(args)>(args)...); };
}

template<class C, typename Ret, typename... Ts>
std::function<Ret(Ts...)> bind_field(const C *c, Ret (C::*m)(Ts...) const)
{
    return [=](auto &&...args) { return (c->*m)(std::forward<decltype(args)>(args)...); };
}

template<typename Ret, typename... Ts>
std::function<Ret(Ts...)> bind_field(Ret (*m)(Ts...))
{
    return [=](auto &&...args) { return (*m)(std::forward<decltype(args)>(args)...); };
}

class DLocalHelper
{
public:
    using AttributeInfoMap = std::unordered_map<DFileInfo::AttributeID, std::tuple<std::string, QVariant>>;

    static AttributeInfoMap &attributeInfoMapFunc();
    static QSharedPointer<DFileInfo> createFileInfoByUri(const QUrl &uri, const char *attributes = "*",
                                                         const DFMIO::DFileInfo::FileQueryInfoFlags flag = DFMIO::DFileInfo::FileQueryInfoFlags::kTypeNone);
    static QSharedPointer<DFileInfo> createFileInfoByUri(const QUrl &uri, GFileInfo *gfileInfo, const char *attributes = "*",
                                                         const DFMIO::DFileInfo::FileQueryInfoFlags flag = DFMIO::DFileInfo::FileQueryInfoFlags::kTypeNone);

    static QVariant attributeFromGFileInfo(GFileInfo *gfileinfo, DFileInfo::AttributeID id, DFMIOErrorCode &errorcode);
    static QVariant customAttributeFromPath(const QString &path, DFileInfo::AttributeID id);
    static QVariant customAttributeFromPathAndInfo(const QString &path, GFileInfo *fileInfo, DFileInfo::AttributeID id);
    static bool setAttributeByGFile(GFile *gfile, DFileInfo::AttributeID id, const QVariant &value, GError **error);
    static bool setAttributeByGFileInfo(GFileInfo *gfileinfo, DFileInfo::AttributeID id, const QVariant &value);
    static std::string attributeStringById(DFileInfo::AttributeID id);
    static QSet<QString> hideListFromUrl(const QUrl &url);
    static bool fileIsHidden(const DFileInfo *dfileinfo, const QSet<QString> &hideList, const bool needRead = true);

    // tools
    static bool checkGFileType(GFile *file, GFileType type);
    static bool isNumOrChar(const QChar ch);
    static bool isNumber(const QChar ch);
    static bool isSymbol(const QChar ch);
    static bool isFullWidthChar(const QChar ch, QChar &normalized);
    static bool compareByStringEx(const QString &str1, const QString &str2);
    static QString numberStr(const QString &str, int pos);
    static bool compareByString(const QString &str1, const QString &str2);
    static int compareByName(const FTSENT **left, const FTSENT **right);
    static int compareBySize(const FTSENT **left, const FTSENT **right);
    static int compareByLastModifed(const FTSENT **left, const FTSENT **right);
    static int compareByLastRead(const FTSENT **left, const FTSENT **right);
    static QSharedPointer<DEnumerator::SortFileInfo> createSortFileInfo(const FTSENT *ent,
                                                                        const QSet<QString> hidList);
    static GFile* createGFile(const QUrl &uri);
private:
    static QVariant getGFileInfoIcon(GFileInfo *gfileinfo, const char *key, DFMIOErrorCode &errorcode);
    static QVariant getGFileInfoString(GFileInfo *gfileinfo, const char *key, DFMIOErrorCode &errorcode);
    static QVariant getGFileInfoByteString(GFileInfo *gfileinfo, const char *key, DFMIOErrorCode &errorcode);
    static QVariant getGFileInfoBool(GFileInfo *gfileinfo, const char *key, DFMIOErrorCode &errorcode);
    static QVariant getGFileInfoUint32(GFileInfo *gfileinfo, const char *key, DFMIOErrorCode &errorcode);
    static QVariant getGFileInfoInt32(GFileInfo *gfileinfo, const char *key, DFMIOErrorCode &errorcode);
    static QVariant getGFileInfoUint64(GFileInfo *gfileinfo, const char *key, DFMIOErrorCode &errorcode);
    static QVariant getGFileInfoInt64(GFileInfo *gfileinfo, const char *key, DFMIOErrorCode &errorcode);
    static bool setGFileInfoString(GFile *gfile, const char *key, const QVariant &value, GError **gerror);
    static bool setGFileInfoByteString(GFile *gfile, const char *key, const QVariant &value, GError **gerror);
    static bool setGFileInfoBool(GFile *gfile, const char *key, const QVariant &value, GError **gerror);
    static bool setGFileInfoUint32(GFile *gfile, const char *key, const QVariant &value, GError **gerror);
    static bool setGFileInfoInt32(GFile *gfile, const char *key, const QVariant &value, GError **gerror);
    static bool setGFileInfoUint64(GFile *gfile, const char *key, const QVariant &value, GError **gerror);
    static bool setGFileInfoInt64(GFile *gfile, const char *key, const QVariant &value, GError **gerror);
    static QString symlinkTarget(const QUrl &url);
    static QString resolveSymlink(const QUrl &url);
    static qint64 fileSizeByEnt(const FTSENT **ent);
};

END_IO_NAMESPACE

#endif   // DGIOHELPER_H
