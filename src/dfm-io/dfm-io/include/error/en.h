/*
 * Copyright (C) 2020 ~ 2021 Uniontech Software Technology Co., Ltd.
 *
 * Author:     zhangsheng<zhangsheng@uniontech.com>
 *
 * Maintainer: max-lv<lvwujun@uniontech.com>
 *             lanxuesong<lanxuesong@uniontech.com>
 *             xushitong<xushitong@uniontech.com>
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
#ifndef DFM_IO_ERROR_EN_H_
#define DFM_IO_ERROR_EN_H_

#include <QString>

enum DFMIOErrorCode {
    DFM_IO_ERROR_NONE = 0,   // No error.

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

    DFM_IO_ERROR_FAILED = 1000,   // Generic error condition for when an operation fails and no more specific DFMIOErrorEnum value is defined.
    DFM_IO_ERROR_OPEN_FAILED,   // File open failed
    DFM_IO_ERROR_OPEN_FLAG_ERROR,   // File open flag is error
    DFM_IO_ERROR_INFO_NO_ATTRIBUTE,   // File info has no attribute
};

inline const QString GetError_En(DFMIOErrorCode errorCode)
{
    switch (errorCode) {
    case DFM_IO_ERROR_NONE:
        return QString("No error");
    case DFM_IO_ERROR_NOT_FOUND:
        return QString("File not found");
    case DFM_IO_ERROR_EXISTS:
        return QString("File already exists");
    case DFM_IO_ERROR_IS_DIRECTORY:
        return QString("File is a directory");
    case DFM_IO_ERROR_NOT_DIRECTORY:
        return QString("File is not a directory");
    case DFM_IO_ERROR_NOT_EMPTY:
        return QString("File is a directory that isn't empty");
    case DFM_IO_ERROR_NOT_REGULAR_FILE:
        return QString("File is not a regular file");
    case DFM_IO_ERROR_NOT_SYMBOLIC_LINK:
        return QString("File is not a symbolic link");
    case DFM_IO_ERROR_NOT_MOUNTABLE_FILE:
        return QString("File cannot be mounted");
    case DFM_IO_ERROR_FILENAME_TOO_LONG:
        return QString("Filename is too many characters");
    case DFM_IO_ERROR_INVALID_FILENAME:
        return QString("Filename is invalid or contains invalid characters");
    case DFM_IO_ERROR_TOO_MANY_LINKS:
        return QString("File contains too many symbolic links");
    case DFM_IO_ERROR_NO_SPACE:
        return QString("No space left on drive");
    case DFM_IO_ERROR_INVALID_ARGUMENT:
        return QString("Invalid argument");
    case DFM_IO_ERROR_PERMISSION_DENIED:
        return QString("Permission denied");
    case DFM_IO_ERROR_NOT_SUPPORTED:
        return QString("Operation (or one of its parameters) not supported");
    case DFM_IO_ERROR_NOT_MOUNTED:
        return QString("File isn't mounted");
    case DFM_IO_ERROR_ALREADY_MOUNTED:
        return QString("File is already mounted");
    case DFM_IO_ERROR_CLOSED:
        return QString("File was closed");
    case DFM_IO_ERROR_CANCELLED:
        return QString("Operation was cancelled");
    case DFM_IO_ERROR_PENDING:
        return QString("Operations are still pending");
    case DFM_IO_ERROR_READ_ONLY:
        return QString("File is read only");
    case DFM_IO_ERROR_CANT_CREATE_BACKUP:
        return QString("Backup couldn't be created");
    case DFM_IO_ERROR_WRONG_ETAG:
        return QString("File's Entity Tag was incorrect");
    case DFM_IO_ERROR_TIMED_OUT:
        return QString("Operation timed out");
    case DFM_IO_ERROR_WOULD_RECURSE:
        return QString("Operation would be recursive");
    case DFM_IO_ERROR_BUSY:
        return QString("File is busy");
    case DFM_IO_ERROR_WOULD_BLOCK:
        return QString("Operation would block");
    case DFM_IO_ERROR_HOST_NOT_FOUND:
        return QString("Host couldn't be found (remote operations)");
    case DFM_IO_ERROR_WOULD_MERGE:
        return QString("Operation would merge files");
    case DFM_IO_ERROR_FAILED_HANDLED:
        return QString("Operation failed and a helper program has already interacted with the user. Do not display any error dialog");
    case DFM_IO_ERROR_TOO_MANY_OPEN_FILES:
        return QString("The current process has too many files open and can't open any more. Duplicate descriptors do count toward this limit");
    case DFM_IO_ERROR_NOT_INITIALIZED:
        return QString("The object has not been initialized");
    case DFM_IO_ERROR_ADDRESS_IN_USE:
        return QString("The requested address is already in use");
    case DFM_IO_ERROR_PARTIAL_INPUT:
        return QString("Need more input to finish operation");
    case DFM_IO_ERROR_INVALID_DATA:
        return QString("The input data was invalid");
    case DFM_IO_ERROR_DBUS_ERROR:
        return QString("A remote object generated an error(dbus)");
    case DFM_IO_ERROR_HOST_UNREACHABLE:
        return QString("Host unreachable");
    case DFM_IO_ERROR_NETWORK_UNREACHABLE:
        return QString("Network unreachable");
    case DFM_IO_ERROR_CONNECTION_REFUSED:
        return QString("Connection refused");
    case DFM_IO_ERROR_PROXY_FAILED:
        return QString("Connection to proxy server failed");
    case DFM_IO_ERROR_PROXY_AUTH_FAILED:
        return QString("Proxy authentication failed");
    case DFM_IO_ERROR_PROXY_NEED_AUTH:
        return QString("Proxy server needs authentication");
    case DFM_IO_ERROR_PROXY_NOT_ALLOWED:
        return QString("Proxy connection is not allowed by ruleset");
    case DFM_IO_ERROR_BROKEN_PIPE:
        return QString("Broken pipe");
    case DFM_IO_ERROR_CONNECTION_CLOSED:
        return QString("Connection closed by peer");
    case DFM_IO_ERROR_NOT_CONNECTED:
        return QString("Transport endpoint is not connected");
    case DFM_IO_ERROR_MESSAGE_TOO_LARGE:
        return QString("Message too large");
    case DFM_IO_ERROR_FAILED:
        return QString("Generic error condition for when an operation fails and no more specific DFMIOErrorEnum value is defined");
    case DFM_IO_ERROR_OPEN_FAILED:
        return QString("File open failed");
    case DFM_IO_ERROR_OPEN_FLAG_ERROR:
        return QString("File open flag error");
    case DFM_IO_ERROR_INFO_NO_ATTRIBUTE:
        return QString("File info has no attribute");
    }

    return QString("Unknown error");
}

#endif   // DFM_IO_ERROR_EN_H_
