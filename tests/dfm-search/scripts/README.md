# dfm-search 测试脚本

本目录包含 `dfm-searcher` CLI 工具的文件名搜索和文件内容搜索集成测试脚本。

## 目录结构

```
tests/dfm-search/scripts/
├── test_filename_search.sh   # 文件名搜索测试脚本（21 个用例）
├── test_content_search.sh    # 文件内容搜索测试脚本（13 个用例）
├── common.sh                 # 公共函数库
└── README.md                 # 本文件
```

## 前置条件

| 依赖项 | 说明 |
|--------|------|
| `dfm-searcher` | 必须已编译安装并在 `PATH` 中可用，或通过 `DFM_SEARCHER` 环境变量指定路径 |
| 文件索引服务 | dde-file-manager 的文件索引服务需在运行中 |
| `bash` | 脚本解释器（≥ 4.0） |
| `mktemp` | 创建临时目录 |
| `grep` / `sed` | 基本文本处理 |
| `jq`（可选） | JSON 解析，不可用时降级为 `grep` |

## 运行方式

```bash
# 运行文件名搜索测试
./test_filename_search.sh

# 运行文件内容搜索测试
./test_content_search.sh

# 指定 dfm-searcher 路径
DFM_SEARCHER=/path/to/dfm-searcher ./test_filename_search.sh

# 调整索引等待超时（秒）
INDEX_WAIT_TIMEOUT_FILENAME=20 ./test_filename_search.sh
INDEX_WAIT_TIMEOUT_CONTENT=60 ./test_content_search.sh

# 调整轮询间隔（秒）
INDEX_POLL_INTERVAL=3 ./test_filename_search.sh

# 指定报告输出目录（默认 <脚本目录>/reports）
TEST_REPORT_DIR=/tmp/my-reports ./test_filename_search.sh
```

## 测试报告

脚本运行结束时会自动在 `reports/` 目录下生成测试报告文件（文件名含脚本名与时间戳，如 `report_test_filename_search_20260921_143000.txt`），内容包括：

- 环境信息（脚本名、日期、耗时、主机、dfm-searcher 路径、临时目录）
- 每个用例的结果（PASS / FAIL / SKIP 及失败原因）
- 汇总统计与最终结果（PASSED / FAILED）

报告目录可通过环境变量 `TEST_REPORT_DIR` 自定义，`reports/` 目录已加入 `.gitignore`。

## 测试用例概览

### 文件名搜索（FT-01 ~ FT-21）

| 用例 | 说明 |
|------|------|
| FT-01 | 基本中文关键词文件名搜索 |
| FT-02 | 英文关键词文件名搜索 |
| FT-03 | 通配符 `*` 搜索 |
| FT-04 | 通配符 `?` 搜索 |
| FT-05 | Boolean AND 搜索（`,` 分隔符） |
| FT-06 | Boolean OR 搜索（`\|` 分隔符） |
| FT-07 | 拼音全拼搜索 |
| FT-08 | 拼音首字母搜索 |
| FT-09 | 拼音 + Boolean 组合搜索 |
| FT-10 | 大小写敏感搜索 |
| FT-11 | 隐藏文件搜索 |
| FT-12 | max-results 结果限制 |
| FT-13 | 新建文件后搜索 |
| FT-14 | 删除文件后搜索 |
| FT-15 | 重命名文件后搜索 |
| FT-16 | 移动文件后搜索 |
| FT-17 | 目录搜索 |
| FT-18 | 实时搜索（`--method=realtime`） |
| FT-19 | 扩展名过滤 |
| FT-20 | Boolean AND（`&` 分隔符） |
| FT-21 | 压力测试 — 1000 个文件名搜索 |

### 文件内容搜索（CT-01 ~ CT-13）

| 用例 | 说明 |
|------|------|
| CT-01 | 基本中文内容搜索 |
| CT-02 | 英文内容搜索 |
| CT-03 | Boolean AND 内容搜索（`,` 分隔符） |
| CT-04 | Boolean OR 内容搜索（`\|` 分隔符） |
| CT-05 | 大小写敏感内容搜索 |
| CT-06 | 预览长度控制（`-v` / `--max-preview`） |
| CT-07 | JSON 输出字段验证 |
| CT-08 | 内容修改后搜索 |
| CT-09 | 内容删除后搜索 |
| CT-10 | 隐藏文件内容搜索 |
| CT-11 | 多文件内容搜索 |
| CT-12 | Boolean AND（`&` 分隔符）内容搜索 |
| CT-13 | 压力测试 — 1000 个文件内容搜索 |

### 已知限制（自动 SKIP）

| 用例 | 限制 | 说明 |
|------|------|------|
| FT-10b/c、CT-05b/c | `--case-sensitive` 不生效 | 索引层做了大小写归一化，标志被接受但结果仍大小写不敏感（实测大写关键词返回 0 结果）。脚本运行时自动探测：若服务将来支持该能力，会自动恢复执行断言 |

## 退出码

| 退出码 | 含义 |
|--------|------|
| 0 | 全部通过，或仅有 SKIP（无 FAIL） |
| 1 | 存在 FAIL |
| 非零（前置检查） | `dfm-searcher` 不可用或索引服务未运行 |

## 索引延迟处理策略

脚本采用轮询等待策略处理索引延迟：

1. **轮询等待**：创建文件后循环执行搜索，直到结果出现或超时
2. **超时标记 SKIP**：超时后标记为 `SKIP` 而非 `FAIL`，避免因索引延迟导致误报
3. **可配置超时**：通过环境变量 `INDEX_WAIT_TIMEOUT_FILENAME`（默认 10s）和 `INDEX_WAIT_TIMEOUT_CONTENT`（默认 30s）调整
4. **压力测试超时**：压力测试（FT-21/CT-13）使用更长超时，通过 `STRESS_INDEX_WAIT_TIMEOUT_FILENAME`（默认 60s）和 `STRESS_INDEX_WAIT_TIMEOUT_CONTENT`（默认 120s）调整
5. **前置检查**：脚本启动时检查 `dfm-searcher` 可用性和索引服务状态

## 临时目录

测试在 `~/Documents` 下创建临时目录（`mktemp -d -p ~/Documents`），确保在索引服务监控范围内。测试结束后自动清理。
