# dfm-searcher 使用指南

> 从零开始，学会用 `dfm-searcher` 搜索文件和预览文件内容。

---

## 它是什么

`dfm-searcher` 是深度文件管理器的搜索命令行工具，基于 Lucene++ 索引引擎，支持：

- **按文件名搜索** — 用关键词、通配符、拼音找文件
- **按文件内容搜索** — 在文档内容中搜索关键词
- **按 OCR 文字搜索** — 在图片中识别的文字里搜索
- **语义搜索** — 用自然语言描述需求，自动解析意图
- **内容预览** — 按需读取已索引文件的内容片段

---

## 两种模式

`dfm-searcher` 有两种工作模式：

| 模式 | 命令格式 | 用途 |
|------|---------|------|
| **搜索模式** | `dfm-searcher [选项] <keyword> <search_path>` | 搜索目录下匹配的文件 |
| **预览模式** | `dfm-searcher preview [选项] [<keyword>] <path> [path...]` | 直接读取指定文件的内容片段 |

---

# 第一部分：搜索

## 一、文件名搜索

### 1. 最基本的搜索

```bash
dfm-searcher "报告" /home/user
```

在 `/home/user` 目录下搜索文件名包含"报告"的文件。

输出示例：

```
/home/user/工作报告.txt
/home/user/2026年报告.docx
/home/user/reports/年度报告.pdf
```

### 2. 限制结果数量

```bash
dfm-searcher --max-results=10 "报告" /home/user
```

最多返回 10 条结果。`--max-results=0` 表示不限制（默认）。

### 3. 区分大小写

```bash
dfm-searcher --case-sensitive "README" /home/user
```

默认搜索不区分大小写。加上 `--case-sensitive` 后 "readme" 不会匹配 "README"。

### 4. 包含隐藏文件

```bash
dfm-searcher --include-hidden "config" /home/user
```

默认不搜索隐藏文件（以 `.` 开头的文件或目录）。加上此选项后包含隐藏文件。

### 5. 实时搜索 vs 索引搜索

```bash
# 索引搜索（默认）：快，但依赖预建索引
dfm-searcher "报告" /home/user

# 实时搜索：慢，但不需要索引，直接遍历文件系统
dfm-searcher --method=realtime "报告" /home/user
```

| 方法 | 速度 | 需要索引 | 适用场景 |
|------|------|:---:|------|
| `--method=indexed`（默认） | 快 | 是 | 日常使用 |
| `--method=realtime` | 慢 | 否 | 索引未覆盖的路径、最新文件 |

---

## 二、内容搜索

### 6. 搜索文件内容

```bash
dfm-searcher --type=content "会议纪要" /home/user/Documents
```

在 `/home/user/Documents` 下搜索文件**内容**包含"会议纪要"的文件。

### 7. 搜索内容并显示预览

```bash
dfm-searcher --type=content -v "会议纪要" /home/user/Documents
```

加 `-v`（verbose）后，每条结果会附带内容预览片段（高亮显示关键词）。

### 8. 控制预览长度

```bash
dfm-searcher --type=content --max-preview=500 -v "会议" /home/user/Documents
```

预览片段最多 500 个字符（默认 200）。

---

## 三、OCR 搜索

### 9. 搜索图片中的文字

```bash
dfm-searcher --type=ocr "登录界面" /home/user/Pictures
```

在图片中搜索 OCR 识别出的文字。支持 PNG、JPG 等格式。

### 10. OCR 搜索 + 预览

```bash
dfm-searcher --type=ocr -v "验证码" /home/user/Pictures
```

输出会包含匹配的 OCR 文字片段。

---

## 四、通配符搜索

### 11. 用 * 和 ? 搜索

```bash
# 匹配所有以 "report" 开头的文件名
dfm-searcher --wildcard "report*" /home/user

# 匹配 "report" + 任意一个字符 + ".txt"
dfm-searcher --wildcard "report?.txt" /home/user

# 匹配所有 .pdf 文件名
dfm-searcher --wildcard "*.pdf" /home/user
```

| 通配符 | 含义 | 示例 |
|--------|------|------|
| `*` | 任意多个字符 | `report*` 匹配 report2026.txt、reports.pdf |
| `?` | 任意一个字符 | `report?` 匹配 reports、reportA，不匹配 reportAB |

---

## 五、布尔搜索（多关键词）

### 12. AND 搜索（所有关键词都要匹配）

```bash
# 用逗号分隔：同时包含 "报告" 和 "2026"
dfm-searcher --query=boolean "报告,2026" /home/user

# 用 & 分隔（同上）
dfm-searcher --query=boolean "报告&2026" /home/user
```

### 13. OR 搜索（任一关键词匹配即可）

```bash
# 用 | 分隔：包含 "报告" 或 "总结"
dfm-searcher --query=boolean "报告|总结" /home/user
```

| 分隔符 | 逻辑 | 示例 |
|--------|------|------|
| `\|` | OR（任一匹配） | `"报告\|总结"` → 匹配报告或总结 |
| `&` | AND（全部匹配） | `"报告&2026"` → 同时包含报告和2026 |
| `,` | AND（全部匹配） | `"报告,2026"` → 同上，向后兼容 |

---

## 六、拼音搜索

### 14. 全拼搜索

```bash
dfm-searcher --pinyin "baogao" /home/user
```

搜索文件名中拼音匹配 "baogao" 的文件，如"报告.txt"。

### 15. 拼音首字母搜索

```bash
dfm-searcher --pinyin-acronym "bg" /home/user
```

"bg" 匹配 "baogao"（报告）、"binggan"（饼干）等拼音首字母为 b+g 的文件名。

### 16. 拼音 + 布尔组合

```bash
dfm-searcher --pinyin --query=boolean "wendang,xinjian" /home/user
```

搜索文件名拼音同时匹配 "wendang"（文档）和 "xinjian"（新建）的文件。

---

## 七、文件类型过滤

### 17. 按类型过滤

```bash
dfm-searcher --file-types=doc,pic "报告" /home/user
```

只返回文档类型和图片类型的搜索结果。

支持的类型：

| 类型 | 说明 |
|------|------|
| `app` | 应用程序文件 |
| `archive` | 压缩文件 |
| `audio` | 音频文件 |
| `doc` | 文档文件 |
| `pic` | 图片文件 |
| `video` | 视频文件 |
| `dir` | 目录 |
| `other` | 其他 |

### 18. 按扩展名过滤

```bash
dfm-searcher --file-extensions=txt,pdf "报告" /home/user
```

只返回 .txt 和 .pdf 文件。

### 19. 类型 + 扩展名组合

```bash
dfm-searcher --file-types=doc --file-extensions=docx,odt "report" /home/user
```

---

## 八、排除目录

### 20. 排除指定目录

```bash
# 排除 /tmp 和 /var 目录
dfm-searcher --exclude=/tmp,/var "报告" /home/user
```

搜索时跳过 `/tmp` 和 `/var` 目录及其子目录，不搜索其中的文件。用逗号分隔多个排除路径。

### 21. 排除缓存等无关目录

```bash
# 排除缓存和回收站目录，减少无关结果
dfm-searcher --exclude=/home/user/.cache,/home/user/.local/share/Trash "报告" /home/user
```

排除缓存、临时文件等目录可以加快搜索速度、减少无关结果。该选项在搜索模式和语义模式下均可使用。

---

## 九、时间范围过滤

### 22. 最近 N 天

```bash
# 最近 3 天内修改的文件
dfm-searcher --time-last=3d "报告" /home/user

# 最近 2 小时
dfm-searcher --time-last=2h "报告" /home/user
```

支持的时间单位：

| 单位 | 含义 | 示例 |
|------|------|------|
| `m` | 分钟 | `--time-last=30m`（最近 30 分钟） |
| `h` | 小时 | `--time-last=2h`（最近 2 小时） |
| `d` | 天 | `--time-last=7d`（最近 7 天） |
| `w` | 周 | `--time-last=2w`（最近 2 周） |
| `M` | 月 | `--time-last=3M`（最近 3 个月） |
| `y` | 年 | `--time-last=1y`（最近 1 年） |

### 23. 预设时间范围

```bash
# 今天的文件
dfm-searcher --time-today "报告" /home/user

# 昨天的文件
dfm-searcher --time-yesterday "报告" /home/user

# 本周的文件
dfm-searcher --time-this-week "报告" /home/user

# 上个月的文件
dfm-searcher --time-last-month "报告" /home/user

# 今年的文件
dfm-searcher --time-this-year "报告" /home/user
```

所有预设选项：

`--time-today` / `--time-yesterday` / `--time-this-week` / `--time-last-week` / `--time-this-month` / `--time-last-month` / `--time-this-year` / `--time-last-year`

### 24. 自定义时间范围

```bash
# 按日期范围
dfm-searcher --time-range="2025-01-01,2025-12-31" "报告" /home/user

# 按精确时间范围
dfm-searcher --time-range="2025-06-01 09:00,2025-06-30 18:00" "报告" /home/user
```

### 25. 按创建时间 vs 修改时间

```bash
# 默认按修改时间（modify）
dfm-searcher --time-last=7d "报告" /home/user

# 按创建时间（birth）
dfm-searcher --time-field=birth --time-last=7d "报告" /home/user
```

### 26. 时间 + 实时搜索组合

```bash
dfm-searcher --method=realtime --time-last=7d "报告" /home/user
```

---

## 十、文件大小过滤

### 27. 按大小范围过滤

```bash
# 1MB 到 100MB 之间的文件
dfm-searcher --size-min=1M --size-max=100M "视频" /home/user
```

支持的大小单位：

| 单位 | 含义 | 示例 |
|------|------|------|
| （无） | 字节 | `--size-min=512`（至少 512 字节） |
| `K` | KB | `--size-min=1K`（至少 1KB） |
| `M` | MB | `--size-min=10M`（至少 10MB） |
| `G` | GB | `--size-max=1G`（最多 1GB） |
| `T` | TB | `--size-max=1T`（最多 1TB） |

### 28. 只限最小或最大

```bash
# 至少 10MB
dfm-searcher --size-min=10M "视频" /home/user

# 最多 1KB
dfm-searcher --size-max=1K "日志" /home/user
```

---

## 十一、语义搜索

### 29. 用自然语言搜索

```bash
dfm-searcher -s "最近3天的图片" /home/user
```

语义搜索会自动解析你的自然语言描述，提取时间、类型、关键词等维度，转化为搜索条件。

### 30. 语义搜索 + JSON

```bash
dfm-searcher -s -j "最近3天的图片" /home/user
```

JSON 输出会包含解析出的意图（intent）信息。

### 31. 语义搜索不要求路径

```bash
dfm-searcher -s "包含会议纪要的文档"
```

语义模式可以不指定搜索路径。

### 32. 语义搜索示例集

```bash
# 查找最近图片
dfm-searcher -s "最近的图片" /home/user

# 查找上周的文档
dfm-searcher -s "上周的文档" /home/user/Documents

# 查找大视频
dfm-searcher -s "大于1G的视频" /home/user

# 查找特定内容
dfm-searcher -s "内容包含会议纪要的文件" /home/user

# 语义 + 详细输出
dfm-searcher -s -v "最近3天的图片" /home/user
```

---

## 十二、输出格式

### 33. JSON 输出

```bash
dfm-searcher -j "报告" /home/user
```

输出结构化 JSON，方便脚本解析：

```json
{
    "type": "search",
    "searchType": "filename",
    "keyword": "报告",
    "totalResults": 2,
    "results": [
        {
            "path": "/home/user/工作报告.txt",
            "filename": "工作报告.txt"
        },
        {
            "path": "/home/user/2026年报告.docx",
            "filename": "2026年报告.docx"
        }
    ]
}
```

### 34. 详细输出（verbose）

```bash
dfm-searcher -v --type=content "会议" /home/user/Documents
```

`-v` 启用详细模式，结果中会包含文件大小、修改时间、高亮内容预览等额外信息。

### 35. JSON + verbose 组合

```bash
dfm-searcher -j -v --type=content "会议" /home/user/Documents
```

---

## 十三、组合搜索

### 36. 文件名 + 文件类型 + 时间

```bash
dfm-searcher --file-types=doc --time-last=7d "报告" /home/user
```

最近 7 天内的文档类型文件，文件名包含"报告"。

### 37. 内容搜索 + 文件名过滤 + 大小过滤

```bash
dfm-searcher --type=content --filename="季度" --size-min=1M "财务" /home/user
```

在内容包含"财务"且文件名包含"季度"、大小至少 1MB 的文件中搜索。

### 38. 实时搜索 + 时间 + 大小 + 类型

```bash
dfm-searcher --method=realtime --time-last=30d --size-min=10M --file-types=video "监控" /home/user
```

---

# 第二部分：内容预览（preview 子命令）

`preview` 子命令用来**直接读取**指定文件的内容片段，不需要运行完整搜索。

## 十四、基本用法

### 39. 读取文件全部内容

```bash
dfm-searcher preview /home/user/notes.txt
```

无 keyword 且未设 `--max-preview` 时，输出文档**全部内容**。

```
/home/user/notes.txt
  2026年7月10日 工作计划：1.完成需求文档 2.代码评审 3.修复bug
  今天重点完成代码评审部分，明天开始写自动化测试…
```

### 40. 读取文件内容（JSON 格式）

```bash
dfm-searcher preview /home/user/notes.txt -j
```

```json
{
    "type": "preview",
    "searchType": "semantic",
    "keyword": "",
    "offset": 0,
    "maxLength": 0,
    "totalResults": 1,
    "results": [
        {
            "path": "/home/user/notes.txt",
            "content": "2026年7月10日 工作计划：1.完成需求文档 2.代码评审 3.修复bug\n今天重点完成代码评审部分，明天开始写自动化测试…",
            "keywordOffset": -1
        }
    ]
}
```

> `maxLength: 0` 表示无限制，返回全文。

---

## 十五、用 --offset 和 --max-preview 控制读取范围

无 keyword 时，默认输出全文。加 `--offset` 可以从指定位置开始读，加 `--max-preview` 可以限制读取长度。

### 41. 从第 100 个字符开始读（不限长度）

```bash
dfm-searcher preview /home/user/notes.txt --offset=100
```

从第 100 个字符开始，读到文件末尾。

### 42. 只读 50 个字符

```bash
dfm-searcher preview /home/user/notes.txt --max-preview=50
```

### 43. 从第 100 个字符开始，只读 50 个字符

```bash
dfm-searcher preview /home/user/notes.txt --offset=100 --max-preview=50
```

### 44. 翻页式读取

第一次读取：

```bash
dfm-searcher preview /home/user/notes.txt --max-preview=100 -j
```

第二次，offset=100：

```bash
dfm-searcher preview /home/user/notes.txt --offset=100 --max-preview=100 -j
```

第三次，offset=200：

```bash
dfm-searcher preview /home/user/notes.txt --offset=200 --max-preview=100 -j
```

以此类推，每次读 100 个字符。

### 45. offset 超过文件长度 → 返回空

```bash
dfm-searcher preview /home/user/notes.txt --offset=99999 -j
```

```json
{
    "results": [
        {
            "content": "",
            "keywordOffset": -1
        }
    ]
}
```

空内容说明已读到文件末尾。

---

## 十六、用 keyword 搜索并预览

### 46. 搜索关键词并预览

```bash
dfm-searcher preview "bug" /home/user/notes.txt
```

```
/home/user/notes.txt
  bug修复：修复了文件管理器在删除大量文件时的崩溃问题
```

内容从关键词所在位置开始，往后最多 200 个字符。

### 47. 搜索关键词 + JSON

```bash
dfm-searcher preview "bug" /home/user/notes.txt -j
```

```json
{
    "type": "preview",
    "keyword": "bug",
    "offset": 0,
    "maxLength": 200,
    "results": [
        {
            "path": "/home/user/notes.txt",
            "content": "bug修复：修复了文件管理器在删除大量文件时的崩溃问题",
            "keywordOffset": 156
        }
    ]
}
```

`keywordOffset: 156` 表示 "bug" 在全文第 156 个字符处。

### 48. 从指定位置开始搜索关键词

如果 "bug" 在文件里出现多次，用 `--offset` 跳过前面的出现：

```bash
# 从第 200 个字符开始搜索 "bug"
dfm-searcher preview "bug" /home/user/notes.txt --offset=200
```

### 49. offset 后面没有关键词 → 返回空

```bash
dfm-searcher preview "bug" /home/user/notes.txt --offset=500 -j
```

```json
{
    "results": [
        {
            "content": "",
            "keywordOffset": -1
        }
    ]
}
```

### 50. 搭配搜索结果使用

搜索返回 `keywordOffset`，可以用它作为 preview 的 `--offset`：

```bash
# 假设搜索返回 keywordOffset=320
dfm-searcher preview "关键词" /home/user/doc.txt --offset=320 --max-preview=200 -j
```

---

## 十七、批量预览多个文件

### 51. 同时预览多个文件

```bash
dfm-searcher preview "报告" doc1.txt doc2.txt doc3.txt
```

```
/home/user/doc1.txt
  报告：2026年Q1财务总结…
/home/user/doc2.txt
  报告：2026年Q2项目进展…
/home/user/doc3.txt
  (no content)
```

### 52. 批量预览 + JSON

```bash
dfm-searcher preview "报告" doc1.txt doc2.txt doc3.txt -j
```

```json
{
    "type": "preview",
    "keyword": "报告",
    "offset": 0,
    "maxLength": 200,
    "totalResults": 3,
    "results": [
        {
            "path": "/home/user/doc1.txt",
            "content": "报告：2026年Q1财务总结…",
            "keywordOffset": 0
        },
        {
            "path": "/home/user/doc2.txt",
            "content": "报告：2026年Q2项目进展…",
            "keywordOffset": 5
        },
        {
            "path": "/home/user/doc3.txt",
            "content": "",
            "keywordOffset": -1
        }
    ]
}
```

---

## 十八、搜索类型

### 53. 自动检测（默认）

不给 `--type`，程序根据文件扩展名自动判断：
- `.txt` `.doc` `.md` 等 → 用内容索引（content）
- `.png` `.jpg` 等 → 用 OCR 索引（ocr）

### 54. 强制指定搜索类型

```bash
# 强制用 OCR 索引
dfm-searcher preview --type=ocr "截图" photo.png

# 强制用内容索引
dfm-searcher preview --type=content "会议" doc.txt
```

### 55. 预览 OCR 图片中的文字

```bash
dfm-searcher preview "登录" screenshot.png -j
```

```json
{
    "type": "preview",
    "searchType": "ocr",
    "keyword": "登录",
    "results": [
        {
            "path": "screenshot.png",
            "content": "登录界面请输入用户名和密码",
            "keywordOffset": 0
        }
    ]
}
```

---

## 十九、无关键词，纯读取内容

### 56. 读取文件前 200 个字符

```bash
dfm-searcher preview /home/user/doc.txt
```

### 57. 从第 500 个字符读取 100 个字符

```bash
dfm-searcher preview /home/user/doc.txt --offset=500 --max-preview=100
```

### 58. 无关键词 + JSON

```bash
dfm-searcher preview /home/user/doc.txt --offset=500 --max-preview=100 -j
```

```json
{
    "keyword": "",
    "offset": 500,
    "maxLength": 100,
    "results": [
        {
            "content": "这里是第500到600个字符之间的原始内容",
            "keywordOffset": -1
        }
    ]
}
```

`keywordOffset: -1` 因为没有给关键词。

---

# 第三部分：参考

## 选项速查表

### 搜索模式选项

| 选项 | 默认值 | 说明 |
|------|--------|------|
| `--type=<filename\|content\|ocr>` | `filename` | 搜索类型 |
| `--method=<indexed\|realtime>` | `indexed` | 搜索方法 |
| `--query=<simple\|boolean\|wildcard>` | `simple` | 查询类型 |
| `--wildcard` | 关 | 启用通配符搜索 |
| `--case-sensitive` | 关 | 区分大小写 |
| `--include-hidden` | 关 | 包含隐藏文件 |
| `--pinyin` | 关 | 启用拼音搜索 |
| `--pinyin-acronym` | 关 | 启用拼音首字母搜索 |
| `--file-types=<types>` | 无 | 按文件类型过滤 |
| `--file-extensions=<exts>` | 无 | 按扩展名过滤 |
| `--exclude=<paths>` | 无 | 排除指定目录，逗号分隔 |
| `--max-results=<n>` | `0`（不限） | 最大结果数 |
| `--max-preview=<n>` | `200` | 内容预览长度 |
| `--filename=<keyword>` | 无 | 在内容/OCR 搜索中按文件名过滤 |
| `--semantic`, `-s` | 关 | 启用语义搜索 |
| `--time-field=<birth\|modify>` | `modify` | 时间字段 |
| `--time-last=<N><unit>` | 无 | 最近 N 时间段 |
| `--time-today` 等预设 | 无 | 预设时间范围 |
| `--time-range=<start>,<end>` | 无 | 自定义时间范围 |
| `--size-min=<size>` | 无 | 最小文件大小 |
| `--size-max=<size>` | 无 | 最大文件大小 |
| `--json`, `-j` | 关 | JSON 输出 |
| `--verbose`, `-v` | 关 | 详细输出 |
| `--version` | — | 显示版本 |
| `--help` | — | 显示帮助 |

### Preview 模式选项

| 选项 | 默认值 | 说明 |
|------|--------|------|
| `--offset=<n>` | `0` | 从文档第几个字符开始阅读/搜索 |
| `--max-preview=<n>` | 无限制(无keyword) / `200`(有keyword) | 本次最多读取的字符数。设为 0 表示无限制 |
| `--type=<content\|ocr>` | 自动 | 强制指定搜索类型 |
| `-j` / `--json` | 关 | JSON 输出 |

---

## JSON 输出字段说明

### 搜索模式 JSON

| 字段 | 位置 | 说明 |
|------|------|------|
| `type` | 顶层 | `"search"` |
| `searchType` | 顶层 | `"filename"` / `"content"` / `"ocr"` / `"semantic"` |
| `keyword` | 顶层 | 搜索关键词 |
| `totalResults` | 顶层 | 结果总数 |
| `path` | result 项 | 文件路径 |
| `filename` | result 项 | 文件名（verbose 模式） |
| `highlightedContent` | result 项 | 高亮内容预览（content/ocr + verbose） |

### Preview 模式 JSON

| 字段 | 位置 | 说明 |
|------|------|------|
| `type` | 顶层 | `"preview"` |
| `searchType` | 顶层 | `"content"` / `"ocr"` / `"semantic"` |
| `keyword` | 顶层 | 搜索关键词，空字符串表示无关键词 |
| `offset` | 顶层 | 本次使用的偏移量 |
| `maxLength` | 顶层 | 本次最大读取长度 |
| `totalResults` | 顶层 | 结果总数（等于文件数） |
| `path` | result 项 | 文件路径 |
| `content` | result 项 | 截取到的内容片段 |
| `keywordOffset` | result 项 | keyword 在全文中的位置（-1 = 无 keyword 或未匹配） |

---

## 字符计数说明

`--offset` 以**字符**（QChar）为单位，不是字节。

一个汉字算一个字符，一个换行符 `\n` 也算一个字符。

例如：

```
你好\n世界
```

| 位置 | 字符 |
|------|------|
| 0 | 你 |
| 1 | 好 |
| 2 | \n |
| 3 | 世 |
| 4 | 界 |

要从"世"开始读，`--offset=3`。

---

## 常见问题

### Q: preview 报 "(no content)" 是什么意思？

可能原因：
1. 文件未被索引（需要先建索引）
2. offset 超出文件长度
3. 给了 keyword 但文件中没有这个词（或在 offset 之后没有）

### Q: preview 和普通搜索有什么区别？

普通搜索搜索整个目录返回匹配文件列表。preview 不搜索，而是直接读取指定文件的内容片段。

### Q: 可以预览任意文件吗？

不能。preview 只能读取已被索引的文件。文件需要先通过文件管理器的内容索引功能建立索引。

### Q: --offset 是字节还是字符？

是字符（QChar）。一个中文字算一个字符，一个换行符也算一个字符。不是字节。

### Q: 索引搜索找不到文件怎么办？

可以改用实时搜索：`--method=realtime`，它会直接遍历文件系统，不需要索引。

### Q: --version 和 -v 冲突吗？

不冲突。`--version` 是长选项显示版本，`-v` 是 `--verbose` 的短选项。两者不冲突。

### Q: 选项可以放在任何位置吗？

可以。`dfm-searcher --type=content "hello" /home/user` 和 `dfm-searcher "hello" /home/user --type=content` 效果一样。
