// SPDX-FileCopyrightText: 2025 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DFM_SEARCH_ERROR_H
#define DFM_SEARCH_ERROR_H

#include <system_error>
#include <QString>

#include <dfm-search/dsearch_global.h>

DFM_SEARCH_BEGIN_NS

// Enumeration for basic search error codes
enum class SearchErrorCode {
    Success = 0,   // No error occurred

    // Errors related to system resources
    PermissionDenied = 1,   // Access to the resource is denied

    // General query errors
    InvalidQuery = 100,   // The query is invalid
    PathIsEmpty,   // The search path is empty
    PathNotFound,   // The search path does not exist
    SearchTimeout,   // The search operation timed out
    InternalError,   // An internal error occurred
    InvalidBoolean,   // A boolean value is invalid
    InvalidSerchMethod,

    // ... other general errors
};

// Enumeration for file name search specific error codes
enum class FileNameSearchErrorCode {
    KeywordIsEmpty = 1000,   // The search keyword is empty
    KeywordTooLong,   // The search keyword exceeds the maximum length

    // Errors related to pinyin search
    InvalidPinyinFormat = 1050,   // The format of the pinyin keyword is invalid

    // Errors related to file type filtering
    InvalidFileTypes = 1100,   // The specified file types are invalid

    // Errors specific to file name indexing
    FileNameIndexNotFound = 1200,   // The file name index could not be found
    FileNameIndexException   // An exception occurred with the file name index
};

// Enumeration for content search specific error codes
enum class ContentSearchErrorCode {
    KeywordTooShort = 2000,   // The search keyword is too short
    WildcardNotSupported = 2001,   // Wildcard search is not supported for content search

    // Errors related to content indexing
    ContentIndexNotFound = 2200,   // The content index could not be found
    ContentIndexException   // An exception occurred with the content index
};

// Base class for search error categories
class SearchErrorCategory : public std::error_category
{
public:
    const char *name() const noexcept override { return "search_error"; }   // Returns the name of the error category
    std::string message(int ev) const override;   // Returns a message corresponding to the error code
    virtual QString qMessage(int ev) const;   // Returns a Qt-friendly error message
};

// Class for file name search error categories
class FileNameSearchErrorCategory : public SearchErrorCategory
{
public:
    const char *name() const noexcept override { return "filename_search_error"; }   // Returns the name of the file name search error category
    std::string message(int ev) const override;   // Returns a message corresponding to the file name search error code
};

// Class for content search error categories
class ContentSearchErrorCategory : public SearchErrorCategory
{
public:
    const char *name() const noexcept override { return "content_search_error"; }   // Returns the name of the content search error category
    std::string message(int ev) const override;   // Returns a message corresponding to the content search error code
};

// Functions to get singleton instances of error categories
const SearchErrorCategory &search_category();   // Returns the singleton instance of the search error category
const FileNameSearchErrorCategory &filename_search_category();   // Returns the singleton instance of the file name search error category
const ContentSearchErrorCategory &content_search_category();   // Returns the singleton instance of the content search error category

// Function to create an error code from a SearchErrorCode
inline std::error_code make_error_code(SearchErrorCode ec)
{
    return std::error_code((int)ec, search_category());
}

// Function to create an error code from a FileNameSearchErrorCode
inline std::error_code make_error_code(FileNameSearchErrorCode ec)
{
    return std::error_code((int)ec, filename_search_category());
}

// Function to create an error code from a ContentSearchErrorCode
inline std::error_code make_error_code(ContentSearchErrorCode ec)
{
    return std::error_code((int)ec, content_search_category());
}

// Class to wrap search error conditions
class SearchError
{
public:
    SearchError()
        : m_code(make_error_code(SearchErrorCode::Success)) { }   // Default constructor initializes to success
    SearchError(SearchErrorCode code)
        : m_code(make_error_code(code)) { }   // Constructor initializes with a SearchErrorCode
    SearchError(FileNameSearchErrorCode code)
        : m_code(make_error_code(code)) { }   // Constructor initializes with a FileNameSearchErrorCode
    SearchError(ContentSearchErrorCode code)
        : m_code(make_error_code(code)) { }   // Constructor initializes with a ContentSearchErrorCode

    bool isError() const { return m_code.value() != static_cast<int>(SearchErrorCode::Success); }   // Checks if there is an error
    const std::error_code &code() const { return m_code; }   // Returns the error code
    QString message() const;   // Returns the error message
    QString name() const;   // Returns the error name

private:
    std::error_code m_code;   // Holds the error code
};

DFM_SEARCH_END_NS

// Register the SearchError type for Qt's meta-object system
Q_DECLARE_METATYPE(DFMSEARCH::SearchError)

#endif   // DFM_SEARCH_ERROR_H
