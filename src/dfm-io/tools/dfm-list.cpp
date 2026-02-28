// SPDX-FileCopyrightText: 2020 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <dfm-io/dfmio_global.h>
#include <dfm-io/denumerator.h>
#include <dfm-io/denumeratorfuture.h>
#include <dfm-io/dfileinfo.h>

#include <QDebug>
#include <QUrl>
#include <QCoreApplication>

USING_IO_NAMESPACE

void printFileInfo(const QSharedPointer<DFileInfo> &info)
{
    QString type;
    if (info->attribute(DFileInfo::AttributeID::kStandardIsDir).toBool())
        type = "DIR ";
    else if (info->attribute(DFileInfo::AttributeID::kStandardIsSymlink).toBool())
        type = "LINK";
    else
        type = "FILE";

    QString perms;
    perms += info->attribute(DFileInfo::AttributeID::kAccessCanRead).toBool() ? "r" : "-";
    perms += info->attribute(DFileInfo::AttributeID::kAccessCanWrite).toBool() ? "w" : "-";
    perms += info->attribute(DFileInfo::AttributeID::kAccessCanExecute).toBool() ? "x" : "-";

    qInfo() << QString("%1 %2 %3").arg(type, 4).arg(perms, 3)
            << info->attribute(DFileInfo::AttributeID::kStandardName).toString();
}

void testFilter(const QUrl &url, DEnumerator::DirFilters filter, const QString &desc)
{
    qInfo() << "\n=== Testing" << desc << "===";

    QSharedPointer<DEnumerator> enumerator(new DEnumerator(url, {}, filter, {}));
    if (!enumerator) {
        qWarning() << "Failed to create enumerator";
        return;
    }

    int count = 0;
    while (enumerator->hasNext()) {
        enumerator->next();
        auto info = enumerator->fileInfo();
        if (info) {
            printFileInfo(info);
            count++;
        }
    }
    qInfo() << "Total:" << count << "items\n";
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        qWarning() << "Usage:" << argv[0] << "<directory_path>";
        return 1;
    }

    QCoreApplication a(argc, argv);
    QUrl url = QUrl::fromLocalFile(QString::fromLocal8Bit(argv[1]));

    if (!url.isValid()) {
        qWarning() << "Invalid URL";
        return 1;
    }

    // 测试所有基本过滤器
    testFilter(url, DEnumerator::DirFilter::kNoFilter, "No Filter");
    testFilter(url, DEnumerator::DirFilter::kDirs, "Directories Only");
    testFilter(url, DEnumerator::DirFilter::kFiles, "Files Only");
    testFilter(url, DEnumerator::DirFilter::kAllDirs, "All Directories (including . and ..)");

    // 测试权限过滤器
    testFilter(url, DEnumerator::DirFilter::kAllEntries | DEnumerator::DirFilter::kReadable, "Readable");
    testFilter(url, DEnumerator::DirFilter::kAllEntries | DEnumerator::DirFilter::kWritable, "Writable");
    testFilter(url, DEnumerator::DirFilter::kAllEntries | DEnumerator::DirFilter::kExecutable, "Executable");

    // 测试特殊过滤器
    testFilter(url, DEnumerator::DirFilter::kAllEntries | DEnumerator::DirFilter::kNoSymLinks, "No Symlinks");

    // 测试组合过滤器
    testFilter(url,
               DEnumerator::DirFilter::kAllDirs | DEnumerator::DirFilter::kHidden,
               "All Dirs + Hidden");

    testFilter(url,
               DEnumerator::DirFilter::kFiles | DEnumerator::DirFilter::kReadable | DEnumerator::DirFilter::kWritable,
               "Files + Readable + Writable");

    testFilter(url,
               DEnumerator::DirFilter::kAllEntries | DEnumerator::DirFilter::kNoDotAndDotDot,
               "All Entries but no . and ..");

    testFilter(url,
               DEnumerator::DirFilter::kDirs | DEnumerator::DirFilter::kNoSymLinks,
               "Directories without symlinks");

    return 0;
}
