// SPDX-FileCopyrightText: 2023 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

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
    QString errorMsg() const
    {
        if (!errMsg.isEmpty())
            return errMsg;
        return GetError_En(errorCode);
    }

    //! Conversion to \c bool, returns \c true, iff !\ref IsError().
    operator bool() const
    {
        return errorCode != DFM_IO_ERROR_NONE;
    }
    //! Whether the result is an error.
    bool isError() const { return errorCode != DFM_IO_ERROR_NONE; }

    bool operator==(const DFMIOError &that) const
    {
        return errorCode == that.errorCode && errMsg == that.errMsg;
    }

    //! Reset error code.
    void clear() { setCode(DFM_IO_ERROR_NONE); }
    //! Update error code and offset.
    void setCode(DFMIOErrorCode code) { errorCode = code; }
    //! Update error message.
    void setMessage(const QString &msg) { errMsg = msg; }

private:
    DFMIOErrorCode errorCode;
    QString errMsg;
};

#endif   // DFM_IO_ERROR_ERROR_H_
