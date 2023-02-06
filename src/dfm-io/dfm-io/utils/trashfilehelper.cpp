#include "trashfilehelper.h"

#include <QFile>
#include <QTextStream>

#include <glib/gstdio.h>

BEGIN_IO_NAMESPACE
TrashFileHelper::TrashFileHelper()
{

}

QString TrashFileHelper::getTrashFilename(const char *basename, int id)
{
    const char *dot;
    QString fileName;
    gchar *temp = nullptr;
    if (id == 1) {
        temp = g_strdup(basename);
        fileName = temp;
        g_free(temp);
        return fileName;
    }

    dot = strchr(basename, '.');
    if (dot) {
         temp = g_strdup_printf("%.*s.%d%s", static_cast<int>(dot - basename), basename, id, dot);
         fileName = temp;
         g_free(temp);
        return fileName;
    }else {
        temp = g_strdup_printf("%s.%d", basename, id);
        fileName = temp;
        g_free(temp);
        return fileName;
    }
}

QString TrashFileHelper::getParent(const QString &path, dev_t *parentDev)
{
    auto pathCopy = path.endsWith("/") ? path.left(path.length() - 1) : path;
    auto parentPath = pathCopy.left(pathCopy.lastIndexOf("/"));
    if (parentPath.isEmpty())
        return QString();
    auto devt = getFileDevType(parentPath);
    if (devt <= 0)
        return QString();
    if (parentDev)
        *parentDev = devt;

    return parentPath;
}

dev_t TrashFileHelper::getFileDevType(const QString &path)
{
    GStatBuf stat;
    if (g_lstat(path.toStdString().c_str(), &stat) != 0) {
        return 0;
    }
    return stat.st_dev;
}

QString TrashFileHelper::findMountpoint(const QString &path, const dev_t dev)
{
    auto dir = path;
    dev_t parentDev;
    QString homeDir(g_get_home_dir());
    if (dev == getFileDevType(homeDir)) {
        return homeDir;
    }
    while (dir != "/") {
        auto parent = getParent(dir, &parentDev);
        if (parent.isEmpty())
            return parent;
        if (parentDev != dev)
            return dir;
        dir = parent;
    }
    return dir;
}

QString TrashFileHelper::trashDir(const QString &path, QString *topDir)
{
    QString homeDir(g_get_home_dir());
    dev_t dirDev;

    auto parent = getParent(path, &dirDev);
    if (parent.isEmpty())
        return parent;

    auto dir = findMountpoint(parent, dirDev);

    if (dir.isEmpty())
        return dir;

    if (dir == homeDir) {
        gchar * trashDir = g_build_filename(g_get_user_data_dir(), "Trash", NULL);
        dir = trashDir;
        g_free(trashDir);
        return dir;
    }

    if (topDir)
        *topDir = dir;

    return dir + "/.Trash-" + QString::number(getuid());
}

QString TrashFileHelper::trashTargetPath(const QString &path, GFile *file)
{
    QString topDir;
    auto trashDirStr = trashDir(path, &topDir);
    if (trashDirStr.isEmpty())
        return trashDirStr;
    gchar *temp = g_file_get_basename(file);
    char *originalNameEscaped = g_uri_escape_string (path.toStdString().c_str(), "/", FALSE);
    QString baseName(temp);
    g_free(temp);
    QString originalName(originalNameEscaped);
    g_free(originalNameEscaped);
    QString buildBaseName = baseName;

    auto trashFileInfo = trashDirStr + "/info/" + baseName + ".trashinfo";
    auto tmpPath = path;
    auto targetBase = topDir.isEmpty() ? path : tmpPath.replace(topDir + "/", "");
    originalName = topDir.isEmpty() ? originalName : originalName.replace(topDir + "/", "");
    int i = 1;
    QStringList baseNameList;
    while (true) {
        QFile file(trashFileInfo);

        if (!file.open(QIODevice::ReadOnly))
            break;

        QByteArray content = file.readAll();

        file.close();

        QTextStream stream(&content, QIODevice::ReadOnly);

        auto path = stream.readLine();
        // 读取第二行
        if (!stream.atEnd())
            path = stream.readLine();

        if (path.startsWith("Path=") && path.replace("Path=", "") == originalName) {
            baseNameList.append(baseName);
        }
        ++i;
        baseName = getTrashFilename(buildBaseName.toStdString().c_str(), i);
        trashFileInfo = trashDirStr + "/info/" + baseName + ".trashinfo";
    }

    if (baseNameList.isEmpty())
        return QString();

    return trashDirStr + "/files/" + baseNameList.last();
}

END_IO_NAMESPACE
