// SPDX-FileCopyrightText: 2020 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "local/dlocalhelper.h"
#include "core/dfileinfo.h"
#include "local/dlocalfileinfo.h"

#include "gio/gfileinfo.h"

#include <QDebug>
#include <QCollator>

#include <sys/stat.h>
#include <sys/types.h>

USING_IO_NAMESPACE

namespace LocalFunc {

static bool isFile(GFileInfo *fileInfo)
{
    if (!fileInfo)
        return false;
    return g_file_info_get_file_type(fileInfo) == G_FILE_TYPE_REGULAR;
}

static bool isDir(GFileInfo *fileInfo)
{
    if (!fileInfo)
        return false;
    return g_file_info_get_file_type(fileInfo) == G_FILE_TYPE_DIRECTORY;
}

static bool isRoot(const QString &path)
{
    return path == "/";
}

static QString fileName(GFileInfo *fileInfo)
{
    if (!fileInfo)
        return QString();

    const char *name = g_file_info_get_name(fileInfo);
    return QString::fromLocal8Bit(name);
}

static QString baseName(GFileInfo *fileInfo)
{
    const QString &fullName = fileName(fileInfo);

    if (isDir(fileInfo))
        return fullName;

    int pos2 = fullName.indexOf(".");
    if (pos2 == -1)
        return fullName;
    else
        return fullName.left(pos2);
}

static QString completeBaseName(GFileInfo *fileInfo)
{
    const QString &fullName = fileName(fileInfo);

    if (isDir(fileInfo))
        return fullName;

    int pos2 = fullName.lastIndexOf(".");
    if (pos2 == -1)
        return fullName;
    else
        return fullName.left(pos2);
}

static QString suffix(GFileInfo *fileInfo)
{
    // path
    if (isDir(fileInfo))
        return "";

    const QString &fullName = fileName(fileInfo);

    int pos2 = fullName.lastIndexOf(".");
    if (pos2 == -1)
        return "";
    else
        return fullName.mid(pos2 + 1);
}

static QString completeSuffix(GFileInfo *fileInfo)
{
    if (isDir(fileInfo))
        return "";

    const QString &fullName = fileName(fileInfo);

    int pos2 = fullName.indexOf(".");
    if (pos2 == -1)
        return "";
    else
        return fullName.mid(pos2 + 1);
}

static QString filePath(const QString &path)
{
    g_autoptr(GFile) file = g_file_new_for_path(path.toStdString().c_str());

    g_autofree gchar *gpath = g_file_get_path(file);   // no blocking I/O
    if (gpath != nullptr)
        return QString::fromLocal8Bit(gpath);

    return "";
}

static QString parentPath(const QString &path)
{
    g_autoptr(GFile) file = g_file_new_for_path(path.toStdString().c_str());
    g_autoptr(GFile) fileParent = g_file_get_parent(file);   // no blocking I/O

    g_autofree gchar *gpath = g_file_get_path(fileParent);   // no blocking I/O
    if (gpath != nullptr)
        return QString::fromLocal8Bit(gpath);
    return "";
}
}   // LocalFunc

QSharedPointer<DFileInfo> DLocalHelper::createFileInfoByUri(const QUrl &uri, const char *attributes /*= "*"*/,
                                                            const DFMIO::DFileInfo::FileQueryInfoFlags flag /*= DFMIO::DFileInfo::FileQueryInfoFlags::TypeNone*/)
{
    return QSharedPointer<DFileInfo>(new DLocalFileInfo(uri, attributes, flag));
}

QSharedPointer<DFileInfo> DLocalHelper::createFileInfoByUri(const QUrl &uri, GFileInfo *gfileInfo, const char *attributes, const DFileInfo::FileQueryInfoFlags flag)
{
    return QSharedPointer<DFileInfo>(new DLocalFileInfo(uri, gfileInfo, attributes, flag));
}

QVariant DLocalHelper::attributeFromGFileInfo(GFileInfo *gfileinfo, DFileInfo::AttributeID id, DFMIOErrorCode &errorcode)
{
    if (!gfileinfo)
        return QVariant();

    // check custom attribute
    if (id > DFileInfo::AttributeID::kCustomStart) {
        return QVariant();
    }

    // check has attribute
    const std::string &key = DLocalHelper::attributeStringById(id);
    bool hasAttr = g_file_info_has_attribute(gfileinfo, key.c_str());
    if (!hasAttr) {
        // if has not attribute, return QVariant(), and caller use default value
        errorcode = DFM_IO_ERROR_INFO_NO_ATTRIBUTE;
        return QVariant();
    }

    switch (id) {
    // uint32_t
    case DFileInfo::AttributeID::kStandardType:
    case DFileInfo::AttributeID::kMountableUnixDevice:
    case DFileInfo::AttributeID::kMountableStartStopType:
    case DFileInfo::AttributeID::kTimeModifiedUsec:
    case DFileInfo::AttributeID::kTimeAccessUsec:
    case DFileInfo::AttributeID::kTimeChangedUsec:
    case DFileInfo::AttributeID::kUnixDevice:
    case DFileInfo::AttributeID::kUnixMode:
    case DFileInfo::AttributeID::kUnixNlink:
    case DFileInfo::AttributeID::kUnixUID:
    case DFileInfo::AttributeID::kUnixGID:
    case DFileInfo::AttributeID::kUnixRdev:
    case DFileInfo::AttributeID::kUnixBlockSize:
    case DFileInfo::AttributeID::kFileSystemUsePreview:
    case DFileInfo::AttributeID::kTrashItemCount: {
        uint32_t ret = g_file_info_get_attribute_uint32(gfileinfo, key.c_str());
        return QVariant(ret);
    }
    // int32_t
    case DFileInfo::AttributeID::kStandardSortOrder: {
        int32_t ret = g_file_info_get_attribute_int32(gfileinfo, key.c_str());
        return QVariant(ret);
    }
    // uint64_t
    case DFileInfo::AttributeID::kStandardSize:
    case DFileInfo::AttributeID::kStandardAllocatedSize:
    case DFileInfo::AttributeID::kTimeModified:
    case DFileInfo::AttributeID::kTimeAccess:
    case DFileInfo::AttributeID::kTimeChanged:
    case DFileInfo::AttributeID::kUnixInode:
    case DFileInfo::AttributeID::kUnixBlocks:
    case DFileInfo::AttributeID::kFileSystemSize:
    case DFileInfo::AttributeID::kFileSystemFree:
    case DFileInfo::AttributeID::kFileSystemUsed: {
        uint64_t ret = g_file_info_get_attribute_uint64(gfileinfo, key.c_str());
        return qulonglong(ret);
    }
    case DFileInfo::AttributeID::kRecentModified: {
        int64_t ret = g_file_info_get_attribute_int64(gfileinfo, key.c_str());
        return qlonglong(ret);
    }
    // bool
    case DFileInfo::AttributeID::kStandardIsHidden:
    case DFileInfo::AttributeID::kStandardIsBackup:
    case DFileInfo::AttributeID::kStandardIsSymlink:
    case DFileInfo::AttributeID::kStandardIsVirtual:
    case DFileInfo::AttributeID::kStandardIsVolatile:
    case DFileInfo::AttributeID::kAccessCanRead:
    case DFileInfo::AttributeID::kAccessCanWrite:
    case DFileInfo::AttributeID::kAccessCanExecute:
    case DFileInfo::AttributeID::kAccessCanDelete:
    case DFileInfo::AttributeID::kAccessCanTrash:
    case DFileInfo::AttributeID::kAccessCanRename:
    case DFileInfo::AttributeID::kMountableCanMount:
    case DFileInfo::AttributeID::kMountableCanUnmount:
    case DFileInfo::AttributeID::kMountableCanEject:
    case DFileInfo::AttributeID::kMountableCanPoll:
    case DFileInfo::AttributeID::kMountableIsMediaCheckAutomatic:
    case DFileInfo::AttributeID::kMountableCanStart:
    case DFileInfo::AttributeID::kMountableCanStartDegraded:
    case DFileInfo::AttributeID::kMountableCanStop:
    case DFileInfo::AttributeID::kUnixIsMountPoint:
    case DFileInfo::AttributeID::kDosIsArchive:
    case DFileInfo::AttributeID::kDosIsSystem:
    case DFileInfo::AttributeID::kFileSystemReadOnly:
    case DFileInfo::AttributeID::kFileSystemRemote: {
        bool ret = g_file_info_get_attribute_boolean(gfileinfo, key.c_str());
        return QVariant(ret);
    }
    // byte string
    case DFileInfo::AttributeID::kStandardName:
    case DFileInfo::AttributeID::kStandardSymlinkTarget:
    case DFileInfo::AttributeID::kTrashOrigPath:
    case DFileInfo::AttributeID::kThumbnailPath: {
        const char *ret = g_file_info_get_attribute_byte_string(gfileinfo, key.c_str());
        return QVariant(ret);
    }
    // string
    case DFileInfo::AttributeID::kStandardDisplayName:
    case DFileInfo::AttributeID::kStandardEditName:
    case DFileInfo::AttributeID::kStandardCopyName:
    case DFileInfo::AttributeID::kStandardContentType:
    case DFileInfo::AttributeID::kStandardFastContentType:
    case DFileInfo::AttributeID::kStandardTargetUri:
    case DFileInfo::AttributeID::kStandardDescription:
    case DFileInfo::AttributeID::kEtagValue:
    case DFileInfo::AttributeID::kIdFile:
    case DFileInfo::AttributeID::kIdFilesystem:
    case DFileInfo::AttributeID::kMountableUnixDeviceFile:
    case DFileInfo::AttributeID::kMountableHalUdi:
    case DFileInfo::AttributeID::kOwnerUser:
    case DFileInfo::AttributeID::kOwnerUserReal:
    case DFileInfo::AttributeID::kOwnerGroup:
    case DFileInfo::AttributeID::kFileSystemType:
    case DFileInfo::AttributeID::kGvfsBackend:
    case DFileInfo::AttributeID::kSelinuxContext:
    case DFileInfo::AttributeID::kTrashDeletionDate: {
        const char *ret = g_file_info_get_attribute_string(gfileinfo, key.c_str());
        return QVariant(ret);
    }
    // object
    case DFileInfo::AttributeID::kStandardIcon:
    case DFileInfo::AttributeID::kPreviewIcon:
    case DFileInfo::AttributeID::kStandardSymbolicIcon: {
        GObject *icon = g_file_info_get_attribute_object(gfileinfo, key.c_str());
        if (!icon)
            return QVariant();

        QList<QString> ret;
        auto names = g_themed_icon_get_names(G_THEMED_ICON(icon));
        for (int j = 0; names && names[j] != nullptr; ++j)
            ret.append(QString::fromLocal8Bit(names[j]));

        return QVariant(ret);
    }

    default:
        return QVariant();
    }
}

QVariant DLocalHelper::customAttributeFromPathAndInfo(const QString &path, GFileInfo *fileInfo, DFileInfo::AttributeID id)
{
    if (id < DFileInfo::AttributeID::kCustomStart)
        return QVariant();

    switch (id) {
    case DFileInfo::AttributeID::kStandardIsFile: {
        return LocalFunc::isFile(fileInfo);
    }
    case DFileInfo::AttributeID::kStandardIsDir: {
        return LocalFunc::isDir(fileInfo);
    }
    case DFileInfo::AttributeID::kStandardIsRoot: {
        return LocalFunc::isRoot(path);
    }
    case DFileInfo::AttributeID::kStandardSuffix: {
        return LocalFunc::suffix(fileInfo);
    }
    case DFileInfo::AttributeID::kStandardCompleteSuffix: {
        return LocalFunc::completeSuffix(fileInfo);
    }
    case DFileInfo::AttributeID::kStandardFilePath: {
        return LocalFunc::filePath(path);
    }
    case DFileInfo::AttributeID::kStandardParentPath: {
        return LocalFunc::parentPath(path);
    }
    case DFileInfo::AttributeID::kStandardBaseName: {
        return LocalFunc::baseName(fileInfo);
    }
    case DFileInfo::AttributeID::kStandardFileName: {
        return LocalFunc::fileName(fileInfo);
    }
    case DFileInfo::AttributeID::kStandardCompleteBaseName: {
        return LocalFunc::completeBaseName(fileInfo);
    }
    default:
        return QVariant();
    }
}

QVariant DLocalHelper::customAttributeFromPath(const QString &path, DFileInfo::AttributeID id)
{
    Q_UNUSED(path)
    Q_UNUSED(id)
    return QVariant();
}

bool DLocalHelper::setAttributeByGFile(GFile *gfile, DFileInfo::AttributeID id, const QVariant &value, GError **gerror)
{
    if (!gfile) {
        return false;
    }

    // check has attribute
    const std::string &key = DLocalHelper::attributeStringById(id);
    if (key.empty()) {
        return false;
    }

    switch (id) {
    // uint32_t
    case DFileInfo::AttributeID::kStandardType:
    case DFileInfo::AttributeID::kMountableUnixDevice:
    case DFileInfo::AttributeID::kMountableStartStopType:
    case DFileInfo::AttributeID::kTimeModifiedUsec:
    case DFileInfo::AttributeID::kTimeAccessUsec:
    case DFileInfo::AttributeID::kTimeChangedUsec:
    case DFileInfo::AttributeID::kTimeCreatedUsec:
    case DFileInfo::AttributeID::kUnixDevice:
    case DFileInfo::AttributeID::kUnixMode:
    case DFileInfo::AttributeID::kUnixNlink:
    case DFileInfo::AttributeID::kUnixUID:
    case DFileInfo::AttributeID::kUnixGID:
    case DFileInfo::AttributeID::kUnixRdev:
    case DFileInfo::AttributeID::kUnixBlockSize:
    case DFileInfo::AttributeID::kFileSystemUsePreview:
    case DFileInfo::AttributeID::kTrashItemCount: {
        g_file_set_attribute_uint32(gfile, key.c_str(), value.toUInt(), G_FILE_QUERY_INFO_NONE, nullptr, gerror);
        if (gerror) {
            g_autofree gchar *url = g_file_get_uri(gfile);
            //qWarning() << "file set attribute failed, url: " << url << " msg: " << (*gerror)->message;

            return false;
        }
        return true;
    }
    // int32_t
    case DFileInfo::AttributeID::kStandardSortOrder: {
        g_file_set_attribute_int32(gfile, key.c_str(), value.toInt(), G_FILE_QUERY_INFO_NONE, nullptr, gerror);
        if (gerror) {
            g_autofree gchar *url = g_file_get_uri(gfile);
            //qWarning() << "file set attribute failed, url: " << url << " msg: " << (*gerror)->message;
            return false;
        }
        return true;
    }
    // uint64_t
    case DFileInfo::AttributeID::kStandardSize:
    case DFileInfo::AttributeID::kStandardAllocatedSize:
    case DFileInfo::AttributeID::kTimeModified:
    case DFileInfo::AttributeID::kTimeAccess:
    case DFileInfo::AttributeID::kTimeChanged:
    case DFileInfo::AttributeID::kTimeCreated:
    case DFileInfo::AttributeID::kUnixInode:
    case DFileInfo::AttributeID::kUnixBlocks:
    case DFileInfo::AttributeID::kFileSystemSize:
    case DFileInfo::AttributeID::kFileSystemFree:
    case DFileInfo::AttributeID::kFileSystemUsed: {
        bool succ = g_file_set_attribute_uint64(gfile, key.c_str(), value.toULongLong(), G_FILE_QUERY_INFO_NONE, nullptr, gerror);
        return succ;
    }
    case DFileInfo::AttributeID::kRecentModified: {
        bool succ = g_file_set_attribute_int64(gfile, key.c_str(), value.toLongLong(), G_FILE_QUERY_INFO_NONE, nullptr, gerror);
        return succ;
    }
    // bool
    case DFileInfo::AttributeID::kStandardIsHidden:
    case DFileInfo::AttributeID::kStandardIsBackup:
    case DFileInfo::AttributeID::kStandardIsSymlink:
    case DFileInfo::AttributeID::kStandardIsVirtual:
    case DFileInfo::AttributeID::kStandardIsVolatile:
    case DFileInfo::AttributeID::kAccessCanRead:
    case DFileInfo::AttributeID::kAccessCanWrite:
    case DFileInfo::AttributeID::kAccessCanExecute:
    case DFileInfo::AttributeID::kAccessCanDelete:
    case DFileInfo::AttributeID::kAccessCanTrash:
    case DFileInfo::AttributeID::kAccessCanRename:
    case DFileInfo::AttributeID::kMountableCanMount:
    case DFileInfo::AttributeID::kMountableCanUnmount:
    case DFileInfo::AttributeID::kMountableCanEject:
    case DFileInfo::AttributeID::kMountableCanPoll:
    case DFileInfo::AttributeID::kMountableIsMediaCheckAutomatic:
    case DFileInfo::AttributeID::kMountableCanStart:
    case DFileInfo::AttributeID::kMountableCanStartDegraded:
    case DFileInfo::AttributeID::kMountableCanStop:
    case DFileInfo::AttributeID::kUnixIsMountPoint:
    case DFileInfo::AttributeID::kDosIsArchive:
    case DFileInfo::AttributeID::kDosIsSystem:
    case DFileInfo::AttributeID::kFileSystemReadOnly:
    case DFileInfo::AttributeID::kFileSystemRemote: {
        gboolean b = value.toBool();
        gpointer gpValue = &b;
        g_file_set_attribute(gfile, key.c_str(), G_FILE_ATTRIBUTE_TYPE_BOOLEAN, gpValue, G_FILE_QUERY_INFO_NONE, nullptr, gerror);
        if (gerror) {
            g_autofree gchar *url = g_file_get_uri(gfile);
            //qWarning() << "file set attribute failed, url: " << url << " msg: " << (*gerror)->message;
            return false;
        }
        return true;
    }
    // byte string
    case DFileInfo::AttributeID::kStandardName:
    case DFileInfo::AttributeID::kStandardSymlinkTarget:
    case DFileInfo::AttributeID::kThumbnailPath: {
        g_file_set_attribute_byte_string(gfile, key.c_str(), value.toString().toLocal8Bit().data(), G_FILE_QUERY_INFO_NONE, nullptr, gerror);
        if (gerror) {
            g_autofree gchar *url = g_file_get_uri(gfile);
            //qWarning() << "file set attribute failed, url: " << url << " msg: " << (*gerror)->message;
            return false;
        }
        return true;
    }
    // string
    case DFileInfo::AttributeID::kStandardDisplayName:
    case DFileInfo::AttributeID::kStandardEditName:
    case DFileInfo::AttributeID::kStandardCopyName:
    case DFileInfo::AttributeID::kStandardContentType:
    case DFileInfo::AttributeID::kStandardFastContentType:
    case DFileInfo::AttributeID::kStandardTargetUri:
    case DFileInfo::AttributeID::kStandardDescription:
    case DFileInfo::AttributeID::kEtagValue:
    case DFileInfo::AttributeID::kIdFile:
    case DFileInfo::AttributeID::kIdFilesystem:
    case DFileInfo::AttributeID::kMountableUnixDeviceFile:
    case DFileInfo::AttributeID::kMountableHalUdi:
    case DFileInfo::AttributeID::kOwnerUser:
    case DFileInfo::AttributeID::kOwnerUserReal:
    case DFileInfo::AttributeID::kOwnerGroup:
    case DFileInfo::AttributeID::kFileSystemType:
    case DFileInfo::AttributeID::kGvfsBackend:
    case DFileInfo::AttributeID::kSelinuxContext:
    case DFileInfo::AttributeID::kTrashDeletionDate:
    case DFileInfo::AttributeID::kTrashOrigPath: {
        g_file_set_attribute_string(gfile, key.c_str(), value.toString().toLocal8Bit().data(), G_FILE_QUERY_INFO_NONE, nullptr, gerror);
        if (gerror) {
            g_autofree gchar *url = g_file_get_uri(gfile);
            //qWarning() << "file set attribute failed, url: " << url << " msg: " << (*gerror)->message;
            return false;
        }
        return true;
    }
    // object
    case DFileInfo::AttributeID::kStandardIcon:
    case DFileInfo::AttributeID::kStandardSymbolicIcon:
    case DFileInfo::AttributeID::kPreviewIcon: {
        //g_file_info_set_attribute_object(gfileinfo, key, value.object());
        // TODO(lanxs)
        return true;
    }

    default:
        return true;
    }
}

bool DLocalHelper::setAttributeByGFileInfo(GFileInfo *gfileinfo, DFileInfo::AttributeID id, const QVariant &value)
{
    // discard
    Q_UNUSED(gfileinfo)
    Q_UNUSED(id)
    Q_UNUSED(value)
    return false;
}

std::string DLocalHelper::attributeStringById(DFileInfo::AttributeID id)
{
    if (DFileInfo::attributeInfoMap.count(id) > 0) {
        const std::string &value = std::get<0>(DFileInfo::attributeInfoMap.at(id));
        return value;
    }
    return "";
}

QSet<QString> DLocalHelper::hideListFromUrl(const QUrl &url)
{
    g_autofree char *contents = nullptr;
    g_autoptr(GError) error = nullptr;
    gsize len = 0;
    g_autoptr(GFile) hiddenFile = g_file_new_for_uri(url.toString().toLocal8Bit().data());

    const bool succ = g_file_load_contents(hiddenFile, nullptr, &contents, &len, nullptr, &error);
    if (succ) {
        if (contents && len > 0) {
            QString dataStr(contents);
            return QSet<QString>::fromList(dataStr.split('\n', QString::SkipEmptyParts));
        }
    }
    return {};
}

bool DLocalHelper::fileIsHidden(const QSharedPointer<DFileInfo> &dfileinfo, const QSet<QString> &hideList, const bool needRead)
{
    if (!dfileinfo)
        return false;

    const QString &fileName = dfileinfo->attribute(DFileInfo::AttributeID::kStandardName, nullptr).toString();
    if (fileName.startsWith(".")) {
        return true;
    } else {
        if (hideList.isEmpty() && needRead) {
            const QString &hiddenPath = dfileinfo->attribute(DFileInfo::AttributeID::kStandardParentPath, nullptr).toString() + "/.hidden";
            const QSet<QString> &hideList = DLocalHelper::hideListFromUrl(QUrl::fromLocalFile(hiddenPath));

            if (hideList.contains(fileName))
                return true;
        } else {
            return hideList.contains(fileName);
        }
    }

    return false;
}

bool DLocalHelper::checkGFileType(GFile *file, GFileType type)
{
    if (!file)
        return false;

    g_autoptr(GFileInfo) gfileinfo = g_file_query_info(file, G_FILE_ATTRIBUTE_STANDARD_TYPE, G_FILE_QUERY_INFO_NONE, nullptr, nullptr);

    if (!gfileinfo)
        return false;

    return g_file_info_get_file_type(gfileinfo) == type;
}
//fix 多线程排序时，该处的全局变量在compareByString函数中可能导致软件崩溃
//QCollator sortCollator;
class DCollator : public QCollator
{
public:
    DCollator() : QCollator()
    {
        setNumericMode(true);
        setCaseSensitivity(Qt::CaseInsensitive);
    }
};

bool DLocalHelper::isNumOrChar(const QChar ch)
{
    return (ch >= 48 && ch <= 57) || (ch >= 65 && ch <= 90) || (ch >= 97 && ch <= 122);
}

bool DLocalHelper::isNumber(const QChar ch)
{
    return (ch >= 48 && ch <= 57);
}

bool DLocalHelper::isSymbol(const QChar ch)
{
    return ch.script() != QChar::Script_Han && !isNumOrChar(ch);
}

QString DLocalHelper::numberStr(const QString &str, int pos)
{
    QString tmp;
    auto total = str.length();

    while (pos > 0 && isNumber(str.at(pos))) {
        pos--;
    }

    if (pos > 0)
        pos++;

    while (pos < total && isNumber(str.at(pos))) {
        tmp += str.at(pos);
        pos++;
    }

    return tmp;
}

// The first is smaller than the second and returns true
bool DLocalHelper::compareByStringEx(const QString &str1, const QString &str2) {
    thread_local static DCollator sortCollator;
    QString suf1 = str1.right(str1.length() - str1.lastIndexOf(".") - 1);
    QString suf2 = str2.right(str2.length() - str2.lastIndexOf(".") - 1);
    QString name1 = str1.left(str1.lastIndexOf("."));
    QString name2 = str2.left(str2.lastIndexOf("."));
    int length1 = name1.length();
    int length2 = name2.length();
    auto total = length1 > length2 ? length2 : length1;

    bool preIsNum = false;
    bool isSybol1 = false, isSybol2 = false, isHanzi1 = false,
            isHanzi2 = false, isNumb1 = false, isNumb2 = false;

    for (int i = 0; i < total; ++i) {
        // 判断相等和大小写相等，跳过
        if (str1.at(i) == str2.at(i) || str1.at(i).toLower() == str2.at(i).toLower()) {
            preIsNum = isNumber(str1.at(i));
            continue;
        }
        // 判断特殊字符就排到最后
        isSybol1 = isSymbol(str1.at(i));
        isSybol2 = isSymbol(str2.at(i));
        if (isSybol1 ^ isSybol2)
            return !isSybol1;

        if (isSybol1)
            return str1.at(i) < str2.at(i);

        // 判断汉字
        isHanzi1 = str1.at(i).script() == QChar::Script_Han;
        isHanzi2 = str2.at(i).script() == QChar::Script_Han;
        if (isHanzi2 ^ isHanzi1)
            return !isHanzi1;

        if (isHanzi1)
            return  sortCollator.compare(str1.at(i), str2.at(i)) < 0;

        // 判断数字或者字符
        isNumb1 = isNumber(str1.at(i));
        isNumb2 = isNumber(str2.at(i));
        if (!isNumb1 && !isNumb2) {
            return str1.at(i).toLower() < str2.at(i).toLower();
        } else if(preIsNum || (isNumb1 && isNumb2)) {
            // 取后面几位的数字作比较后面的数字,先比较位数
            // 位数大的大
            auto str1n = numberStr(str1, i).toUInt();
            auto str2n = numberStr(str2, i).toUInt();
            if (str1n == str2n)
                return str1.at(i) < str2.at(i);
            return str1n < str2n;
        }

        return isNumb1;
    }

    if (length1 == length2) {
        if (suf1.isEmpty() ^ suf2.isEmpty())
            return suf1.isEmpty();

        if (suf2.startsWith(suf1) ^ suf1.startsWith(suf2))
            return suf2.startsWith(suf1);

        return suf1 < suf2;
    }

    return length1 < length2;
}

bool DLocalHelper::compareByString(const QString &str1, const QString &str2)
{
    // 处理文件名称为  新建文件a 新建文件夹 排序错误的问题
    //  按名称排序
    //  1、按名称排序规则为：数字→字母→汉字→其它；
    //  2、其中数字由小到大排列，字母由a～z、 A~Z排列（例如：a A b B），汉字:按拼音首字母由a～z排列；
    //  3、如果首字母相同看第二位字母，以此类推；
    //  4、其他：特殊字符和乱码排在后面；
    return compareByStringEx(str1, str2);
}

int DLocalHelper::compareByName(const FTSENT **left, const FTSENT **right)
{
    QString str1 = QString((*left)->fts_name), str2 = QString((*right)->fts_name);
    auto tt = compareByString(str1, str2);
    return tt ? -1 : 1;
}

int DLocalHelper::compareBySize(const FTSENT **left, const FTSENT **right)
{
    if ((*left)->fts_statp->st_size == (*right)->fts_statp->st_size)
        return compareByName(left, right);
    return (*left)->fts_statp->st_size > (*right)->fts_statp->st_size;
}

int DLocalHelper::compareByLastModifed(const FTSENT **left, const FTSENT **right)
{
    if ((*left)->fts_statp->st_mtim.tv_sec == (*right)->fts_statp->st_mtim.tv_sec) {
        if ((*left)->fts_statp->st_mtim.tv_nsec > (*right)->fts_statp->st_mtim.tv_nsec)
            return compareByName(left, right);
        return (*left)->fts_statp->st_mtim.tv_nsec > (*right)->fts_statp->st_mtim.tv_nsec;
    }
    return (*left)->fts_statp->st_mtim.tv_sec > (*right)->fts_statp->st_mtim.tv_sec;
}

int DLocalHelper::compareByLastRead(const FTSENT **left, const FTSENT **right)
{
    if ((*left)->fts_statp->st_atim.tv_sec == (*right)->fts_statp->st_atim.tv_sec){
        if ((*left)->fts_statp->st_atim.tv_nsec > (*right)->fts_statp->st_atim.tv_nsec)
            return compareByName(left, right);
        return (*left)->fts_statp->st_atim.tv_nsec > (*right)->fts_statp->st_atim.tv_nsec;
    }
    return (*left)->fts_statp->st_atim.tv_sec > (*right)->fts_statp->st_atim.tv_sec;
}

QSharedPointer<DEnumerator::SortFileInfo> DLocalHelper::createSortFileInfo(const FTSENT *ent, const QSharedPointer<DFileInfo> &info,
                                                                           const QSet<QString> hidList)
{
    auto sortPointer = QSharedPointer<DEnumerator::SortFileInfo>(new DEnumerator::SortFileInfo);
    auto name = QString(ent->fts_name);
    auto path = QString(ent->fts_path);
    if (info) {
        sortPointer->isDir = info->attribute(DFileInfo::AttributeID::kStandardIsDir).toBool();
        sortPointer->isFile = !sortPointer->isDir;
        sortPointer->isSymLink = info->attribute(DFileInfo::AttributeID::kStandardIsSymlink).toBool();
        auto fileName = info->attribute(DFileInfo::AttributeID::kStandardFileName).toString();
        sortPointer->isHide = name.startsWith(".")
                ? true
                : hidList.contains(name);
        sortPointer->isReadable = info->attribute(DFileInfo::AttributeID::kAccessCanRead).toBool();
        sortPointer->isWriteable = info->attribute(DFileInfo::AttributeID::kAccessCanWrite).toBool();
        sortPointer->isExecutable = info->attribute(DFileInfo::AttributeID::kAccessCanExecute).toBool();
        sortPointer->url = QUrl::fromLocalFile(path);
        return sortPointer;
    }

    sortPointer->isDir = S_ISDIR(ent->fts_statp->st_mode);
    sortPointer->isFile = !sortPointer->isDir;
    sortPointer->isSymLink = S_ISLNK(ent->fts_statp->st_mode);
    sortPointer->isHide = name.startsWith(".") ? true : hidList.contains(name);
    sortPointer->isReadable = ent->fts_statp->st_mode & S_IREAD;
    sortPointer->isWriteable = ent->fts_statp->st_mode & S_IWRITE;
    sortPointer->isExecutable = ent->fts_statp->st_mode & S_IEXEC;
    sortPointer->url = QUrl::fromLocalFile(ent->fts_path);
    return sortPointer;
}
