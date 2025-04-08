#include "dfm-search/searcherror.h"

DFM_SEARCH_BEGIN_NS

// 实现基础错误分类的消息
std::string SearchErrorCategory::message(int ev) const
{
    // TODO (search):
    switch (static_cast<SearchErrorCode>(ev)) {
    case SearchErrorCode::Success:
        return "Success";
    case SearchErrorCode::InvalidQuery:
        return "Invalid search query";
    // ... 其他错误消息
    default:
        return "Unknown error";
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
    // TODO (search):
    // ... 其他错误消息
    default:
        return "Unknown filename search error";
    }
}

std::string ContentSearchErrorCategory::message(int ev) const
{
    // TODO (search):
    switch (static_cast<ContentSearchErrorCode>(ev)) {
    case ContentSearchErrorCode::KeywordTooShort:
        return "keyword too shortKeywordTooShort";
    // ... 其他错误消息
    default:
        return "Unknown content search error";
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
