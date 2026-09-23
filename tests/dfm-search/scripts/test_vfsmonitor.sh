#!/bin/bash
# SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: GPL-3.0-or-later

# =============================================================================
# test_vfsmonitor.sh — vfsmonitor 文件监视器有效性测试脚本
#
# 通过文件系统操作（创建/删除/重命名/移动/修改）后用 dfm-searcher 搜索，
# 验证索引是否随文件变化实时更新，从而间接验证 vfsmonitor 文件监视器有效性。
#
# 与现有 FT/CT 系列的核心区别：
#   - 测试目标：监视器是否驱动索引实时更新（非搜索功能本身）
#   - 验证方向：双向（旧状态消失 AND 新状态出现）
#   - 场景覆盖：文件/目录的创建、删除、重命名、移动、符号链接、
#     内容新增/修改/清空、批量创建、重复写入、大文件
#
# 用法: ./test_vfsmonitor.sh
# 环境变量:
#   DFM_SEARCHER               — dfm-searcher 可执行文件路径（默认: dfm-searcher）
#   INDEX_WAIT_TIMEOUT_FILENAME — 文件名索引等待超时秒数（默认: 10）
#   INDEX_WAIT_TIMEOUT_CONTENT  — 内容索引等待超时秒数（默认: 30）
#   INDEX_POLL_INTERVAL         — 索引轮询间隔秒数（默认: 2）
#   VM_BATCH_COUNT              — VM-13 批量测试文件数（默认: 100）
# =============================================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/common.sh"

# 陷阱：确保退出时清理
trap cleanup_test_env EXIT

# 批量测试文件数（VM-13）
VM_BATCH_COUNT="${VM_BATCH_COUNT:-100}"

# VM-13 批量测试超时：按 base * max(1, VM_BATCH_COUNT / 50) 比例放大
VM_BATCH_TIMEOUT=$(( FILENAME_INDEX_TIMEOUT * (VM_BATCH_COUNT / 50 > 0 ? VM_BATCH_COUNT / 50 : 1) ))

echo "========================================="
echo " vfsmonitor 文件监视器有效性测试"
echo "========================================="
echo " VM_BATCH_COUNT = $VM_BATCH_COUNT"
echo " VM_BATCH_TIMEOUT = ${VM_BATCH_TIMEOUT}s"
echo ""

# 前置条件检查
if ! check_prerequisites; then
    echo "Prerequisites not met. Exiting."
    exit 1
fi

# 初始化测试环境
setup_test_env "vm-test"
TEST_DIR="$TEST_TEMP_DIR"

# =============================================================================
# 测试用例 VM-01 ~ VM-15
# =============================================================================

# ---------- VM-01: 文件创建后索引 ----------
echo ""
echo "--- VM-01: File creation triggers index update ---"
echo "content" > "$TEST_DIR/vm_created_file.txt"

if wait_for_index "filename" "vm_created_file" "$TEST_DIR" "vm_created_file.txt"; then
    result=$(run_searcher "filename" "vm_created_file" "$TEST_DIR") || true
    assert_found "VM-01: Newly created file found in index" "$result" "vm_created_file.txt"
else
    fail "VM-01: Newly created file found in index" "Index not ready within ${FILENAME_INDEX_TIMEOUT}s"
fi

# ---------- VM-02: 文件删除后索引 ----------
echo ""
echo "--- VM-02: File deletion triggers index update ---"
echo "content" > "$TEST_DIR/vm_to_delete.txt"
if wait_for_index "filename" "vm_to_delete" "$TEST_DIR" "vm_to_delete.txt"; then
    rm "$TEST_DIR/vm_to_delete.txt"
    # 轮询等待索引更新：旧文件名应从搜索结果消失
    local_elapsed=0
    deleted_ok=false
    while [[ $local_elapsed -lt $FILENAME_INDEX_TIMEOUT ]]; do
        result=$(run_searcher "filename" "vm_to_delete" "$TEST_DIR") || true
        if ! json_contains "$result" "vm_to_delete.txt"; then
            deleted_ok=true
            break
        fi
        sleep "$INDEX_POLL_INTERVAL"
        local_elapsed=$((local_elapsed + INDEX_POLL_INTERVAL))
    done

    TEST_TOTAL=$((TEST_TOTAL + 1))
    if $deleted_ok; then
        pass "VM-02: Deleted file no longer in index"
    else
        fail "VM-02: Deleted file no longer in index" "Index not updated after deletion"
    fi
else
    fail "VM-02: Deleted file no longer in index" "Initial index not ready within ${FILENAME_INDEX_TIMEOUT}s"
fi

# ---------- VM-03: 文件重命名后索引 ----------
echo ""
echo "--- VM-03: File rename triggers index update ---"
echo "content" > "$TEST_DIR/vm_old_name.txt"
if wait_for_index "filename" "vm_old_name" "$TEST_DIR" "vm_old_name.txt"; then
    mv "$TEST_DIR/vm_old_name.txt" "$TEST_DIR/vm_new_name.txt"
    # 双向轮询：新名称出现 AND 旧名称消失
    local_elapsed=0
    new_ok=false
    old_gone=false
    while [[ $local_elapsed -lt $FILENAME_INDEX_TIMEOUT ]]; do
        result=$(run_searcher "filename" "vm_new_name" "$TEST_DIR") || true
        json_contains "$result" "vm_new_name.txt" && new_ok=true
        old_result=$(run_searcher "filename" "vm_old_name" "$TEST_DIR") || true
        json_contains "$old_result" "vm_old_name.txt" || old_gone=true
        $new_ok && $old_gone && break
        sleep "$INDEX_POLL_INTERVAL"
        local_elapsed=$((local_elapsed + INDEX_POLL_INTERVAL))
    done

    TEST_TOTAL=$((TEST_TOTAL + 1))
    if $new_ok && $old_gone; then
        pass "VM-03: Renamed file indexed by new name and old name gone"
    else
        fail "VM-03: Renamed file indexed by new name and old name gone" \
             "new_ok=$new_ok, old_gone=$old_gone — index may not have updated"
    fi
else
    fail "VM-03: Renamed file indexed by new name and old name gone" \
         "Initial index not ready within ${FILENAME_INDEX_TIMEOUT}s"
fi

# ---------- VM-04: 文件移动后索引 ----------
echo ""
echo "--- VM-04: File move triggers index update ---"
mkdir -p "$TEST_DIR/vm_move_src" "$TEST_DIR/vm_move_dst"
echo "content" > "$TEST_DIR/vm_move_src/vm_movable.txt"
if wait_for_index "filename" "vm_movable" "$TEST_DIR" "vm_movable.txt"; then
    mv "$TEST_DIR/vm_move_src/vm_movable.txt" "$TEST_DIR/vm_move_dst/vm_movable.txt"
    # 双向轮询：新路径出现 AND 旧路径消失
    local_elapsed=0
    new_ok=false
    old_gone=false
    while [[ $local_elapsed -lt $FILENAME_INDEX_TIMEOUT ]]; do
        new_result=$(run_searcher "filename" "vm_movable" "$TEST_DIR/vm_move_dst") || true
        json_contains "$new_result" "vm_movable.txt" && new_ok=true
        old_result=$(run_searcher "filename" "vm_movable" "$TEST_DIR/vm_move_src") || true
        json_contains "$old_result" "vm_movable.txt" || old_gone=true
        $new_ok && $old_gone && break
        sleep "$INDEX_POLL_INTERVAL"
        local_elapsed=$((local_elapsed + INDEX_POLL_INTERVAL))
    done

    TEST_TOTAL=$((TEST_TOTAL + 1))
    if $new_ok && $old_gone; then
        pass "VM-04: Moved file found in new path and gone from old path"
    else
        fail "VM-04: Moved file found in new path and gone from old path" \
             "new_ok=$new_ok, old_gone=$old_gone — index may not have updated"
    fi
else
    fail "VM-04: Moved file found in new path and gone from old path" \
         "Initial index not ready within ${FILENAME_INDEX_TIMEOUT}s"
fi

# ---------- VM-05: 目录创建后索引 ----------
echo ""
echo "--- VM-05: Directory creation triggers index update ---"
mkdir -p "$TEST_DIR/vm_created_dir"

if wait_for_index "filename" "vm_created_dir" "$TEST_DIR" "vm_created_dir"; then
    result=$(run_searcher "filename" "vm_created_dir" "$TEST_DIR") || true
    assert_found "VM-05: Newly created directory found in index" "$result" "vm_created_dir"
else
    fail "VM-05: Newly created directory found in index" "Index not ready within ${FILENAME_INDEX_TIMEOUT}s"
fi

# ---------- VM-06: 目录删除后索引 ----------
echo ""
echo "--- VM-06: Directory deletion triggers index update ---"
mkdir -p "$TEST_DIR/vm_dir_to_delete"
if wait_for_index "filename" "vm_dir_to_delete" "$TEST_DIR" "vm_dir_to_delete"; then
    rmdir "$TEST_DIR/vm_dir_to_delete"
    # 轮询等待索引更新：旧目录名应从搜索结果消失
    local_elapsed=0
    dir_deleted_ok=false
    while [[ $local_elapsed -lt $FILENAME_INDEX_TIMEOUT ]]; do
        result=$(run_searcher "filename" "vm_dir_to_delete" "$TEST_DIR") || true
        if ! json_contains "$result" "vm_dir_to_delete"; then
            dir_deleted_ok=true
            break
        fi
        sleep "$INDEX_POLL_INTERVAL"
        local_elapsed=$((local_elapsed + INDEX_POLL_INTERVAL))
    done

    TEST_TOTAL=$((TEST_TOTAL + 1))
    if $dir_deleted_ok; then
        pass "VM-06: Deleted directory no longer in index"
    else
        fail "VM-06: Deleted directory no longer in index" "Index not updated after directory deletion"
    fi
else
    fail "VM-06: Deleted directory no longer in index" \
         "Initial index not ready within ${FILENAME_INDEX_TIMEOUT}s"
fi

# ---------- VM-07: 目录移动后索引 ----------
echo ""
echo "--- VM-07: Directory move triggers index update ---"
mkdir -p "$TEST_DIR/vm_dir_move_src/vm_movable_dir"
if wait_for_index "filename" "vm_movable_dir" "$TEST_DIR" "vm_movable_dir"; then
    mv "$TEST_DIR/vm_dir_move_src/vm_movable_dir" "$TEST_DIR/vm_movable_dir_new"
    # 双向轮询：新路径出现 AND 旧路径消失
    local_elapsed=0
    dir_new_ok=false
    dir_old_gone=false
    while [[ $local_elapsed -lt $FILENAME_INDEX_TIMEOUT ]]; do
        new_result=$(run_searcher "filename" "vm_movable_dir_new" "$TEST_DIR") || true
        json_contains "$new_result" "vm_movable_dir_new" && dir_new_ok=true
        old_result=$(run_searcher "filename" "vm_movable_dir" "$TEST_DIR/vm_dir_move_src") || true
        json_contains "$old_result" "vm_movable_dir" || dir_old_gone=true
        $dir_new_ok && $dir_old_gone && break
        sleep "$INDEX_POLL_INTERVAL"
        local_elapsed=$((local_elapsed + INDEX_POLL_INTERVAL))
    done

    TEST_TOTAL=$((TEST_TOTAL + 1))
    if $dir_new_ok && $dir_old_gone; then
        pass "VM-07: Moved directory found in new path and gone from old path"
    else
        fail "VM-07: Moved directory found in new path and gone from old path" \
             "new_ok=$dir_new_ok, old_gone=$dir_old_gone — index may not have updated"
    fi
else
    fail "VM-07: Moved directory found in new path and gone from old path" \
         "Initial index not ready within ${FILENAME_INDEX_TIMEOUT}s"
fi

# ---------- VM-08: 符号链接条目入索引（不跟随） ----------
# 设计契约：link 条目本身入索引（file link / dir link / dangling link，按链接
# 自身路径可搜），但绝不跟随——dir link 内部内容不经链接路径入索引（防索引环），
# 真实目标内容仍按真实路径入索引。
echo ""
echo "--- VM-08: Symlink entries are indexed but never followed ---"
echo "content" > "$TEST_DIR/vm_link_target.txt"
mkdir -p "$TEST_DIR/vm_link_realdir"
echo "content" > "$TEST_DIR/vm_link_realdir/vm_link_inner.txt"

if ! wait_for_index "filename" "vm_link_target" "$TEST_DIR" "vm_link_target.txt"; then
    fail "VM-08: Symlink entries are indexed but never followed" \
         "Initial index not ready within ${FILENAME_INDEX_TIMEOUT}s"
else
    # a) 文件链接；b) 目录链接；c) 悬空链接（目标不存在，文件管理器中仍可见）
    ln -s "$TEST_DIR/vm_link_target.txt" "$TEST_DIR/vm_link_file.txt"
    ln -s "$TEST_DIR/vm_link_realdir" "$TEST_DIR/vm_link_dir"
    ln -s "$TEST_DIR/vm_no_such_target.txt" "$TEST_DIR/vm_link_dangling.txt"

    link_file_ok=false
    link_dir_ok=false
    link_dangling_ok=false
    elapsed=0
    while [[ $elapsed -lt $FILENAME_INDEX_TIMEOUT ]]; do
        result=$(run_searcher "filename" "vm_link" "$TEST_DIR") || true
        json_contains "$result" "vm_link_file.txt" && link_file_ok=true
        json_contains "$result" "vm_link_dir" && link_dir_ok=true
        json_contains "$result" "vm_link_dangling.txt" && link_dangling_ok=true
        $link_file_ok && $link_dir_ok && $link_dangling_ok && break
        sleep "$INDEX_POLL_INTERVAL"
        elapsed=$((elapsed + INDEX_POLL_INTERVAL))
    done

    TEST_TOTAL=$((TEST_TOTAL + 1))
    if $link_file_ok; then
        pass "VM-08a: File symlink indexed by its own name"
    else
        fail "VM-08a: File symlink indexed by its own name" \
             "vm_link_file.txt not found in index — symlink filter regressed?"
    fi

    TEST_TOTAL=$((TEST_TOTAL + 1))
    if $link_dir_ok; then
        pass "VM-08b: Directory symlink indexed by its own name"
    else
        fail "VM-08b: Directory symlink indexed by its own name" \
             "vm_link_dir not found in index — symlink filter regressed?"
    fi

    TEST_TOTAL=$((TEST_TOTAL + 1))
    if $link_dangling_ok; then
        pass "VM-08c: Dangling symlink indexed by its own name"
    else
        fail "VM-08c: Dangling symlink indexed by its own name" \
             "vm_link_dangling.txt not found in index — symlink filter regressed?"
    fi

    # d) 绝不跟随：dir link 内部文件不经链接路径入索引（真实路径正常可搜）
    inner_real_ok=false
    elapsed=0
    while [[ $elapsed -lt $FILENAME_INDEX_TIMEOUT ]]; do
        result=$(run_searcher "filename" "vm_link_inner" "$TEST_DIR") || true
        json_contains "$result" "$TEST_DIR/vm_link_realdir/vm_link_inner.txt" && inner_real_ok=true
        $inner_real_ok && break
        sleep "$INDEX_POLL_INTERVAL"
        elapsed=$((elapsed + INDEX_POLL_INTERVAL))
    done

    result=$(run_searcher "filename" "vm_link_inner" "$TEST_DIR") || true
    TEST_TOTAL=$((TEST_TOTAL + 1))
    if $inner_real_ok && ! json_contains "$result" "$TEST_DIR/vm_link_dir/"; then
        pass "VM-08d: Dir link contents not indexed through the link path"
    else
        fail "VM-08d: Dir link contents not indexed through the link path" \
             "inner_real_ok=$inner_real_ok — contents must only be indexed under the real path"
    fi
fi

# ---------- VM-09: 文件内容新增后索引 ----------
echo ""
echo "--- VM-09: File content creation triggers content index update ---"
mkdir -p "$TEST_DIR/vm_content"
echo "这份文档包含独特关键词 vm_content_keyword_new。" > "$TEST_DIR/vm_content/new.txt"

if wait_for_index "content" "vm_content_keyword_new" "$TEST_DIR" "new.txt"; then
    result=$(run_searcher "content" "vm_content_keyword_new" "$TEST_DIR") || true
    assert_found "VM-09: Newly created content found in index" "$result" "new.txt"
else
    fail "VM-09: Newly created content found in index" \
         "Content index not ready within ${CONTENT_INDEX_TIMEOUT}s"
fi

# ---------- VM-10: 文件内容修改后索引 ----------
echo ""
echo "--- VM-10: File content modification triggers content index update ---"
mkdir -p "$TEST_DIR/vm_content_mod"
echo "原始内容包含关键词 vm_content_keyword_old。" > "$TEST_DIR/vm_content_mod/mod.txt"
if wait_for_index "content" "vm_content_keyword_old" "$TEST_DIR" "mod.txt"; then
    # 重写文件内容为新关键词
    echo "修改后的内容包含独特关键词 vm_content_keyword_updated。" > "$TEST_DIR/vm_content_mod/mod.txt"
    # 双向轮询：新关键词出现 AND 旧关键词消失
    local_elapsed=0
    new_kw_ok=false
    old_kw_gone=false
    while [[ $local_elapsed -lt $CONTENT_INDEX_TIMEOUT ]]; do
        new_result=$(run_searcher "content" "vm_content_keyword_updated" "$TEST_DIR") || true
        json_contains "$new_result" "mod.txt" && new_kw_ok=true
        old_result=$(run_searcher "content" "vm_content_keyword_old" "$TEST_DIR") || true
        json_contains "$old_result" "mod.txt" || old_kw_gone=true
        $new_kw_ok && $old_kw_gone && break
        sleep "$INDEX_POLL_INTERVAL"
        local_elapsed=$((local_elapsed + INDEX_POLL_INTERVAL))
    done

    TEST_TOTAL=$((TEST_TOTAL + 1))
    if $new_kw_ok && $old_kw_gone; then
        pass "VM-10: Updated content found and old keyword gone"
    else
        fail "VM-10: Updated content found and old keyword gone" \
             "new_ok=$new_kw_ok, old_gone=$old_kw_gone — content index may not have updated"
    fi
else
    fail "VM-10: Updated content found and old keyword gone" \
         "Initial content index not ready within ${CONTENT_INDEX_TIMEOUT}s"
fi

# ---------- VM-11: 文件内容清空后索引 ----------
echo ""
echo "--- VM-11: File content clear triggers content index update ---"
mkdir -p "$TEST_DIR/vm_content_clear"
echo "这份文档包含待清空关键词 vm_content_keyword_clear。" > "$TEST_DIR/vm_content_clear/clear.txt"
if wait_for_index "content" "vm_content_keyword_clear" "$TEST_DIR" "clear.txt"; then
    # 清空文件内容
    > "$TEST_DIR/vm_content_clear/clear.txt"
    # 轮询等待索引更新：原关键词应从搜索结果消失
    local_elapsed=0
    cleared_ok=false
    while [[ $local_elapsed -lt $CONTENT_INDEX_TIMEOUT ]]; do
        result=$(run_searcher "content" "vm_content_keyword_clear" "$TEST_DIR") || true
        if ! json_contains "$result" "clear.txt"; then
            cleared_ok=true
            break
        fi
        sleep "$INDEX_POLL_INTERVAL"
        local_elapsed=$((local_elapsed + INDEX_POLL_INTERVAL))
    done

    TEST_TOTAL=$((TEST_TOTAL + 1))
    if $cleared_ok; then
        pass "VM-11: Cleared content keyword no longer in index"
    else
        fail "VM-11: Cleared content keyword no longer in index" \
             "Content index not updated after clearing file"
    fi
else
    fail "VM-11: Cleared content keyword no longer in index" \
         "Initial content index not ready within ${CONTENT_INDEX_TIMEOUT}s"
fi

# ---------- VM-12: 文件删除后内容索引 ----------
echo ""
echo "--- VM-12: File deletion triggers content index update ---"
mkdir -p "$TEST_DIR/vm_content_del"
echo "这份文档包含待删除关键词 vm_content_keyword_del。" > "$TEST_DIR/vm_content_del/del.txt"
if wait_for_index "content" "vm_content_keyword_del" "$TEST_DIR" "del.txt"; then
    rm "$TEST_DIR/vm_content_del/del.txt"
    # 轮询等待索引更新：原关键词应从搜索结果消失
    local_elapsed=0
    del_ok=false
    while [[ $local_elapsed -lt $CONTENT_INDEX_TIMEOUT ]]; do
        result=$(run_searcher "content" "vm_content_keyword_del" "$TEST_DIR") || true
        if ! json_contains "$result" "del.txt"; then
            del_ok=true
            break
        fi
        sleep "$INDEX_POLL_INTERVAL"
        local_elapsed=$((local_elapsed + INDEX_POLL_INTERVAL))
    done

    TEST_TOTAL=$((TEST_TOTAL + 1))
    if $del_ok; then
        pass "VM-12: Deleted file content no longer in index"
    else
        fail "VM-12: Deleted file content no longer in index" \
             "Content index not updated after file deletion"
    fi
else
    fail "VM-12: Deleted file content no longer in index" \
         "Initial content index not ready within ${CONTENT_INDEX_TIMEOUT}s"
fi

# ---------- VM-13: 批量文件创建后索引 ----------
echo ""
echo "--- VM-13: Batch file creation triggers index update ---"
VM_BATCH_DIR="$TEST_DIR/vm_batch"
mkdir -p "$VM_BATCH_DIR"

# 统计去重后的 vm_batch 文件数
count_vm_batch_files() {
    if command -v jq &>/dev/null; then
        echo "$1" | jq -r '[.results[]? | select(test("vm_batch_[0-9]{4}\\.txt$"))] | unique | length' 2>/dev/null || echo 0
    else
        echo "$1" | grep -oE 'vm_batch_[0-9]{4}\.txt' | sort -u | wc -l
    fi
}

# 创建 VM_BATCH_COUNT 个文件，每个文件名包含独特 keyword
echo "Creating $VM_BATCH_COUNT files with unique filename keywords..."
for i in $(seq 1 "$VM_BATCH_COUNT"); do
    printf -v idx "%04d" "$i"
    echo "content" > "$VM_BATCH_DIR/vm_batch_${idx}.txt"
done

# 等待索引：轮询去重后的文件数量，直到全部文件入索引或超时
echo "Waiting for index (timeout: ${VM_BATCH_TIMEOUT}s)..."
elapsed=0
result=""
found_count=0
while [[ $elapsed -lt $VM_BATCH_TIMEOUT ]]; do
    result=$(run_searcher "filename" "vm_batch" "$VM_BATCH_DIR" "--max-results=$((VM_BATCH_COUNT + 100))") || true
    found_count=$(count_vm_batch_files "$result")
    if [[ "$found_count" -eq "$VM_BATCH_COUNT" ]]; then
        break
    fi
    sleep "$INDEX_POLL_INTERVAL"
    elapsed=$((elapsed + INDEX_POLL_INTERVAL))
done

TEST_TOTAL=$((TEST_TOTAL + 1))
if [[ "$found_count" -eq "$VM_BATCH_COUNT" ]]; then
    pass "VM-13: All $VM_BATCH_COUNT batch files found in index"
else
    fail "VM-13: All $VM_BATCH_COUNT batch files found in index" \
         "$found_count/$VM_BATCH_COUNT files found — index may not have fully updated"
fi

# 抽样验证几个单独的 keyword
if [[ "$found_count" -eq "$VM_BATCH_COUNT" ]]; then
    for sample_idx in 0001 0025 0050 0075 0100; do
        sample_result=$(run_searcher "filename" "vm_batch_${sample_idx}" "$VM_BATCH_DIR") || true
        assert_found "VM-13: Spot check vm_batch_${sample_idx}" "$sample_result" "vm_batch_${sample_idx}.txt"
    done
fi

# 清理批量测试文件
rm -rf "$VM_BATCH_DIR"
echo "Cleaned up batch test files."

# ---------- VM-14: 重复写入同一文件后索引 ----------
echo ""
echo "--- VM-14: Repeated write to same file triggers index update ---"
mkdir -p "$TEST_DIR/vm_repeat"
echo "第一次写入关键词 vm_repeat_kw_first。" > "$TEST_DIR/vm_repeat/repeat.txt"
if wait_for_index "content" "vm_repeat_kw_first" "$TEST_DIR" "repeat.txt"; then
    # 连续写入不同关键词
    echo "第二次写入关键词 vm_repeat_kw_second。" > "$TEST_DIR/vm_repeat/repeat.txt"
    sleep 1
    echo "第三次写入关键词 vm_repeat_kw_final。" > "$TEST_DIR/vm_repeat/repeat.txt"
    # 轮询等待索引更新：最后一次关键词应可搜到
    local_elapsed=0
    final_ok=false
    while [[ $local_elapsed -lt $CONTENT_INDEX_TIMEOUT ]]; do
        result=$(run_searcher "content" "vm_repeat_kw_final" "$TEST_DIR") || true
        if json_contains "$result" "repeat.txt"; then
            final_ok=true
            break
        fi
        sleep "$INDEX_POLL_INTERVAL"
        local_elapsed=$((local_elapsed + INDEX_POLL_INTERVAL))
    done

    TEST_TOTAL=$((TEST_TOTAL + 1))
    if $final_ok; then
        pass "VM-14: Final write keyword found in index"
    else
        fail "VM-14: Final write keyword found in index" \
             "Content index not updated after repeated writes"
    fi
else
    fail "VM-14: Final write keyword found in index" \
         "Initial content index not ready within ${CONTENT_INDEX_TIMEOUT}s"
fi

# ---------- VM-15: 大文件写入后索引 ----------
echo ""
echo "--- VM-15: Large file write triggers content index update ---"
mkdir -p "$TEST_DIR/vm_large"
VM_LARGE_FILE="$TEST_DIR/vm_large/large.txt"

# 生成约 1MB 文本文件，在中间和末尾各植入独特关键词
{
    for i in $(seq 1 20000); do
        echo "填充行 $i：这是一些普通文本内容用于填充文件大小。"
    done
    echo "大文件中间的独特关键词 vm_large_keyword_mid。"
    for i in $(seq 1 20000); do
        echo "填充行 $i：这是更多普通文本内容用于填充文件大小。"
    done
    echo "大文件末尾的独特关键词 vm_large_keyword_end。"
} > "$VM_LARGE_FILE"

if wait_for_index "content" "vm_large_keyword_end" "$TEST_DIR" "large.txt"; then
    result=$(run_searcher "content" "vm_large_keyword_end" "$TEST_DIR") || true
    assert_found "VM-15: Large file end keyword found in index" "$result" "large.txt"

    result=$(run_searcher "content" "vm_large_keyword_mid" "$TEST_DIR") || true
    assert_found "VM-15: Large file mid keyword found in index" "$result" "large.txt"
else
    fail "VM-15: Large file end keyword found in index" \
         "Content index not ready within ${CONTENT_INDEX_TIMEOUT}s"
    fail "VM-15: Large file mid keyword found in index" \
         "Content index not ready within ${CONTENT_INDEX_TIMEOUT}s"
fi

# =============================================================================
# 输出汇总
# =============================================================================
print_summary
