// SPDX-FileCopyrightText: 2020 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DFM_IO_ERROR_EN_H_
#define DFM_IO_ERROR_EN_H_

#include <QString>
#include <QObject>

enum DFMIOErrorCode {
    DFM_IO_ERROR_NONE = -1,   // No error.
    DFM_IO_ERROR_FAILED,   // Generic error condition for when an operation fails and no more specific DFMIOErrorEnum value is defined.
    DFM_IO_ERROR_NOT_FOUND,   // File not found.
    DFM_IO_ERROR_EXISTS,   // File already exists.
    DFM_IO_ERROR_IS_DIRECTORY,   // File is a directory.
    DFM_IO_ERROR_NOT_DIRECTORY,   // File is not a directory.
    DFM_IO_ERROR_NOT_EMPTY,   // File is a directory that isn't empty.
    DFM_IO_ERROR_NOT_REGULAR_FILE,   // File is not a regular file.
    DFM_IO_ERROR_NOT_SYMBOLIC_LINK,   // File is not a symbolic link.
    DFM_IO_ERROR_NOT_MOUNTABLE_FILE,   // File cannot be mounted.
    DFM_IO_ERROR_FILENAME_TOO_LONG,   // Filename is too many characters.
    DFM_IO_ERROR_INVALID_FILENAME,   // Filename is invalid or contains invalid characters.
    DFM_IO_ERROR_TOO_MANY_LINKS,   // File contains too many symbolic links.
    DFM_IO_ERROR_NO_SPACE,   // No space left on drive.
    DFM_IO_ERROR_INVALID_ARGUMENT,   // Invalid argument.
    DFM_IO_ERROR_PERMISSION_DENIED,   // Permission denied.
    DFM_IO_ERROR_NOT_SUPPORTED,   // Operation (or one of its parameters) not supported.
    DFM_IO_ERROR_NOT_MOUNTED,   // File isn't mounted.
    DFM_IO_ERROR_ALREADY_MOUNTED,   // File is already mounted.
    DFM_IO_ERROR_CLOSED,   // File was closed.
    DFM_IO_ERROR_CANCELLED,   // Operation was cancelled.
    DFM_IO_ERROR_PENDING,   // Operations are still pending.
    DFM_IO_ERROR_READ_ONLY,   // File is read only.
    DFM_IO_ERROR_CANT_CREATE_BACKUP,   // Backup couldn't be created.
    DFM_IO_ERROR_WRONG_ETAG,   // File's Entity Tag was incorrect.
    DFM_IO_ERROR_TIMED_OUT,   // Operation timed out.
    DFM_IO_ERROR_WOULD_RECURSE,   // Operation would be recursive.
    DFM_IO_ERROR_BUSY,   // File is busy.
    DFM_IO_ERROR_WOULD_BLOCK,   // Operation would block.
    DFM_IO_ERROR_HOST_NOT_FOUND,   // Host couldn't be found (remote operations).
    DFM_IO_ERROR_WOULD_MERGE,   // Operation would merge files.
    DFM_IO_ERROR_FAILED_HANDLED,   // Operation failed and a helper program has already interacted with the user.
    // Do not display any error dialog.
    DFM_IO_ERROR_TOO_MANY_OPEN_FILES,   // The current process has too many files open and can't open any more.
    // Duplicate descriptors do count toward this limit.
    DFM_IO_ERROR_NOT_INITIALIZED,   // The object has not been initialized.
    DFM_IO_ERROR_ADDRESS_IN_USE,   // The requested address is already in use.
    DFM_IO_ERROR_PARTIAL_INPUT,   // Need more input to finish operation.
    DFM_IO_ERROR_INVALID_DATA,   // The input data was invalid.
    DFM_IO_ERROR_DBUS_ERROR,   // A remote object generated an error(dbus).
    DFM_IO_ERROR_HOST_UNREACHABLE,   // Host unreachable.
    DFM_IO_ERROR_NETWORK_UNREACHABLE,   // Network unreachable.
    DFM_IO_ERROR_CONNECTION_REFUSED,   // Connection refused.
    DFM_IO_ERROR_PROXY_FAILED,   // Connection to proxy server failed.
    DFM_IO_ERROR_PROXY_AUTH_FAILED,   // Proxy authentication failed.
    DFM_IO_ERROR_PROXY_NEED_AUTH,   // Proxy server needs authentication.
    DFM_IO_ERROR_PROXY_NOT_ALLOWED,   // Proxy connection is not allowed by ruleset.
    DFM_IO_ERROR_BROKEN_PIPE,   // Broken pipe.
    DFM_IO_ERROR_CONNECTION_CLOSED,   // Connection closed by peer.
    DFM_IO_ERROR_NOT_CONNECTED,   // Transport endpoint is not connected.
    DFM_IO_ERROR_MESSAGE_TOO_LARGE,   // Message too large.

    DFM_IO_ERROR_USER_FAILED = 1000,   // Generic error condition for when an operation fails and no more specific DFMIOErrorEnum value is defined.
    DFM_IO_ERROR_OPEN_FAILED,   // File open failed
    DFM_IO_ERROR_OPEN_FLAG_ERROR,   // File open flag is error
    DFM_IO_ERROR_INFO_NO_ATTRIBUTE,   // File info has no attribute
    DFM_IO_ERROR_FTS_OPEN,   // open file by fts failed
    DFM_IO_ERROR_HOST_IS_DOWN, // remote server maybe down
};

inline const QString GetError_En(DFMIOErrorCode errorCode)
{
    switch (errorCode) {
    case DFM_IO_ERROR_NONE:
        return QObject::tr("No error");
    case DFM_IO_ERROR_NOT_FOUND:
        return QObject::tr("File not found");
    case DFM_IO_ERROR_EXISTS:
        return QObject::tr("File already exists");
    case DFM_IO_ERROR_IS_DIRECTORY:
        return QObject::tr("File is a directory");
    case DFM_IO_ERROR_NOT_DIRECTORY:
        return QObject::tr("File is not a directory");
    case DFM_IO_ERROR_NOT_EMPTY:
        return QObject::tr("File is a directory that isn't empty");
    case DFM_IO_ERROR_NOT_REGULAR_FILE:
        return QObject::tr("File is not a regular file");
    case DFM_IO_ERROR_NOT_SYMBOLIC_LINK:
        return QObject::tr("File is not a symbolic link");
    case DFM_IO_ERROR_NOT_MOUNTABLE_FILE:
        return QObject::tr("File cannot be mounted");
    case DFM_IO_ERROR_FILENAME_TOO_LONG:
        return QObject::tr("Filename has too many characters");
    case DFM_IO_ERROR_INVALID_FILENAME:
        return QObject::tr("Filename is invalid or contains invalid characters");
    case DFM_IO_ERROR_TOO_MANY_LINKS:
        return QObject::tr("File contains too many symbolic links");
    case DFM_IO_ERROR_NO_SPACE:
        return QObject::tr("No space left on drive");
    case DFM_IO_ERROR_INVALID_ARGUMENT:
        return QObject::tr("Invalid argument");
    case DFM_IO_ERROR_PERMISSION_DENIED:
        return QObject::tr("Permission denied");
    case DFM_IO_ERROR_NOT_SUPPORTED:
        return QObject::tr("Operation (or one of its parameters) not supported");
    case DFM_IO_ERROR_NOT_MOUNTED:
        return QObject::tr("File isn't mounted");
    case DFM_IO_ERROR_ALREADY_MOUNTED:
        return QObject::tr("File is already mounted");
    case DFM_IO_ERROR_CLOSED:
        return QObject::tr("File was closed");
    case DFM_IO_ERROR_CANCELLED:
        return QObject::tr("Operation was cancelled");
    case DFM_IO_ERROR_PENDING:
        return QObject::tr("Operations are still pending");
    case DFM_IO_ERROR_READ_ONLY:
        return QObject::tr("File is read-only");
    case DFM_IO_ERROR_CANT_CREATE_BACKUP:
        return QObject::tr("Backup couldn't be created");
    case DFM_IO_ERROR_WRONG_ETAG:
        return QObject::tr("File's Entity Tag was incorrect");
    case DFM_IO_ERROR_TIMED_OUT:
        return QObject::tr("Operation timed out");
    case DFM_IO_ERROR_WOULD_RECURSE:
        return QObject::tr("Operation would be recursive");
    case DFM_IO_ERROR_BUSY:
        return QObject::tr("File is busy");
    case DFM_IO_ERROR_WOULD_BLOCK:
        return QObject::tr("Operation would block");
    case DFM_IO_ERROR_HOST_NOT_FOUND:
        return QObject::tr("Host couldn't be found (remote operations)");
    case DFM_IO_ERROR_WOULD_MERGE:
        return QObject::tr("Operation would merge files");
    case DFM_IO_ERROR_FAILED_HANDLED:
        return QObject::tr("Operation failed and a helper program has already interacted with the user. Do not display any error dialog");
    case DFM_IO_ERROR_TOO_MANY_OPEN_FILES:
        return QObject::tr("The current process has too many files open and can't open any more. Duplicate descriptors do count toward this limit");
    case DFM_IO_ERROR_NOT_INITIALIZED:
        return QObject::tr("The object has not been initialized");
    case DFM_IO_ERROR_ADDRESS_IN_USE:
        return QObject::tr("The requested address is already in use");
    case DFM_IO_ERROR_PARTIAL_INPUT:
        return QObject::tr("Need more input to finish operation");
    case DFM_IO_ERROR_INVALID_DATA:
        return QObject::tr("The input data was invalid");
    case DFM_IO_ERROR_DBUS_ERROR:
        return QObject::tr("A remote object generated an error(dbus)");
    case DFM_IO_ERROR_HOST_UNREACHABLE:
        return QObject::tr("Host unreachable");
    case DFM_IO_ERROR_NETWORK_UNREACHABLE:
        return QObject::tr("Network unreachable");
    case DFM_IO_ERROR_CONNECTION_REFUSED:
        return QObject::tr("Connection refused");
    case DFM_IO_ERROR_PROXY_FAILED:
        return QObject::tr("Connection to proxy server failed");
    case DFM_IO_ERROR_PROXY_AUTH_FAILED:
        return QObject::tr("Proxy authentication failed");
    case DFM_IO_ERROR_PROXY_NEED_AUTH:
        return QObject::tr("Proxy server needs authentication");
    case DFM_IO_ERROR_PROXY_NOT_ALLOWED:
        return QObject::tr("Proxy connection is not allowed by ruleset");
    case DFM_IO_ERROR_BROKEN_PIPE:
        return QObject::tr("Broken pipe");
    case DFM_IO_ERROR_CONNECTION_CLOSED:
        return QObject::tr("Connection closed by peer");
    case DFM_IO_ERROR_NOT_CONNECTED:
        return QObject::tr("Transport endpoint is not connected");
    case DFM_IO_ERROR_MESSAGE_TOO_LARGE:
        return QObject::tr("Message too large");
    case DFM_IO_ERROR_FAILED:
        return QObject::tr("Generic error condition for when an operation fails and no more specific DFMIOErrorEnum value is defined");
    case DFM_IO_ERROR_OPEN_FAILED:
        return QObject::tr("Failed to open the file");
    case DFM_IO_ERROR_OPEN_FLAG_ERROR:
        return QObject::tr("File open flag error");
    case DFM_IO_ERROR_INFO_NO_ATTRIBUTE:
        return QObject::tr("File info has no attribute");
    case DFM_IO_ERROR_FTS_OPEN:
        return QObject::tr("open file by fts failed");
    case DFM_IO_ERROR_USER_FAILED:
        return QString();
    case DFM_IO_ERROR_HOST_IS_DOWN:
        return QObject::tr("Host is down");
    }

    return QString("Unknown error");
}

#endif   // DFM_IO_ERROR_EN_H_
