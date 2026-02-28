// SPDX-FileCopyrightText: 2025 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include "dfm-search/searcherror.h"

DFM_SEARCH_BEGIN_NS

// 实现基础错误分类的消息
std::string SearchErrorCategory::message(int ev) const
{
    switch (static_cast<SearchErrorCode>(ev)) {
    case SearchErrorCode::Success:
        return "Success: The operation completed successfully.";
    case SearchErrorCode::InvalidQuery:
        return "Invalid search query: The provided search query is not valid. Please check the syntax and try again.";
    case SearchErrorCode::PathIsEmpty:
        return "Path is empty: The search path provided is empty. Please specify a valid path.";
    case SearchErrorCode::PathNotFound:
        return "Path not found: The specified search path does not exist. Please verify the path.";
    case SearchErrorCode::SearchTimeout:
        return "Search timeout: The search operation took too long and has timed out. Please try again with a more specific query.";
    case SearchErrorCode::InternalError:
        return "Internal error: An unexpected error occurred during the search operation. Please check the logs for more details.";
    case SearchErrorCode::InvalidBoolean:
        return "Invalid boolean: The boolean operation specified in the query is not valid. Please check the query syntax.";
    case SearchErrorCode::InvalidSerchMethod:
        return "Invalid search method";
        // ... 其他错误消息
    default:
        return "Unknown error: An unknown error occurred. Please contact support.";
    }
}

QString SearchErrorCategory::qMessage(int ev) const
{
    return QString::fromStdString(message(ev));
}

// 实现文件名搜索错误分类的消息
std::string FileNameSearchErrorCategory::message(int ev) const
{
    switch (static_cast<FileNameSearchErrorCode>(ev)) {
    case FileNameSearchErrorCode::KeywordIsEmpty:
        return "Keyword is empty: The search keyword cannot be empty. Please provide a valid keyword.";
    case FileNameSearchErrorCode::KeywordTooLong:
        return "Keyword too long: The search keyword exceeds the maximum allowed length. Please shorten the keyword.";
    case FileNameSearchErrorCode::InvalidPinyinFormat:
        return "Invalid Pinyin format: The provided Pinyin format is not valid. Please check the format and try again.";
    case FileNameSearchErrorCode::InvalidFileTypes:
        return "Invalid file types: One or more specified file types are not valid. Please check the file types and try again.";
    case FileNameSearchErrorCode::FileNameIndexNotFound:
        return "File name index not found: The search index for file names could not be found. Please ensure the index is created.";
    case FileNameSearchErrorCode::FileNameIndexException:
        return "File name index exception: An error occurred while accessing the file name index. Please check the index integrity.";
    // ... 其他错误消息
    default:
        return "Unknown filename search error: An unknown error occurred related to filename search. Please contact support.";
    }
}

std::string ContentSearchErrorCategory::message(int ev) const
{
    switch (static_cast<ContentSearchErrorCode>(ev)) {
    case ContentSearchErrorCode::KeywordTooShort:
        return "Keyword too short: The search keyword is too short to perform a search. Please provide a longer keyword.";
    case ContentSearchErrorCode::WildcardNotSupported:
        return "Wildcard not supported: Wildcard search is not supported for content search. Please use simple or boolean query instead.";
    case ContentSearchErrorCode::ContentIndexNotFound:
        return "Content index not found: The content index could not be found. Please ensure the index is created.";
    case ContentSearchErrorCode::ContentIndexException:
        return "Content index exception: An error occurred while accessing the content index. Please check the index integrity.";
    // ... 其他错误消息
    default:
        return "Unknown content search error: An unknown error occurred related to content search. Please contact support.";
    }
}

// ... 实现其他错误分类的消息方法 ...

// 获取错误分类单例
const SearchErrorCategory &search_category()
{
    static SearchErrorCategory c;
    return c;
}

const FileNameSearchErrorCategory &filename_search_category()
{
    static FileNameSearchErrorCategory c;
    return c;
}

const ContentSearchErrorCategory &content_search_category()
{
    static ContentSearchErrorCategory c;
    return c;
}

// ... 实现其他错误分类单例 ...

QString SearchError::message() const
{
    if (auto cat = dynamic_cast<const SearchErrorCategory *>(&m_code.category())) {
        return cat->qMessage(m_code.value());
    }
    return QString::fromStdString(m_code.message());
}

QString SearchError::name() const
{
    return QString::fromLocal8Bit(m_code.category().name());
}
DFM_SEARCH_END_NS
