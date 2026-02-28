// SPDX-FileCopyrightText: 2024 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <dfm-burn/dpacketwritingcontroller.h>

#include "private/dpacketwritingcontroller_p.h"

extern "C" {
#include "3rdparty/udfclient/udf.h"
#include "3rdparty/udfclient/udf_bswap.h"
}

#include <QDebug>
#include <QFileInfo>

#include <unistd.h>
#include <sys/time.h>

extern "C" {
extern struct curdir
{
    char *name;
    struct udf_mountpoint *mountpoint; /* foreign */
    struct udf_node *udf_node; /* foreign */
    struct hash_entry *udf_mountpoint_entry; /* `current' mountpoint entry */
} curdir;

extern void udfclient_pwd(int args);
extern void udfclient_lcd(int args, char *arg1);
extern void udfclient_cd(int args, char *arg1);
extern char *udfclient_realpath(char *cur_path, char *relpath, char **leaf);
extern int udfclient_lookup_pathname(struct udf_node *cur_node, struct udf_node **res_node, char *restpath_given);
extern int udfclient_getattr(struct udf_node *udf_node, struct stat *stat);
extern int udfclient_readdir(struct udf_node *udf_node, struct uio *result_uio, int *eof_res);
extern int udfclient_put_subtree(struct udf_node *parent_node, char *srcprefix, char *srcname, char *dstprefix, char *dstname, uint64_t *totalsize);
extern int udfclient_rm_subtree(struct udf_node *parent_node, struct udf_node *dir_node, char *name, char *full_parent_name);
}

#define LS_SUBTREE_DIR_BUFFER_SIZE (16 * 1024)
DFM_BURN_BEGIN_NS

DPacketWritingControllerPrivate::DPacketWritingControllerPrivate()
{
    char pwd[1024];
    ::getcwd(pwd, 1024);

    oldLocalWoringPath = QString::fromLocal8Bit(pwd);
}

bool DPacketWritingControllerPrivate::initCurrentDir()
{
    bzero(&curdir, sizeof(struct curdir));
    curdir.mountpoint = NULL;
    curdir.name = strdup("/");

    // disc cd
    const QString &rootPath { makeDiscRootPath() };
    if (rootPath.isEmpty()) {
        qWarning() << "Make dsic root path failed!";
        return false;
    }
    cd(rootPath);

    // local cd
    lcd(localWorkingPath);
    return true;
}

void DPacketWritingControllerPrivate::lcd(const QString &path)
{
    QFileInfo info { path };
    if (info.exists() && info.isDir())
        udfclient_lcd(1, path.toLocal8Bit().data());
}

void DPacketWritingControllerPrivate::cd(const QString &path)
{
    udfclient_cd(1, path.toLocal8Bit().data());
}

QString DPacketWritingControllerPrivate::makeDiscRootPath()
{
    struct udf_node *udf_node;
    uint8_t *buffer;
    struct uio dir_uio;
    struct iovec dir_uiovec;
    struct dirent *dirent;
    struct stat stat;
    int eof;
    char *node_name, *leaf_name;
    int error;
    QString path;

    node_name = udfclient_realpath(curdir.name, "", &leaf_name);

    error = udfclient_lookup_pathname(NULL, &udf_node, node_name);
    if (error) {
        fprintf(stderr, "%s\n", strerror(error));
        free(node_name);
        return path;
    }

    error = udfclient_getattr(udf_node, &stat);
    if (stat.st_mode & S_IFDIR) {
        /* start at the start of the directory */
        dir_uio.uio_offset = 0;
        dir_uio.uio_iov = &dir_uiovec;
        dir_uio.uio_iovcnt = 1;
        buffer = (uint8_t *)calloc(1, LS_SUBTREE_DIR_BUFFER_SIZE);
        if (!buffer)
            return path;

        do {
            dir_uiovec.iov_base = buffer;
            dir_uiovec.iov_len = LS_SUBTREE_DIR_BUFFER_SIZE;
            dir_uio.uio_resid = LS_SUBTREE_DIR_BUFFER_SIZE;
            dir_uio.uio_rw = UIO_WRITE;

            error = udfclient_readdir(udf_node, &dir_uio, &eof);
            if (error) {
                fprintf(stderr, "error during readdir: %s\n", strerror(error));
                break;
            }
            dirent = (struct dirent *)(buffer);
            path = QString::fromLocal8Bit(dirent->d_name);
        } while (0);
        free(buffer);
    }
    free(node_name);

    return path;
}

int64_t DPacketWritingControllerPrivate::getmtime()
{
    struct timeval tp;

    gettimeofday(&tp, NULL);
    return 1000000 * tp.tv_sec + tp.tv_usec;
}

DPacketWritingController::DPacketWritingController(const QString &dev, const QString &workingPath, QObject *parent)
    : QObject(parent), dptr(new DPacketWritingControllerPrivate)
{
    dptr->deviceName = dev;
    dptr->localWorkingPath = workingPath;
}

DPacketWritingController::~DPacketWritingController()
{
    close();
}

bool DPacketWritingController::open()
{
    Q_ASSERT(!dptr->deviceName.isEmpty());
    qInfo() << "Opening device:" << dptr->deviceName;

    QFileInfo local { dptr->localWorkingPath };
    if (!local.exists() || !local.isDir()) {
        dptr->errorMsg = QString("Invalid local working directory: %1").arg(dptr->localWorkingPath);
        return false;
    }

    udf_init();
    struct udf_discinfo *disc;
    int mnt_flags { UDF_MNT_RDONLY };
    mnt_flags &= ~UDF_MNT_RDONLY;
    mnt_flags |= UDF_MNT_FORCE;
    int error = udf_mount_disc(dptr->deviceName.toLocal8Bit().data(),
                               NULL, 0, mnt_flags, &disc);
    if (error) {
        fprintf(stderr, "Can't open my device; bailing out : %s\n", strerror(error));
        dptr->errorMsg = QString::fromLocal8Bit(strerror(error));
        return false;
    }

    if (!dptr->initCurrentDir()) {
        dptr->errorMsg = "Init dir faield";
        return false;
    }

    dptr->deviceOpended = true;
    return true;
}

void DPacketWritingController::close()
{
    if (!isOpen())
        return;

    dptr->deviceOpended = false;
    struct udf_discinfo *disc, *next_disc;
    qInfo() << "Closing discs";

    disc = SLIST_FIRST(&udf_discs_list);
    while (disc) {
        next_disc = SLIST_NEXT(disc, next_disc);
        udf_dismount_disc(disc);
        disc = next_disc;
    }

    free(udf_bufcache);
    udf_bufcache = NULL;
    qInfo() << "Restore local working path:" << dptr->oldLocalWoringPath;
    dptr->lcd(dptr->oldLocalWoringPath);
    udfclient_pwd(0);
}

bool DPacketWritingController::put(const QString &fileName)
{
    Q_ASSERT(!fileName.isNull());

    struct udf_node *curdir_node;
    uint64_t start, now, totalsize, avg_speed;
    int error { 0 };

    error = udfclient_lookup_pathname(NULL, &curdir_node, curdir.name);
    if (error) {
        dptr->errorMsg = "Current directory not found";
        return false;
    }

    qDebug() << "Attempting to copy: " << fileName;
    /* writeout file/dir tree and measure the time/speed */
    totalsize = 0;
    start = dptr->getmtime();
    error = udfclient_put_subtree(curdir_node, ".", fileName.toLocal8Bit().data(), ".",
                                  fileName.toLocal8Bit().data(), &totalsize);
    if (error) {
        dptr->errorMsg = QString::fromLocal8Bit(strerror(error));
        return false;
    }
    now = dptr->getmtime();
    if (now - start > 0) {
        avg_speed = (1000000 * totalsize) / (now - start);
        qDebug() << "A total of" << (uint32_t)(totalsize / 1024)
                 << "kb transfered at an overal average of" << (uint32_t)(avg_speed / 1024) << "kb/sec";
    } else {
        qDebug() << "Transfered" << (uint32_t)(totalsize / 1024) << "kb";
    }
    return true;
}

/*!
 * \brief just rename now
 */
bool DPacketWritingController::mv(const QString &srcName, const QString &destName)
{
    struct udf_node *rename_me, *present, *old_parent, *new_parent;
    char *rename_from_name, *rename_to_name, *old_parent_name, *new_parent_name;
    int error;

    char *from = strdup(srcName.toLocal8Bit().data());
    char *from_bak = from;

    /* `from' gets substituted by its leaf name */
    rename_from_name = udfclient_realpath(curdir.name, from, &from);
    error = udfclient_lookup_pathname(NULL, &rename_me, rename_from_name);
    if (error || !rename_me) {
        dptr->errorMsg = "Can't find file/dir to be renamed";
        free(rename_from_name);
        free(from_bak);
        return false;
    }

    old_parent_name = udfclient_realpath(rename_from_name, "..", NULL);
    error = udfclient_lookup_pathname(NULL, &old_parent, old_parent_name);
    if (error || !old_parent) {
        dptr->errorMsg = "Can't determine rootdir of renamed file?";
        free(rename_from_name);
        free(old_parent_name);
        free(from_bak);
        return false;
    }

    char *to = strdup(destName.toLocal8Bit().data());
    char *to_bak = to;
    /* `to' gets substituted by its leaf name */
    rename_to_name = udfclient_realpath(curdir.name, to, &to);
    udfclient_lookup_pathname(NULL, &present, rename_to_name);
    new_parent_name = udfclient_realpath(rename_to_name, "..", NULL);
    error = udfclient_lookup_pathname(NULL, &new_parent, new_parent_name);
    if (error || !new_parent) {
        dptr->errorMsg = "Can't determine rootdir of destination";
        free(rename_from_name);
        free(rename_to_name);
        free(old_parent_name);
        free(new_parent_name);
        free(from_bak);
        free(to_bak);
        return false;
    }

    error = udf_rename(old_parent, rename_me, from, new_parent, present, to);
    if (error)
        dptr->errorMsg = QString("Can't move file or directory: %1").arg(strerror(error));

    free(rename_from_name);
    free(rename_to_name);
    free(old_parent_name);
    free(new_parent_name);
    free(from_bak);
    free(to_bak);

    return error ? false : true;
}

bool DPacketWritingController::rm(const QString &fileName)
{
    Q_ASSERT(!fileName.isNull());

    struct udf_node *remove_node, *parent_node;
    struct stat stat;
    char *target_name, *leaf_name, *full_parent_name;
    int error, len;

    leaf_name = strdup(fileName.toLocal8Bit().data());
    char *leaf_name_bak = leaf_name;
    target_name = udfclient_realpath(curdir.name, leaf_name, &leaf_name);
    error = udfclient_lookup_pathname(NULL, &remove_node, target_name);

    if (error || !remove_node) {
        printf("rm %s : %s\n", target_name, strerror(error));
        dptr->errorMsg = QString("rm %1 : %2").arg(target_name).arg(strerror(error));
        free(target_name);
        free(leaf_name_bak);
        return false;
    }

    full_parent_name = udfclient_realpath(target_name, "..", NULL);
    error = udfclient_lookup_pathname(NULL, &parent_node, full_parent_name);
    if (error || !parent_node) {
        dptr->errorMsg = QString("rm %1 : parent lookup failed : %2").arg(target_name).arg(strerror(error));
        free(target_name);
        free(full_parent_name);
        free(leaf_name_bak);
        return false;
    }

    error = udfclient_getattr(remove_node, &stat);
    if (!error) {
        if (stat.st_mode & S_IFDIR) {
            len = strlen(target_name);
            if (target_name[len - 1] == '/') target_name[len - 1] = '\0';
            error = udfclient_rm_subtree(parent_node, remove_node, leaf_name, target_name);
        } else {
            error = udf_remove_file(parent_node, remove_node, leaf_name);
            if (!error)
                qDebug() << "rm " << full_parent_name << "/" << leaf_name;
        }
    }
    if (error) {
        fprintf(stderr, "While removing file/dir : %s\n", strerror(error));
        dptr->errorMsg = QString("rm failed: %1").arg(strerror(error));
    }

    free(target_name);
    free(full_parent_name);
    free(leaf_name_bak);

    if (error)
        return false;

    return true;
}

QString DPacketWritingController::device() const
{
    return dptr->deviceName;
}

QString DPacketWritingController::localWorkingDirectory() const
{
    return dptr->localWorkingPath;
}

QString DPacketWritingController::lastError() const
{
    return dptr->errorMsg;
}

bool DPacketWritingController::isOpen() const
{
    return dptr->deviceOpended;
}

DFM_BURN_END_NS
