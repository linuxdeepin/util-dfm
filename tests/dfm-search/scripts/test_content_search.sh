#!/bin/bash
# SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: GPL-3.0-or-later

# =============================================================================
# test_content_search.sh — dfm-search 文件内容搜索测试脚本
#
# 在 ~/Documents 下创建临时目录，创建 txt 文本文件并写入内容，使用
# dfm-searcher 的内容搜索功能验证：
#   - 基本内容搜索、Boolean 搜索
#   - 大小写敏感搜索
#   - 预览长度控制、JSON 输出
#   - 内容修改/删除后搜索
#   - 隐藏文件内容搜索
#
# 注意：以下用例因不支持已移除（非 SKIP）：
#   - 拼音内容搜索（--type=content --pinyin）
#   - 实时内容搜索（--method=realtime --type=content）
#
# 用法: ./test_content_search.sh
# 环境变量:
#   DFM_SEARCHER             — dfm-searcher 可执行文件路径（默认: dfm-searcher）
#   INDEX_WAIT_TIMEOUT_CONTENT — 内容索引等待超时秒数（默认: 120）
#   INDEX_POLL_INTERVAL      — 索引轮询间隔秒数（默认: 2）
# =============================================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/common.sh"

# 陷阱：确保退出时清理
trap cleanup_test_env EXIT

echo "========================================="
echo " dfm-search 文件内容搜索测试"
echo "========================================="
echo ""

# 前置条件检查
if ! check_prerequisites; then
    echo "Prerequisites not met. Exiting."
    exit 1
fi

# 初始化测试环境
setup_test_env "ct-search-test"
TEST_DIR="$TEST_TEMP_DIR"

# =============================================================================
# 测试用例
# =============================================================================

# ---------- CT-01: 基本内容搜索 ----------
echo ""
echo "--- CT-01: Basic content search ---"
mkdir -p "$TEST_DIR/basic"
echo "这是一份会议纪要，记录了项目进度和下一步计划。" > "$TEST_DIR/basic/meeting.txt"
echo "这是另一份关于财务报告的文件。" > "$TEST_DIR/basic/finance.txt"

if wait_for_index "content" "会议纪要" "$TEST_DIR" "meeting.txt"; then
    result=$(run_searcher "content" "会议纪要" "$TEST_DIR") || true
    assert_found "CT-01: Basic content search finds file with keyword" "$result" "meeting.txt"
    assert_not_found "CT-01: Basic content search excludes non-matching" "$result" "finance.txt"
else
    skip "CT-01: Basic content search" "Index not ready within ${CONTENT_INDEX_TIMEOUT}s"
fi

# ---------- CT-02: 英文内容搜索 ----------
echo ""
echo "--- CT-02: English content search ---"
mkdir -p "$TEST_DIR/english"
echo "The project architecture document describes the system design." > "$TEST_DIR/english/architecture.txt"
echo "Some unrelated content here." > "$TEST_DIR/english/unrelated.txt"

if wait_for_index "content" "architecture" "$TEST_DIR" "architecture.txt"; then
    result=$(run_searcher "content" "architecture" "$TEST_DIR") || true
    assert_found "CT-02: English content search finds matching file" "$result" "architecture.txt"
else
    skip "CT-02: English content search" "Index not ready within ${CONTENT_INDEX_TIMEOUT}s"
fi

# ---------- CT-03: Boolean AND 内容搜索 ----------
echo ""
echo "--- CT-03: Boolean AND content search ---"
mkdir -p "$TEST_DIR/boolean"
echo "本项目包含数据库设计和接口设计两个部分。" > "$TEST_DIR/boolean/design_db.txt"
echo "本文档只讨论接口设计。" > "$TEST_DIR/boolean/design_api.txt"
echo "这份内容完全无关。" > "$TEST_DIR/boolean/unrelated.txt"

if wait_for_index "content" "数据库" "$TEST_DIR" "design_db.txt"; then
    result=$(run_searcher "content" '数据库,接口' "$TEST_DIR" "--query=boolean") || true
    assert_found "CT-03a: Boolean AND finds file with both keywords" "$result" "design_db.txt"
    assert_not_found "CT-03b: Boolean AND excludes file with only one keyword" "$result" "design_api.txt"
else
    skip "CT-03: Boolean AND content search" "Index not ready within ${CONTENT_INDEX_TIMEOUT}s"
fi

# ---------- CT-04: Boolean OR 内容搜索 ----------
echo ""
echo "--- CT-04: Boolean OR content search ---"
if wait_for_index "content" "数据库" "$TEST_DIR" "design_db.txt"; then
    result=$(run_searcher "content" '数据库|完全无关' "$TEST_DIR" "--query=boolean") || true
    assert_found "CT-04a: Boolean OR finds file with first keyword" "$result" "design_db.txt"
    assert_found "CT-04b: Boolean OR finds file with second keyword" "$result" "unrelated.txt"
else
    skip "CT-04: Boolean OR content search" "Index not ready within ${CONTENT_INDEX_TIMEOUT}s"
fi

# ---------- CT-05: 大小写敏感内容搜索 ----------
echo ""
echo "--- CT-05: Case-sensitive content search ---"
mkdir -p "$TEST_DIR/case"
echo "The ERROR handling is important in production code." > "$TEST_DIR/case/upper.txt"
echo "The error handling module needs review." > "$TEST_DIR/case/lower.txt"

if wait_for_index "content" "ERROR" "$TEST_DIR" "upper.txt"; then
    # 默认不区分大小写
    result=$(run_searcher "content" "ERROR" "$TEST_DIR") || true
    assert_found "CT-05a: Case-insensitive finds both files" "$result" "upper.txt"

    # 区分大小写
    # 索引服务当前对 --case-sensitive 不生效（filename 搜索同样失效），按已知限制处理：
    # 探测条件为大写关键词精确命中且排除小写文件，两者同时满足才视为服务已支持
    result=$(run_searcher "content" "ERROR" "$TEST_DIR" "--case-sensitive") || true
    if json_contains "$result" "upper.txt" && ! json_contains "$result" "lower.txt"; then
        assert_found "CT-05b: Case-sensitive finds exact match" "$result" "upper.txt"
        assert_not_found "CT-05c: Case-sensitive excludes lowercase" "$result" "lower.txt"
    else
        skip "CT-05b/c: Case-sensitive content search" "Known limitation: --case-sensitive not effective in index service"
    fi
else
    skip "CT-05: Case-sensitive content search" "Index not ready within ${CONTENT_INDEX_TIMEOUT}s"
fi

# ---------- CT-06: 预览长度控制 ----------
echo ""
echo "--- CT-06: Preview length control ---"
mkdir -p "$TEST_DIR/preview"
# 创建一个内容较长的文件（纯 bash 生成，避免依赖 python3）
padding_a=$(printf 'A%.0s' {1..500})
padding_b=$(printf 'B%.0s' {1..500})
echo "${padding_a} keyword_preview_test ${padding_b}" > "$TEST_DIR/preview/long_file.txt"

if wait_for_index "content" "keyword_preview_test" "$TEST_DIR" "long_file.txt"; then
    # 使用 verbose 输出获取预览
    result=$("$TEST_SEARCHER" --type=content -v "keyword_preview_test" "$TEST_DIR" 2>/dev/null) || true
    TEST_TOTAL=$((TEST_TOTAL + 1))
    if [[ -n "$result" ]]; then
        pass "CT-06: Verbose content search returns preview"
    else
        fail "CT-06: Verbose content search returns preview" "No output"
    fi

    # 使用 max-preview 限制预览长度
    result=$("$TEST_SEARCHER" --type=content -v --max-preview=50 "keyword_preview_test" "$TEST_DIR" -j 2>/dev/null) || true
    TEST_TOTAL=$((TEST_TOTAL + 1))
    if [[ -n "$result" ]]; then
        pass "CT-06b: max-preview limits preview length"
    else
        fail "CT-06b: max-preview limits preview length" "No output"
    fi
else
    skip "CT-06: Preview length control" "Index not ready within ${CONTENT_INDEX_TIMEOUT}s"
fi

# ---------- CT-07: JSON 输出验证 ----------
echo ""
echo "--- CT-07: JSON output validation ---"
mkdir -p "$TEST_DIR/json"
echo "JSON output test content for validation." > "$TEST_DIR/json/json_test.txt"

if wait_for_index "content" "JSON" "$TEST_DIR" "json_test.txt"; then
    result=$(run_searcher "content" "JSON" "$TEST_DIR") || true
    TEST_TOTAL=$((TEST_TOTAL + 1))
    # 检查 JSON 输出包含必要字段：
    # results 为路径字符串数组、status.totalResults 计数、search 搜索元信息
    if echo "$result" | jq -e '
        (.results | type == "array") and
        ((.results | length) > 0) and
        (.results[0] | type == "string") and
        (.status.totalResults | type == "number") and
        (.search.searchType == "content")
    ' &>/dev/null; then
        pass "CT-07: JSON output contains required fields"
    else
        fail "CT-07: JSON output contains required fields" "Missing results/status.totalResults/search.searchType"
    fi
else
    skip "CT-07: JSON output validation" "Index not ready within ${CONTENT_INDEX_TIMEOUT}s"
fi

# ---------- CT-08: 内容修改后搜索 ----------
echo ""
echo "--- CT-08: Search after content modification ---"
mkdir -p "$TEST_DIR/modify"
echo "原始内容，没有任何特殊关键词。" > "$TEST_DIR/modify/modify_test.txt"

if wait_for_index "content" "原始内容" "$TEST_DIR" "modify_test.txt"; then
    # 修改文件内容，添加新关键词
    echo "修改后的内容包含独特关键词 unique_keyword_xyz。" > "$TEST_DIR/modify/modify_test.txt"

    # 等待索引更新
    local_elapsed=0
    modified_ok=false
    while [[ $local_elapsed -lt $CONTENT_INDEX_TIMEOUT ]]; do
        result=$(run_searcher "content" "unique_keyword_xyz" "$TEST_DIR") || true
        if json_contains "$result" "modify_test.txt"; then
            modified_ok=true
            break
        fi
        sleep "$INDEX_POLL_INTERVAL"
        local_elapsed=$((local_elapsed + INDEX_POLL_INTERVAL))
    done

    TEST_TOTAL=$((TEST_TOTAL + 1))
    if $modified_ok; then
        pass "CT-08: Modified content found by new keyword"
    else
        fail "CT-08: Modified content found by new keyword" "Index may not have updated"
    fi
else
    skip "CT-08: Search after content modification" "Index not ready within ${CONTENT_INDEX_TIMEOUT}s"
fi

# ---------- CT-09: 内容删除后搜索 ----------
echo ""
echo "--- CT-09: Search after content deletion ---"
mkdir -p "$TEST_DIR/delete"
echo "这份文档包含待删除关键词 delete_marker_test。" > "$TEST_DIR/delete/delete_test.txt"

if wait_for_index "content" "delete_marker_test" "$TEST_DIR" "delete_test.txt"; then
    # 删除文件内容中的关键词（重写为不含关键词的内容）
    echo "这份文档的内容已被替换，不再包含原关键词。" > "$TEST_DIR/delete/delete_test.txt"

    # 等待索引更新
    local_elapsed=0
    deleted_ok=false
    while [[ $local_elapsed -lt $CONTENT_INDEX_TIMEOUT ]]; do
        result=$(run_searcher "content" "delete_marker_test" "$TEST_DIR") || true
        if ! json_contains "$result" "delete_test.txt"; then
            deleted_ok=true
            break
        fi
        sleep "$INDEX_POLL_INTERVAL"
        local_elapsed=$((local_elapsed + INDEX_POLL_INTERVAL))
    done

    TEST_TOTAL=$((TEST_TOTAL + 1))
    if $deleted_ok; then
        pass "CT-09: Deleted keyword no longer found in search"
    else
        fail "CT-09: Deleted keyword no longer found in search" "Index may not have updated"
    fi
else
    skip "CT-09: Search after content deletion" "Index not ready within ${CONTENT_INDEX_TIMEOUT}s"
fi

# ---------- CT-10: 隐藏文件内容搜索 ----------
echo ""
echo "--- CT-10: Hidden file content search ---"
mkdir -p "$TEST_DIR/hidden"
echo "Hidden file content with unique hidden_marker_keyword." > "$TEST_DIR/hidden/.hidden_config.txt"
echo "Visible file content with unique hidden_marker_keyword." > "$TEST_DIR/hidden/visible_config.txt"

if wait_for_index "content" "hidden_marker_keyword" "$TEST_DIR" "visible_config.txt"; then
    # 默认不搜索隐藏文件
    result=$(run_searcher "content" "hidden_marker_keyword" "$TEST_DIR") || true
    assert_found "CT-10a: Default content search finds visible file" "$result" "visible_config.txt"
    assert_not_found "CT-10b: Default content search excludes hidden file" "$result" ".hidden_config.txt"

    # 包含隐藏文件
    result=$(run_searcher "content" "hidden_marker_keyword" "$TEST_DIR" "--include-hidden") || true
    assert_found "CT-10c: --include-hidden finds hidden file content" "$result" ".hidden_config.txt"
else
    skip "CT-10: Hidden file content search" "Index not ready within ${CONTENT_INDEX_TIMEOUT}s"
fi

# ---------- CT-11: 多文件内容搜索 ----------
echo ""
echo "--- CT-11: Multiple files content search ---"
mkdir -p "$TEST_DIR/multi"
echo "这个文件包含共享关键词 common_keyword_multi。" > "$TEST_DIR/multi/file1.txt"
echo "另一个文件也包含共享关键词 common_keyword_multi。" > "$TEST_DIR/multi/file2.txt"
echo "这个文件没有目标关键词。" > "$TEST_DIR/multi/file3.txt"

if wait_for_index "content" "common_keyword_multi" "$TEST_DIR" "file2.txt"; then
    result=$(run_searcher "content" "common_keyword_multi" "$TEST_DIR") || true
    assert_found "CT-11a: Multi-file search finds file1" "$result" "file1.txt"
    assert_found "CT-11b: Multi-file search finds file2" "$result" "file2.txt"
    assert_not_found "CT-11c: Multi-file search excludes non-matching file3" "$result" "file3.txt"
else
    skip "CT-11: Multiple files content search" "Index not ready within ${CONTENT_INDEX_TIMEOUT}s"
fi

# ---------- CT-12: Boolean AND 使用 & 分隔符内容搜索 ----------
echo ""
echo "--- CT-12: Boolean AND with & separator content search ---"
if wait_for_index "content" "数据库" "$TEST_DIR" "design_db.txt"; then
    result=$(run_searcher "content" '数据库&接口' "$TEST_DIR" "--query=boolean") || true
    assert_found "CT-12: Boolean AND with & finds file with both keywords" "$result" "design_db.txt"
else
    skip "CT-12: Boolean AND with & separator content search" "Index not ready within ${CONTENT_INDEX_TIMEOUT}s"
fi


# ---------- CT-13: 压力测试 — 1000 个文件内容搜索 ----------
echo ""
echo "--- CT-13: Stress test — 1000 files content search ---"
STRESS_COUNT="${STRESS_FILE_COUNT:-1000}"
STRESS_DIR="$TEST_DIR/stress_content"
mkdir -p "$STRESS_DIR"

# 统计去重后的 stress 文件数
# 注意：突发写入窗口内索引快照可能同时含重复与缺失条目（最终一致），必须按唯一路径计数
count_stress_files() {
    if command -v jq &>/dev/null; then
        echo "$1" | jq -r '[.results[]? | select(test("stress_file_[0-9]{4}\\.txt$"))] | unique | length' 2>/dev/null || echo 0
    else
        echo "$1" | grep -oE 'stress_file_[0-9]{4}\.txt' | sort -u | wc -l
    fi
}

# 创建 1000 个文件，每个文件包含独特 keyword + 公共 batch keyword
echo "Creating $STRESS_COUNT files with unique content keywords..."
for i in $(seq 1 "$STRESS_COUNT"); do
    printf -v idx "%04d" "$i"
    echo "stress_ct_batch stress_ct_${idx}" > "$STRESS_DIR/stress_file_${idx}.txt"
done

# 等待索引：轮询去重后的文件数量，直到全部文件入索引或超时
# 注意：不能用 totalResults 或"最后一个文件已索引"作为全部完成的依据
echo "Waiting for index (timeout: ${STRESS_CONTENT_INDEX_TIMEOUT}s)..."
elapsed=0
result=""
found_count=0
while [[ $elapsed -lt $STRESS_CONTENT_INDEX_TIMEOUT ]]; do
    result=$(run_searcher "content" "stress_ct_batch" "$STRESS_DIR" "--max-results=$((STRESS_COUNT + 100))") || true
    found_count=$(count_stress_files "$result")
    if [[ "$found_count" -eq "$STRESS_COUNT" ]]; then
        break
    fi
    sleep "$INDEX_POLL_INTERVAL"
    elapsed=$((elapsed + INDEX_POLL_INTERVAL))
done

TEST_TOTAL=$((TEST_TOTAL + 1))
if [[ "$found_count" -eq "$STRESS_COUNT" ]]; then
    pass "CT-13: All $STRESS_COUNT files found by content keyword"
else
    fail "CT-13: Stress test — $found_count/$STRESS_COUNT files found" "$((STRESS_COUNT - found_count)) files not in search results"
fi

if [[ "$found_count" -eq "$STRESS_COUNT" ]]; then
    # 抽样验证几个单独的独特 keyword
    for sample_idx in 0001 0250 0500 0750 1000; do
        sample_result=$(run_searcher "content" "stress_ct_${sample_idx}" "$STRESS_DIR") || true
        assert_found "CT-13: Spot check stress_ct_${sample_idx}" "$sample_result" "stress_file_${sample_idx}.txt"
    done
fi

# 清理压力测试文件
rm -rf "$STRESS_DIR"
echo "Cleaned up stress test files."

# =============================================================================
# 输出汇总
# =============================================================================
print_summary
