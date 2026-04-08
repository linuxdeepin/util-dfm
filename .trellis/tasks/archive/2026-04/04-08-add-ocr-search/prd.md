# Add OCR Search Type to dfm-search

## Goal

在 dfm-search 库中新增 OCR 搜索类型，支持对图片中识别出的文字进行全文搜索。OCR 搜索作为 Content 搜索的简化版，只支持索引策略，不支持高亮返回。

## Requirements

### 1. 枚举和常量定义

- [ ] 在 `SearchType` 枚举中新增 `Ocr` 类型
- [ ] 在 `lucene_field_names.h` 中新增 `LuceneFieldNames::OcrText` 命名空间
- [ ] 在 `searcherror.h` 中新增 `OcrSearchErrorCode` 枚举

### 2. OCR 搜索模块结构

参考 `contentsearch` 目录结构，创建 `ocrtextsearch` 模块：

```
src/dfm-search/dfm-search-lib/ocrtextsearch/
├── ocrtextsearchengine.h/.cpp    # OCR 搜索引擎
├── ocrtextsearchapi.cpp          # API 实现
└── ocrtextstrategies/
    ├── basestrategy.h            # 策略基类
    └── indexedstrategy.h/.cpp    # 索引策略实现

include/dfm-search/dfm-search/
└── ocrtextsearchapi.h            # 公共 API 头文件
```

### 3. 功能实现

#### OcrTextIndexedStrategy

- 参考 `ContentIndexedStrategy` 实现
- 使用已定义的 OCR 索引路径（`Global::ocrTextIndexDirectory()`）
- 支持 `isFilenameContentMixedAndSearchEnabled` 混合搜索
- 复用现有的 Lucene 查询工具和分析器

#### OcrTextSearchEngine

- 继承 `GenericSearchEngine`
- 仅支持索引策略
- 注册到 `SearchFactory`

#### OcrTextOptionsAPI

- 参考 `ContentOptionsAPI` 实现
- 支持混合搜索选项

### 4. Lucene 字段定义

新增 OCR 字段（与 Content 结构一致）：

```cpp
namespace OcrText {
    constexpr const wchar_t kOcrContents[] = L"ocr_contents";
    constexpr const wchar_t kFilename[] = L"filename";
    constexpr const wchar_t kPath[] = L"path";
    constexpr const wchar_t kIsHidden[] = L"is_hidden";
    constexpr const wchar_t kAncestorPaths[] = L"ancestor_paths";
}
```

### 5. 错误码定义

新增 OCR 搜索错误码范围：3000-3999

```cpp
enum OcrSearchErrorCode {
    OcrIndexNotAvailable = 3000,
    OcrIndexPathError = 3001,
    OcrQueryError = 3002,
    // ...
};
```

## Acceptance Criteria

- [ ] `SearchType::Ocr` 枚举值已添加
- [ ] OCR 搜索模块目录结构完整
- [ ] `OcrTextIndexedStrategy` 可执行搜索
- [ ] 支持文件名+OCR内容混合搜索
- [ ] 通过工厂创建 OCR 搜索引擎
- [ ] 编译无错误
- [ ] 代码风格符合项目规范

## Technical Notes

### OCR 与 Content 的差异

| 项目 | Content | OCR |
|------|---------|-----|
| 索引目录 | fulltext-index | ocrtext-index |
| 内容字段 | `contents` | `ocr_contents` |
| 文件类型 | 文档 | 图片 |
| 高亮 | 支持 | 不需要 |

### 已有接口（可复用）

- `Global::ocrTextIndexDirectory()` - OCR 索引目录
- `Global::isOcrTextIndexAvailable()` - 索引可用性检查
- `Global::isPathInOcrTextIndexDirectory()` - 路径检查
- `Global::ocrTextIndexVersion()` - 索引版本

### 设计原则

1. **DRY**: 复用 Content 搜索的查询构建逻辑
2. **KISS**: OCR 作为简化版，不需要高亮等复杂特性
3. **YAGNI**: 仅实现索引策略，不预留实时策略扩展
