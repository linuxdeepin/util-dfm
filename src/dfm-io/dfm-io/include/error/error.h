#ifndef DFM_IO_ERROR_ERROR_H_
#define DFM_IO_ERROR_ERROR_H_

#include "en.h"

struct DFMIOError
{
public:
    // Default constructor, no error.
    DFMIOError()
        : errorCode(DFM_IO_ERROR_NONE) {}
    // Constructor to set an error.
    DFMIOError(DFMIOErrorCode code)
        : errorCode(code) {}

    //! Get the error code.
    DFMIOErrorCode code() const { return errorCode; }
    QString errorMsg() const { return GetError_En(errorCode); }

    //! Conversion to \c bool, returns \c true, iff !\ref IsError().
    operator bool() const
    {
        return errorCode != DFM_IO_ERROR_NONE;
    }
    //! Whether the result is an error.
    bool isError() const { return errorCode != DFM_IO_ERROR_NONE; }

    bool operator==(const DFMIOError &that) const { return errorCode == that.errorCode; }
    bool operator==(DFMIOErrorCode code) const { return errorCode == code; }
    friend bool operator==(DFMIOErrorCode code, const DFMIOError &err) { return code == err.errorCode; }

    //! Reset error code.
    void clear() { setCode(DFM_IO_ERROR_NONE); }
    //! Update error code and offset.
    void setCode(DFMIOErrorCode code) { errorCode = code; }

private:
    DFMIOErrorCode errorCode;
};

#endif   // DFM_IO_ERROR_ERROR_H_
