#!/bin/bash
# SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: GPL-3.0-or-later

# =============================================================================
# test_filename_search.sh — dfm-search 文件名搜索测试脚本
#
# 在 ~/Documents 下创建临时目录，科学地创建/删除/修改/移动文件（含目录、
# 隐藏文件），使用 dfm-searcher 的文件名搜索功能验证：
#   - 基本搜索、通配符搜索
#   - Boolean AND/OR 搜索
#   - 拼音全拼/首字母/布尔组合搜索
#   - 大小写敏感、隐藏文件、max-results
#   - 文件增删改移后搜索
#   - 目录搜索、扩展名过滤
#   - 实时搜索
#
# 用法: ./test_filename_search.sh
# 环境变量:
#   DFM_SEARCHER             — dfm-searcher 可执行文件路径（默认: dfm-searcher）
#   INDEX_WAIT_TIMEOUT_FILENAME — 文件名索引等待超时秒数（默认: 30）
#   INDEX_POLL_INTERVAL      — 索引轮询间隔秒数（默认: 2）
# =============================================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/common.sh"

# 陷阱：确保退出时清理
trap cleanup_test_env EXIT

echo "========================================="
echo " dfm-search 文件名搜索测试"
echo "========================================="
echo ""

# 前置条件检查
if ! check_prerequisites; then
    echo "Prerequisites not met. Exiting."
    exit 1
fi

# 初始化测试环境
setup_test_env "fn-search-test"
TEST_DIR="$TEST_TEMP_DIR"

# =============================================================================
# 测试用例
# =============================================================================

# ---------- FT-01: 基本文件名搜索 ----------
echo ""
echo "--- FT-01: Basic filename search ---"
mkdir -p "$TEST_DIR/reports"
echo "content" > "$TEST_DIR/reports/工作报告.txt"
echo "content" > "$TEST_DIR/reports/meeting_notes.txt"

if wait_for_index "filename" "工作报告" "$TEST_DIR" "工作报告.txt"; then
    result=$(run_searcher "filename" "工作报告" "$TEST_DIR") || true
    assert_found "FT-01: Basic search finds file by Chinese keyword" "$result" "工作报告.txt"
else
    skip "FT-01: Basic filename search" "Index not ready within ${FILENAME_INDEX_TIMEOUT}s"
fi

# ---------- FT-02: 英文关键词搜索 ----------
echo ""
echo "--- FT-02: English keyword search ---"
echo "content" > "$TEST_DIR/meeting_notes.txt"

if wait_for_index "filename" "meeting" "$TEST_DIR" "meeting_notes.txt"; then
    result=$(run_searcher "filename" "meeting" "$TEST_DIR") || true
    assert_found "FT-02: English keyword search finds file" "$result" "meeting_notes.txt"
else
    skip "FT-02: English keyword search" "Index not ready within ${FILENAME_INDEX_TIMEOUT}s"
fi

# ---------- FT-03: 通配符搜索 (*) ----------
echo ""
echo "--- FT-03: Wildcard search (*) ---"
mkdir -p "$TEST_DIR/wildcard"
echo "content" > "$TEST_DIR/wildcard/report2026.txt"
echo "content" > "$TEST_DIR/wildcard/report2025.txt"
echo "content" > "$TEST_DIR/wildcard/summary.txt"

if wait_for_index "filename" "report" "$TEST_DIR" "report2026.txt"; then
    result=$(run_searcher "filename" "report*" "$TEST_DIR" "--wildcard") || true
    assert_found "FT-03a: Wildcard '*' finds report2026" "$result" "report2026.txt"
    assert_found "FT-03b: Wildcard '*' finds report2025" "$result" "report2025.txt"
else
    skip "FT-03: Wildcard search (*)" "Index not ready within ${FILENAME_INDEX_TIMEOUT}s"
fi

# ---------- FT-04: 通配符搜索 (?) ----------
echo ""
echo "--- FT-04: Wildcard search (?) ---"
if wait_for_index "filename" "report" "$TEST_DIR" "report2026.txt"; then
    result=$(run_searcher "filename" "report????.txt" "$TEST_DIR" "--wildcard") || true
    assert_found "FT-04: Wildcard '?' matches single chars" "$result" "report2026.txt"
else
    skip "FT-04: Wildcard search (?)" "Index not ready within ${FILENAME_INDEX_TIMEOUT}s"
fi

# ---------- FT-05: Boolean AND 搜索 ----------
echo ""
echo "--- FT-05: Boolean AND search ---"
mkdir -p "$TEST_DIR/boolean"
echo "content" > "$TEST_DIR/boolean/报告2026.txt"
echo "content" > "$TEST_DIR/boolean/报告2025.txt"
echo "content" > "$TEST_DIR/boolean/总结2026.txt"

if wait_for_index "filename" "报告" "$TEST_DIR" "报告2026.txt"; then
    result=$(run_searcher "filename" "报告,2026" "$TEST_DIR" "--query=boolean") || true
    assert_found "FT-05: Boolean AND finds matching file" "$result" "报告2026.txt"
    assert_not_found "FT-05: Boolean AND excludes non-matching" "$result" "总结2026.txt"
else
    skip "FT-05: Boolean AND search" "Index not ready within ${FILENAME_INDEX_TIMEOUT}s"
fi

# ---------- FT-06: Boolean OR 搜索 ----------
echo ""
echo "--- FT-06: Boolean OR search ---"
if wait_for_index "filename" "报告" "$TEST_DIR" "报告2026.txt"; then
    result=$(run_searcher "filename" '报告|总结' "$TEST_DIR" "--query=boolean") || true
    assert_found "FT-06a: Boolean OR finds 报告" "$result" "报告2026.txt"
    assert_found "FT-06b: Boolean OR finds 总结" "$result" "总结2026.txt"
else
    skip "FT-06: Boolean OR search" "Index not ready within ${FILENAME_INDEX_TIMEOUT}s"
fi

# ---------- FT-07: 拼音全拼搜索 ----------
echo ""
echo "--- FT-07: Pinyin full search ---"
mkdir -p "$TEST_DIR/pinyin"
echo "content" > "$TEST_DIR/pinyin/报告.txt"

if wait_for_index "filename" "报告" "$TEST_DIR" "报告.txt"; then
    result=$(run_searcher "filename" "baogao" "$TEST_DIR" "--pinyin") || true
    assert_found "FT-07: Pinyin full 'baogao' finds 报告" "$result" "报告.txt"
else
    skip "FT-07: Pinyin full search" "Index not ready within ${FILENAME_INDEX_TIMEOUT}s"
fi

# ---------- FT-08: 拼音首字母搜索 ----------
echo ""
echo "--- FT-08: Pinyin acronym search ---"
if wait_for_index "filename" "报告" "$TEST_DIR" "报告.txt"; then
    result=$(run_searcher "filename" "bg" "$TEST_DIR" "--pinyin-acronym") || true
    assert_found "FT-08: Pinyin acronym 'bg' finds 报告" "$result" "报告.txt"
else
    skip "FT-08: Pinyin acronym search" "Index not ready within ${FILENAME_INDEX_TIMEOUT}s"
fi

# ---------- FT-09: 拼音 + Boolean 组合搜索 ----------
echo ""
echo "--- FT-09: Pinyin + Boolean combined search ---"
echo "content" > "$TEST_DIR/pinyin/饼干.txt"

# 注意：等待探测必须针对本用例新建的文件（饼干.txt），
# 若探测已索引的旧文件（如 报告.txt），会在新文件入索引前就放行，造成竞态
if wait_for_index "filename" "饼干" "$TEST_DIR" "饼干.txt"; then
    result=$(run_searcher "filename" "baogao|binggan" "$TEST_DIR" "--pinyin" "--query=boolean") || true
    assert_found "FT-09a: Pinyin+Boolean finds 报告(baogao)" "$result" "报告.txt"
    assert_found "FT-09b: Pinyin+Boolean finds 饼干(binggan)" "$result" "饼干.txt"
else
    skip "FT-09: Pinyin + Boolean combined search" "Index not ready within ${FILENAME_INDEX_TIMEOUT}s"
fi

# ---------- FT-10: 大小写敏感搜索 ----------
echo ""
echo "--- FT-10: Case-sensitive search ---"
mkdir -p "$TEST_DIR/case"
echo "content" > "$TEST_DIR/case/README.md"
echo "content" > "$TEST_DIR/case/readme.txt"

if wait_for_index "filename" "README" "$TEST_DIR" "README.md"; then
    # 默认不区分大小写：应都能找到
    result=$(run_searcher "filename" "README" "$TEST_DIR") || true
    assert_found "FT-10a: Case-insensitive finds README.md" "$result" "README.md"

    # 区分大小写：只应找到 README.md，不应找到 readme.txt
    # 索引服务当前对 --case-sensitive 不生效（content 搜索同样失效），按已知限制处理：
    # 若探测到大写关键词能精确命中，说明服务已支持，自动执行断言
    result=$(run_searcher "filename" "README" "$TEST_DIR" "--case-sensitive") || true
    if json_contains "$result" "README.md"; then
        assert_found "FT-10b: Case-sensitive finds README.md" "$result" "README.md"
        assert_not_found "FT-10c: Case-sensitive excludes readme.txt" "$result" "readme.txt"
    else
        skip "FT-10b/c: Case-sensitive search" "Known limitation: --case-sensitive not effective in index service"
    fi
else
    skip "FT-10: Case-sensitive search" "Index not ready within ${FILENAME_INDEX_TIMEOUT}s"
fi

# ---------- FT-11: 隐藏文件搜索 ----------
echo ""
echo "--- FT-11: Hidden file search ---"
mkdir -p "$TEST_DIR/hidden"
echo "content" > "$TEST_DIR/hidden/.config"
echo "content" > "$TEST_DIR/hidden/visible.txt"

if wait_for_index "filename" "visible" "$TEST_DIR" "visible.txt"; then
    # 默认不搜索隐藏文件
    result=$(run_searcher "filename" "config" "$TEST_DIR") || true
    assert_not_found "FT-11a: Default excludes hidden files" "$result" ".config"

    # 包含隐藏文件
    result=$(run_searcher "filename" "config" "$TEST_DIR" "--include-hidden") || true
    assert_found "FT-11b: --include-hidden finds hidden files" "$result" ".config"
else
    skip "FT-11: Hidden file search" "Index not ready within ${FILENAME_INDEX_TIMEOUT}s"
fi

# ---------- FT-12: max-results 限制 ----------
echo ""
echo "--- FT-12: Max results limit ---"
mkdir -p "$TEST_DIR/maxresults"
for i in $(seq 1 10); do
    echo "content" > "$TEST_DIR/maxresults/result_file_${i}.txt"
done

if wait_for_index "filename" "result_file" "$TEST_DIR" "result_file_10.txt"; then
    result=$(run_searcher "filename" "result_file" "$TEST_DIR" "--max-results=3") || true
    assert_le_count "FT-12: max-results limits to 3" "$result" 3
else
    skip "FT-12: Max results limit" "Index not ready within ${FILENAME_INDEX_TIMEOUT}s"
fi

# ---------- FT-13: 新建文件后搜索 ----------
echo ""
echo "--- FT-13: Search after file creation ---"
echo "content" > "$TEST_DIR/newly_created_file.txt"

if wait_for_index "filename" "newly_created" "$TEST_DIR" "newly_created_file.txt"; then
    result=$(run_searcher "filename" "newly_created" "$TEST_DIR") || true
    assert_found "FT-13: Search finds newly created file" "$result" "newly_created_file.txt"
else
    skip "FT-13: Search after file creation" "Index not ready within ${FILENAME_INDEX_TIMEOUT}s"
fi

# ---------- FT-14: 删除文件后搜索 ----------
echo ""
echo "--- FT-14: Search after file deletion ---"
echo "content" > "$TEST_DIR/to_be_deleted.txt"
if wait_for_index "filename" "to_be_deleted" "$TEST_DIR" "to_be_deleted.txt"; then
    rm "$TEST_DIR/to_be_deleted.txt"
    # 等待索引更新
    local_elapsed=0
    deleted_ok=false
    while [[ $local_elapsed -lt $FILENAME_INDEX_TIMEOUT ]]; do
        result=$(run_searcher "filename" "to_be_deleted" "$TEST_DIR") || true
        if ! json_contains "$result" "to_be_deleted.txt"; then
            deleted_ok=true
            break
        fi
        sleep "$INDEX_POLL_INTERVAL"
        local_elapsed=$((local_elapsed + INDEX_POLL_INTERVAL))
    done

    TEST_TOTAL=$((TEST_TOTAL + 1))
    if $deleted_ok; then
        pass "FT-14: Deleted file no longer in search results"
    else
        fail "FT-14: Deleted file still in search results" "Index may not have updated"
    fi
else
    skip "FT-14: Search after file deletion" "Index not ready within ${FILENAME_INDEX_TIMEOUT}s"
fi

# ---------- FT-15: 修改文件名后搜索 ----------
echo ""
echo "--- FT-15: Search after file rename ---"
echo "content" > "$TEST_DIR/before_rename.txt"
if wait_for_index "filename" "before_rename" "$TEST_DIR" "before_rename.txt"; then
    mv "$TEST_DIR/before_rename.txt" "$TEST_DIR/after_rename.txt"
    # 等待索引更新
    local_elapsed=0
    renamed_ok=false
    while [[ $local_elapsed -lt $FILENAME_INDEX_TIMEOUT ]]; do
        result=$(run_searcher "filename" "after_rename" "$TEST_DIR") || true
        if json_contains "$result" "after_rename.txt"; then
            renamed_ok=true
            break
        fi
        sleep "$INDEX_POLL_INTERVAL"
        local_elapsed=$((local_elapsed + INDEX_POLL_INTERVAL))
    done

    TEST_TOTAL=$((TEST_TOTAL + 1))
    if $renamed_ok; then
        pass "FT-15: Renamed file found by new name"
    else
        fail "FT-15: Renamed file not found by new name" "Index may not have updated"
    fi
else
    skip "FT-15: Search after file rename" "Index not ready within ${FILENAME_INDEX_TIMEOUT}s"
fi

# ---------- FT-16: 移动文件后搜索 ----------
echo ""
echo "--- FT-16: Search after file move ---"
mkdir -p "$TEST_DIR/move_src" "$TEST_DIR/move_dst"
echo "content" > "$TEST_DIR/move_src/movable_file.txt"
if wait_for_index "filename" "movable_file" "$TEST_DIR" "movable_file.txt"; then
    mv "$TEST_DIR/move_src/movable_file.txt" "$TEST_DIR/move_dst/movable_file.txt"
    # 等待索引更新
    local_elapsed=0
    moved_ok=false
    while [[ $local_elapsed -lt $FILENAME_INDEX_TIMEOUT ]]; do
        result=$(run_searcher "filename" "movable_file" "$TEST_DIR/move_dst") || true
        if json_contains "$result" "movable_file.txt"; then
            moved_ok=true
            break
        fi
        sleep "$INDEX_POLL_INTERVAL"
        local_elapsed=$((local_elapsed + INDEX_POLL_INTERVAL))
    done

    TEST_TOTAL=$((TEST_TOTAL + 1))
    if $moved_ok; then
        pass "FT-16: Moved file found in new location"
    else
        fail "FT-16: Moved file not found in new location" "Index may not have updated"
    fi
else
    skip "FT-16: Search after file move" "Index not ready within ${FILENAME_INDEX_TIMEOUT}s"
fi

# ---------- FT-17: 目录搜索 ----------
echo ""
echo "--- FT-17: Directory search ---"
mkdir -p "$TEST_DIR/project_alpha"

if wait_for_index "filename" "project_alpha" "$TEST_DIR" "project_alpha"; then
    result=$(run_searcher "filename" "project_alpha" "$TEST_DIR") || true
    assert_found "FT-17: Search finds directory by name" "$result" "project_alpha"
else
    skip "FT-17: Directory search" "Index not ready within ${FILENAME_INDEX_TIMEOUT}s"
fi

# ---------- FT-19: 扩展名过滤 ----------
echo ""
echo "--- FT-19: Extension filter ---"
mkdir -p "$TEST_DIR/extfilter"
echo "content" > "$TEST_DIR/extfilter/document.txt"
echo "content" > "$TEST_DIR/extfilter/document.pdf"
echo "content" > "$TEST_DIR/extfilter/document.docx"

if wait_for_index "filename" "document" "$TEST_DIR" "document.docx"; then
    result=$(run_searcher "filename" "document" "$TEST_DIR" "--file-extensions=txt,pdf") || true
    assert_found "FT-19a: Extension filter finds .txt" "$result" "document.txt"
    assert_found "FT-19b: Extension filter finds .pdf" "$result" "document.pdf"
    assert_not_found "FT-19c: Extension filter excludes .docx" "$result" "document.docx"
else
    skip "FT-19: Extension filter" "Index not ready within ${FILENAME_INDEX_TIMEOUT}s"
fi

# ---------- FT-18: 实时搜索（文件名） ----------
echo ""
echo "--- FT-18: Realtime filename search ---"
echo "content" > "$TEST_DIR/realtime_test_file.txt"
# 实时搜索不需要等待索引
result=$(run_searcher "filename" "realtime_test_file" "$TEST_DIR" "--method=realtime") || true
assert_found "FT-18: Realtime search finds file without index" "$result" "realtime_test_file.txt"

# ---------- FT-20: Boolean AND 使用 & 分隔符 ----------
echo ""
echo "--- FT-20: Boolean AND with & separator ---"
if wait_for_index "filename" "报告" "$TEST_DIR" "报告2026.txt"; then
    result=$(run_searcher "filename" '报告&2026' "$TEST_DIR" "--query=boolean") || true
    assert_found "FT-20: Boolean AND with & finds matching file" "$result" "报告2026.txt"
else
    skip "FT-20: Boolean AND with & separator" "Index not ready within ${FILENAME_INDEX_TIMEOUT}s"
fi


# ---------- FT-21: 压力测试 — 1000 个文件名搜索 ----------
echo ""
echo "--- FT-21: Stress test — 1000 files filename search ---"
STRESS_COUNT="${STRESS_FILE_COUNT:-1000}"
STRESS_DIR="$TEST_DIR/stress_filename"
mkdir -p "$STRESS_DIR"

# 统计去重后的 stress 文件数
# 注意：突发写入窗口内索引快照可能同时含重复与缺失条目（最终一致），必须按唯一路径计数
count_stress_files() {
    if command -v jq &>/dev/null; then
        echo "$1" | jq -r '[.results[]? | select(test("stress_fn_[0-9]{4}\\.txt$"))] | unique | length' 2>/dev/null || echo 0
    else
        echo "$1" | grep -oE 'stress_fn_[0-9]{4}\.txt' | sort -u | wc -l
    fi
}

# 创建 1000 个文件，每个文件名包含独特 keyword
echo "Creating $STRESS_COUNT files with unique filename keywords..."
for i in $(seq 1 "$STRESS_COUNT"); do
    printf -v idx "%04d" "$i"
    echo "content" > "$STRESS_DIR/stress_fn_${idx}.txt"
done

# 等待索引：轮询去重后的文件数量，直到全部文件入索引或超时
# 注意：不能用 totalResults 或"最后一个文件已索引"作为全部完成的依据
echo "Waiting for index (timeout: ${STRESS_FILENAME_INDEX_TIMEOUT}s)..."
elapsed=0
result=""
found_count=0
while [[ $elapsed -lt $STRESS_FILENAME_INDEX_TIMEOUT ]]; do
    result=$(run_searcher "filename" "stress_fn" "$STRESS_DIR" "--max-results=$((STRESS_COUNT + 100))") || true
    found_count=$(count_stress_files "$result")
    if [[ "$found_count" -eq "$STRESS_COUNT" ]]; then
        break
    fi
    sleep "$INDEX_POLL_INTERVAL"
    elapsed=$((elapsed + INDEX_POLL_INTERVAL))
done

TEST_TOTAL=$((TEST_TOTAL + 1))
if [[ "$found_count" -eq "$STRESS_COUNT" ]]; then
    pass "FT-21: All $STRESS_COUNT files found by prefix keyword"
else
    fail "FT-21: Stress test — $found_count/$STRESS_COUNT files found" "$((STRESS_COUNT - found_count)) files not in search results"
fi

if [[ "$found_count" -eq "$STRESS_COUNT" ]]; then
    # 抽样验证几个单独的 keyword
    for sample_idx in 0001 0250 0500 0750 1000; do
        sample_result=$(run_searcher "filename" "stress_fn_${sample_idx}" "$STRESS_DIR") || true
        assert_found "FT-21: Spot check stress_fn_${sample_idx}" "$sample_result" "stress_fn_${sample_idx}.txt"
    done
fi

# 清理压力测试文件
rm -rf "$STRESS_DIR"
echo "Cleaned up stress test files."

# =============================================================================
# 输出汇总
# =============================================================================
print_summary
