# dfm-search 错误处理规范

## 概述

dfm-search 使用现代 C++ 的 `std::error_code` 模式进行错误处理，不使用异常。

## 错误类型

### 1. 分层错误码设计

**位置**: `include/dfm-search/dfm-search/searcherror.h:16-83`

```cpp
// 通用搜索错误码 (0-999)
enum class SearchErrorCode : int {
    Success = 0,
    PermissionDenied = 1,
    InvalidQuery = 100,
    PathIsEmpty,
    PathNotFound,
    SearchTimeout,
    InternalError,
    InvalidBoolean,
    InvalidSerchMethod,
};

// 文件名搜索错误码 (1000-1999)
enum class FileNameSearchErrorCode : int {
    KeywordIsEmpty = 1000,
    KeywordTooLong,
    InvalidPinyinFormat = 1050,
    InvalidFileTypes = 1100,
    FileNameIndexNotFound = 1200,
    FileNameIndexException = 1300,
};

// 内容搜索错误码 (2000-2999)
enum class ContentSearchErrorCode : int {
    KeywordTooShort = 2000,
    WildcardNotSupported = 2001,
    ContentIndexNotFound = 2200,
    ContentIndexException = 2300,
};
```

### 2. 错误分类（ErrorCategory）

```cpp
class SearchErrorCategory : public std::error_category
{
public:
    const char *name() const noexcept override { return "search_error"; }

    std::string message(int ev) const override {
        switch (static_cast<SearchErrorCode>(ev)) {
        case SearchErrorCode::Success:
            return "Success: The operation completed successfully.";
        case SearchErrorCode::InvalidQuery:
            return "Invalid search query: The provided search query is not valid.";
        // ...
        default:
            return "Unknown error";
        }
    }

    // Qt 友好的消息获取
    virtual QString qMessage(int ev) const {
        return QString::fromStdString(message(ev));
    }
};

// 继承支持派生类型
class FileNameSearchErrorCategory : public SearchErrorCategory {
    std::string message(int ev) const override {
        switch (static_cast<FileNameSearchErrorCode>(ev)) {
        case FileNameSearchErrorCode::KeywordIsEmpty:
            return "Keyword is empty: The search keyword cannot be empty.";
        // ...
        }
    }
};

class ContentSearchErrorCategory : public SearchErrorCategory { /* ... */ };
```

### 3. 错误包装类

**位置**: `include/dfm-search/dfm-search/searcherror.h:109-128`

```cpp
class SearchError {
public:
    SearchError() = default;
    explicit SearchError(SearchErrorCode code);
    explicit SearchError(FileNameSearchErrorCode code);
    explicit SearchError(ContentSearchErrorCode code);

    bool isError() const { return m_code.value() != 0; }
    const std::error_code &code() const { return m_code; }
    QString message() const;
    QString name() const;

private:
    std::error_code m_code;
};
```

## make_error_code 实现

**位置**: `src/dfm-search/dfm-search-lib/core/searcherror.cpp`

```cpp
// 单例模式提供 ErrorCategory 实例
const SearchErrorCategory &search_category() {
    static SearchErrorCategory c;
    return c;
}

const FileNameSearchErrorCategory &filename_search_category() {
    static FileNameSearchErrorCategory c;
    return c;
}

const ContentSearchErrorCategory &content_search_category() {
    static ContentSearchErrorCategory c;
    return c;
}

// 在 searcherror.h 中声明
inline std::error_code make_error_code(SearchErrorCode ec) {
    return std::error_code(static_cast<int>(ec), search_category());
}

inline std::error_code make_error_code(FileNameSearchErrorCode ec) {
    return std::error_code(static_cast<int>(ec), filename_search_category());
}
```

## 错误处理模式

### 1. 信号报告错误

```cpp
class AbstractSearchEngine : public QObject {
    Q_OBJECT
Q_SIGNALS:
    void errorOccurred(const DFMSEARCH::SearchError &error);
};

// 使用
connect(engine, &SearchEngine::errorOccurred, [](const SearchError &err) {
    qWarning() << "Search error:" << err.message();
});
```

### 2. 返回错误状态

```cpp
class IndexManager {
public:
    bool buildIndex(const QString &path, SearchError &error) {
        if (path.isEmpty()) {
            error = SearchError(FileNameSearchErrorCode::KeywordIsEmpty);
            return false;
        }
        // ... 构建索引
        return true;
    }
};

// 使用
SearchError err;
if (!manager.buildIndex(path, err)) {
    qWarning() << err.message();
}
```

## 错误处理最佳实践

### DO - 应该做

1. **使用标准错误码模式**
```cpp
// 返回 + 错误参数模式
bool doSomething(SearchError &error);

// 使用信号异步报告
Q_SIGNAL void errorOccurred(const SearchError &error);
```

2. **单例模式管理 ErrorCategory**
```cpp
const SearchErrorCategory &search_category() {
    static SearchErrorCategory c;
    return c;
}
```

3. **清晰的错误分层**
```cpp
// 基础错误: 0-999
enum class SearchErrorCode { ... };

// 文件名错误: 1000-1999
enum class FileNameSearchErrorCode { ... };

// 内容错误: 2000-2999
enum class ContentSearchErrorCode { ... };
```

### DON'T - 不应该做

1. **不要使用异常**
```cpp
// 错误:
if (path.isEmpty()) {
    throw std::runtime_error("Path is empty");
}

// 正确:
if (path.isEmpty()) {
    error = SearchError(FileNameSearchErrorCode::KeywordIsEmpty);
    return false;
}
```

2. **不要使用 std::shared_ptr 管理错误**
```cpp
// 错误:
std::shared_ptr<SearchError> error;

// 正确:
SearchError error;  // 值语义，轻量级
```

3. **不要使用裸指针管理 ErrorCategory**
```cpp
// 错误:
static SearchErrorCategory *c = new SearchErrorCategory();
// 永远不会被 delete

// 正确:
static SearchErrorCategory c;  // 静态局部变量
```

## Qt 集成

```cpp
// qMessage() 提供友好的 Qt 字符串接口
class SearchError {
public:
    QString message() const {
        if (auto cat = dynamic_cast<const SearchErrorCategory *>(&m_code.category())) {
            return cat->qMessage(m_code.value());
        }
        return QString::fromStdString(m_code.message());
    }

    QString name() const {
        return QString::fromLocal8Bit(m_code.category().name());
    }
};
```

## 示例代码引用

| 文件路径 | 描述 |
|---------|------|
| `include/dfm-search/dfm-search/searcherror.h` | 错误码和 ErrorCategory 定义 |
| `src/dfm-search/dfm-search-lib/core/searcherror.cpp` | ErrorCategory 实现 |
| `src/dfm-search/dfm-search-lib/core/abstractsearchengine.h` | 错误信号声明 |
