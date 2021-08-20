#ifndef DFM_IO_ERROR_ERROR_H_
#define DFM_IO_ERROR_ERROR_H_

#include "dfmio_global.h"

enum DFMIOErrorCode {
    DFM_IO_ERROR_NONE = 0,              // No error.

    DFM_IO_ERROR_NOT_FOUND,             // File not found.
    DFM_IO_ERROR_EXISTS,                // File already exists.
    DFM_IO_ERROR_IS_DIRECTORY,          // File is a directory.
    DFM_IO_ERROR_NOT_DIRECTORY,         // File is not a directory.
    DFM_IO_ERROR_NOT_EMPTY,             // File is a directory that isn't empty.
    DFM_IO_ERROR_NOT_REGULAR_FILE,      // File is not a regular file.
    DFM_IO_ERROR_NOT_SYMBOLIC_LINK,     // File is not a symbolic link.
    DFM_IO_ERROR_NOT_MOUNTABLE_FILE,    // File cannot be mounted.
    DFM_IO_ERROR_FILENAME_TOO_LONG,     // Filename is too many characters.
    DFM_IO_ERROR_INVALID_FILENAME,      // Filename is invalid or contains invalid characters.
    DFM_IO_ERROR_TOO_MANY_LINKS,        // File contains too many symbolic links.
    DFM_IO_ERROR_NO_SPACE,              // No space left on drive.
    DFM_IO_ERROR_INVALID_ARGUMENT,      // Invalid argument.
    DFM_IO_ERROR_PERMISSION_DENIED,     // Permission denied.
    DFM_IO_ERROR_NOT_SUPPORTED,         // Operation (or one of its parameters) not supported.
    DFM_IO_ERROR_NOT_MOUNTED,           // File isn't mounted.
    DFM_IO_ERROR_ALREADY_MOUNTED,       // File is already mounted.
    DFM_IO_ERROR_CLOSED,                // File was closed.
    DFM_IO_ERROR_CANCELLED,             // Operation was cancelled.
    DFM_IO_ERROR_PENDING,               // Operations are still pending.
    DFM_IO_ERROR_READ_ONLY,             // File is read only.
    DFM_IO_ERROR_CANT_CREATE_BACKUP,    // Backup couldn't be created.
    DFM_IO_ERROR_WRONG_ETAG,            // File's Entity Tag was incorrect.
    DFM_IO_ERROR_TIMED_OUT,             // Operation timed out.
    DFM_IO_ERROR_WOULD_RECURSE,         // Operation would be recursive.
    DFM_IO_ERROR_BUSY,                  // File is busy.
    DFM_IO_ERROR_WOULD_BLOCK,           // Operation would block.
    DFM_IO_ERROR_HOST_NOT_FOUND,        // Host couldn't be found (remote operations).
    DFM_IO_ERROR_WOULD_MERGE,           // Operation would merge files.
    DFM_IO_ERROR_FAILED_HANDLED,        // Operation failed and a helper program has already interacted with the user.
                                        // Do not display any error dialog.
    DFM_IO_ERROR_TOO_MANY_OPEN_FILES,   // The current process has too many files open and can't open any more.
                                        // Duplicate descriptors do count toward this limit.
    DFM_IO_ERROR_NOT_INITIALIZED,       // The object has not been initialized.
    DFM_IO_ERROR_ADDRESS_IN_USE,        // The requested address is already in use.
    DFM_IO_ERROR_PARTIAL_INPUT,         // Need more input to finish operation.
    DFM_IO_ERROR_INVALID_DATA,          // The input data was invalid.
    DFM_IO_ERROR_DBUS_ERROR,            // A remote object generated an error(dbus).
    DFM_IO_ERROR_HOST_UNREACHABLE,      // Host unreachable.
    DFM_IO_ERROR_NETWORK_UNREACHABLE,   // Network unreachable.
    DFM_IO_ERROR_CONNECTION_REFUSED,    // Connection refused.
    DFM_IO_ERROR_PROXY_FAILED,          // Connection to proxy server failed.
    DFM_IO_ERROR_PROXY_AUTH_FAILED,     // Proxy authentication failed.
    DFM_IO_ERROR_PROXY_NEED_AUTH,       // Proxy server needs authentication.
    DFM_IO_ERROR_PROXY_NOT_ALLOWED,     // Proxy connection is not allowed by ruleset.
    DFM_IO_ERROR_BROKEN_PIPE,           // Broken pipe.
    DFM_IO_ERROR_CONNECTION_CLOSED,     // Connection closed by peer.
    DFM_IO_ERROR_NOT_CONNECTED,         // Transport endpoint is not connected.
    DFM_IO_ERROR_MESSAGE_TOO_LARGE,     // Message too large.

    DFM_IO_ERROR_FAILED = 1000,         // Generic error condition for when an operation fails and no more specific DFMIOErrorEnum value is defined.
};

struct DFMIOError {
public:
    // Default constructor, no error.
    DFMIOError() : errorCode(DFM_IO_ERROR_NONE) {}
    // Constructor to set an error.
    DFMIOError(DFMIOErrorCode code) : errorCode(code) {}

    //! Get the error code.
    DFMIOErrorCode code() const { return errorCode; }

    //! Conversion to \c bool, returns \c true, iff !\ref IsError().
    operator bool() const { return errorCode != DFM_IO_ERROR_NONE; }
    //! Whether the result is an error.
    bool isError() const { return errorCode != DFM_IO_ERROR_NONE; }

    bool operator==(const DFMIOError& that) const { return errorCode == that.errorCode; }
    bool operator==(DFMIOErrorCode code) const { return errorCode == code; }
    friend bool operator==(DFMIOErrorCode code, const DFMIOError & err) { return code == err.errorCode; }

    //! Reset error code.
    void clear() { setCode(DFM_IO_ERROR_NONE); }
    //! Update error code and offset.
    void setCode(DFMIOErrorCode code) { errorCode = code; }

private:
    DFMIOErrorCode errorCode;
};

//END_IO_NAMESPACE

#endif // DFM_IO_ERROR_ERROR_H_
