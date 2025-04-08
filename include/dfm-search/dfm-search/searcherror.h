#ifndef DFM_SEARCH_ERROR_H
#define DFM_SEARCH_ERROR_H

#include <system_error>
#include <QString>

#include <dfm-search/dsearch_global.h>

DFM_SEARCH_BEGIN_NS

// 基础搜索错误枚举
enum class SearchErrorCode {
    Success = 0,

    // 系统资源相关
    PermissionDenied = 1,

    // 查询通用错误
    InvalidQuery = 100,
    PathIsEmpty,
    PathNotFound,   // 搜索路径不存在
    SearchTimeout,   // 搜索超时
    InternalError,   // 内部错误
    InvalidBoolean,

    // ... 其他通用错误
};

// 文件名搜索特定错误
enum class FileNameSearchErrorCode {
    KeywordIsEmpty = 1000,
    KeywordTooLong,

    // 拼音搜索相关
    InvalidPinyinFormat = 1050,

    // 文件类型过滤相关
    InvalidFileTypes = 1100,

    // 文件名索引特定错误
    FileNameIndexNotFound = 1200,
    FileNameIndexException
};

// 内容搜索特定错误
enum class ContentSearchErrorCode {
    KeywordTooShort = 2000,

    // 内容索引特定错误
    ContentIndexNotFound = 2200,
    ContentIndexException
};

// 搜索错误分类基类
class SearchErrorCategory : public std::error_category
{
public:
    const char *name() const noexcept override { return "search_error"; }
    std::string message(int ev) const override;
    virtual QString qMessage(int ev) const;   // Qt友好的错误消息
};

// 文件名搜索错误分类
class FileNameSearchErrorCategory : public SearchErrorCategory
{
public:
    const char *name() const noexcept override { return "filename_search_error"; }
    std::string message(int ev) const override;
};

// 内容搜索错误分类
class ContentSearchErrorCategory : public SearchErrorCategory
{
public:
    const char *name() const noexcept override { return "content_search_error"; }
    std::string message(int ev) const override;
};

// 获取错误分类单例
const SearchErrorCategory &search_category();
const FileNameSearchErrorCategory &filename_search_category();
const ContentSearchErrorCategory &content_search_category();

inline std::error_code make_error_code(SearchErrorCode ec)
{
    return std::error_code((int)ec, search_category());
}

inline std::error_code make_error_code(FileNameSearchErrorCode ec)
{
    return std::error_code((int)ec, filename_search_category());
}

inline std::error_code make_error_code(ContentSearchErrorCode ec)
{
    return std::error_code((int)ec, content_search_category());
}

// 错误条件包装类
class SearchError
{
public:
    SearchError()
        : m_code(make_error_code(SearchErrorCode::Success)) { }
    SearchError(SearchErrorCode code)
        : m_code(make_error_code(code)) { }
    SearchError(FileNameSearchErrorCode code)
        : m_code(make_error_code(code)) { }
    SearchError(ContentSearchErrorCode code)
        : m_code(make_error_code(code)) { }

    bool isError() const { return m_code.value() != static_cast<int>(SearchErrorCode::Success); }
    const std::error_code &code() const { return m_code; }
    QString message() const;
    QString name() const;

private:
    std::error_code m_code;
};

DFM_SEARCH_END_NS

// 注册元类型
Q_DECLARE_METATYPE(DFMSEARCH::SearchError)

#endif   // DFM_SEARCH_ERROR_H
