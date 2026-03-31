# dfm-search 命名约定

## 类命名

### 公共接口类

**规则**: 大驼峰 (PascalCase)

```cpp
class SearchEngine;             // 搜索引擎
class AbstractSearchEngine;     // 抽象搜索引擎
class GenericSearchEngine;      // 通用搜索引擎
class FileNameSearchEngine;     // 文件名搜索引擎
class ContentSearchEngine;      // 内容搜索引擎
class SearchWorker;             // 搜索工作线程
class SearchFactory;            // 搜索工厂
class SearchQuery;              // 搜索查询
class SearchOptions;            // 搜索选项
class SearchResult;             // 搜索结果
```

### 策略类

**规则**: 功能描述 + `Strategy`

```cpp
class BaseSearchStrategy;       // 基础搜索策略
class FileNameBaseStrategy;     // 文件名基础策略
class FileNameIndexedStrategy;  // 文件名索引策略
class FileNameRealtimeStrategy; // 文件名实时策略
class ContentBaseStrategy;      // 内容基础策略
class ContentIndexedStrategy;   // 内容索引策略
class QueryBuilder;             // 查询构建器
class IndexManager;             // 索引管理器
```

### 数据类

**规则**: 类名 + `Data`

```cpp
class SearchResultData;         // 搜索结果数据
class SearchOptionsData;        // 搜索选项数据
class SearchWorkerPrivate;      // 工作线程私有数据
```

### API 类

**规则**: 功能描述 + `API`

```cpp
class FileNameOptionsAPI;       // 文件名选项 API
class FileNameResultAPI;        // 文件名结果 API
class ContentOptionsAPI;        // 内容选项 API
class ContentResultAPI;         // 内容结果 API
```

### 工厂类

**规则**: 功能描述 + `Factory`

```cpp
class SearchStrategyFactory;            // 搜索策略工厂
class FileNameSearchStrategyFactory;    // 文件名策略工厂
class ContentSearchStrategyFactory;     // 内容策略工厂
```

## 方法命名

### 公共方法

**规则**: 小驼峰 (camelCase)

```cpp
// 搜索操作
void search(const SearchQuery &query);
void cancel();
bool isCancelled() const;

// 属性访问
SearchType searchType() const;
void setSearchType(SearchType type);
SearchStatus status() const;
SearchOptions searchOptions() const;
void setSearchOptions(const SearchOptions &options);

// 结果处理
void handleSearchResult(const SearchResult &result);
void handleSearchFinished(const SearchResultList &results);
void handleErrorOccurred(const SearchError &error);
```

### 静态工厂方法

```cpp
class SearchFactory {
public:
    static SearchEngine *create(SearchType type, QObject *parent = nullptr);
    static SearchQuery createQuery(const QString &keyword, QueryType type);
};
```

## 成员变量命名

### 主成员变量

**规则**: `m_` 前缀 + 小驼峰 (camelCase)

```cpp
class GenericSearchEngine {
private:
    SearchOptions m_options;
    SearchQuery m_currentQuery;
    SearchResultList m_results;
    SearchError m_lastError;
    QTimer m_batchTimer;
    SearchResultList m_batchResults;
    std::atomic<SearchStatus> m_status;
    std::atomic<bool> m_cancelled;
};
```

### 指针成员

**规则**: `m_` + 描述名

```cpp
class GenericSearchEngine {
private:
    QThread m_workerThread;
    SearchWorker *m_worker;
};
```

### PIMPL 成员

**规则**: 用于 PIMPL 的指针用 `d_ptr` 或 `d`

```cpp
class SearchResult {
protected:
    std::unique_ptr<SearchResultData> d;  // PIMPL
};

class SearchEngine {
private:
    std::unique_ptr<AbstractSearchEngine> d_ptr;  // PIMPL
};

class FileNameIndexedStrategy {
private:
    std::unique_ptr<QueryBuilder> m_queryBuilder;
    std::unique_ptr<IndexManager> m_indexManager;
};
```

## 枚举命名

### 枚举类型

**规则**: 大驼峰 (PascalCase)

```cpp
enum SearchType { ... };
enum SearchStatus { ... };
enum SearchMethod { ... };
enum QueryType { ... };
```

### enum class 值

**规则**: 大驼峰 (PascalCase) 或 `k` + 大驼峰

```cpp
enum SearchType {
    FileName,
    Content,
    Custom = 50
};

enum SearchStatus {
    Ready,
    Searching,
    Finished,
    Cancelled,
    Error
};

enum SearchMethod {
    Indexed,
    Realtime
};

enum class SearchErrorCode {
    Success = 0,
    InvalidQuery = 100,
    PathIsEmpty,
    PathNotFound,
    SearchTimeout,
    InternalError
};
```

## 文件命名

| 类型 | 规则 | 示例 |
|------|------|------|
| 公共头文件 | 类名小写 + `.h` | `searchengine.h`, `filenamesearchapi.h` |
| 私有头文件 | 类名小写 + `.h` | `searchresultdata.h` |
| 实现文件 | 类名小写 + `.cpp` | `searchengine.cpp`, `indexedstrategy.cpp` |

## 命名空间

```cpp
// 全局头文件定义宏
#define DFMSEARCH dfmsearch
#define DFM_SEARCH_BEGIN_NS namespace DFMSEARCH {
#define DFM_SEARCH_END_NS }

// 使用
DFM_SEARCH_BEGIN_NS
class SearchEngine { /* ... */ };
DFM_SEARCH_END_NS
```

## Lambda 和回调命名

```cpp
// 命名清晰的 lambda
auto resultHandler = [this](const SearchResult &result) {
    m_results.append(result);
};

// 槽函数命名
private Q_SLOTS:
    void handleSearchResult(const SearchResult &result);
    void handleSearchFinished(const SearchResultList &results);
    void handleErrorOccurred(const SearchError &error);
```

## 示例代码引用

| 文件路径 | 描述 |
|---------|------|
| `include/dfm-search/dfm-search/searchoptions.h` | 成员变量命名 |
| `src/dfm-search/dfm-search-lib/core/genericsearchengine.h` | m_ 前缀成员变量 |
| `src/dfm-search/dfm-search-lib/filenamesearch/filenamestrategies/indexedstrategy.h` | 策略类命名 |
