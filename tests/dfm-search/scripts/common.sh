#!/bin/bash
# SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: GPL-3.0-or-later

# =============================================================================
# common.sh — dfm-search 测试公共函数库
#
# 提供测试环境管理、索引等待、搜索断言、测试报告等公共能力，
# 供 test_filename_search.sh 和 test_content_search.sh 复用。
# =============================================================================

# ---------- 全局变量 ----------

TEST_PASS=0
TEST_FAIL=0
TEST_SKIP=0
TEST_TOTAL=0
TEST_TEMP_DIR=""
TEST_SEARCHER="${DFM_SEARCHER:-dfm-searcher}"

# 测试报告相关
# 报告输出目录，默认为 <脚本目录>/reports，可通过环境变量 TEST_REPORT_DIR 覆盖
TEST_REPORT_DIR="${TEST_REPORT_DIR:-}"
TEST_REPORT_FILE=""
TEST_REPORT_TMP=""
TEST_START_TS=0

# 索引等待超时（秒），可通过环境变量 INDEX_WAIT_TIMEOUT 覆盖
# 文件名索引默认 10 秒，内容索引默认 30 秒
FILENAME_INDEX_TIMEOUT="${INDEX_WAIT_TIMEOUT_FILENAME:-10}"
CONTENT_INDEX_TIMEOUT="${INDEX_WAIT_TIMEOUT_CONTENT:-30}"

# 压力测试索引等待超时（秒），可通过环境变量覆盖
# 大批量文件索引需要更长等待时间
STRESS_FILENAME_INDEX_TIMEOUT="${STRESS_INDEX_WAIT_TIMEOUT_FILENAME:-60}"
STRESS_CONTENT_INDEX_TIMEOUT="${STRESS_INDEX_WAIT_TIMEOUT_CONTENT:-120}"

# 索引轮询间隔（秒）
INDEX_POLL_INTERVAL="${INDEX_POLL_INTERVAL:-2}"

# ---------- 颜色输出 ----------

if [[ -t 1 ]]; then
    COLOR_PASS='\033[32m'
    COLOR_FAIL='\033[31m'
    COLOR_SKIP='\033[33m'
    COLOR_RESET='\033[0m'
else
    COLOR_PASS=''
    COLOR_FAIL=''
    COLOR_SKIP=''
    COLOR_RESET=''
fi

# ---------- 测试环境管理 ----------

# setup_test_env — 创建临时测试目录并初始化计数器
# 临时目录在 ~/Documents 下创建，确保在索引服务监控范围内
setup_test_env() {
    local prefix="${1:-dfm-search-test}"

    # 确保 ~/Documents 目录存在
    if [[ ! -d "$HOME/Documents" ]]; then
        echo "ERROR: ~/Documents directory does not exist" >&2
        return 1
    fi

    # 在 ~/Documents 下创建临时目录，确保索引服务能监控到
    TEST_TEMP_DIR=$(mktemp -d -p "$HOME/Documents" "${prefix}.XXXXXX")
    if [[ -z "$TEST_TEMP_DIR" ]]; then
        echo "ERROR: Failed to create temp directory under ~/Documents" >&2
        return 1
    fi

    TEST_PASS=0
    TEST_FAIL=0
    TEST_SKIP=0
    TEST_TOTAL=0
    TEST_START_TS=$(date +%s)

    echo "Test temp dir: $TEST_TEMP_DIR"
}

# cleanup_test_env — 清理临时测试目录
cleanup_test_env() {
    if [[ -n "$TEST_REPORT_TMP" && -f "$TEST_REPORT_TMP" ]]; then
        rm -f "$TEST_REPORT_TMP"
    fi
    if [[ -n "$TEST_TEMP_DIR" && -d "$TEST_TEMP_DIR" ]]; then
        rm -rf "$TEST_TEMP_DIR"
        echo "Cleaned up: $TEST_TEMP_DIR"
    fi
}

# ---------- 前置条件检查 ----------

# check_prerequisites — 检查 dfm-searcher 可用性和索引服务状态
# 返回 0 表示通过，非 0 表示不满足
check_prerequisites() {
    # 检查 dfm-searcher 是否可用
    if ! command -v "$TEST_SEARCHER" &>/dev/null; then
        echo "ERROR: $TEST_SEARCHER not found in PATH"
        echo "       Please build and install dfm-searcher first"
        return 1
    fi

    # 检查索引服务是否运行（尝试一次简单搜索）
    local check_output
    check_output=$("$TEST_SEARCHER" -j "test" "$HOME/Documents" 2>&1)
    if [[ $? -ne 0 ]]; then
        echo "WARNING: dfm-searcher returned error on simple search"
        echo "         Index service may not be running"
        echo "         Output: $check_output"
        return 1
    fi

    echo "Prerequisites check passed."
    return 0
}

# ---------- 索引等待 ----------

# wait_for_index — 轮询等待索引建立
# 参数:
#   $1: 搜索类型 (filename | content)
#   $2: 关键词
#   $3: 搜索路径
#   $4: 期望匹配的文件名片段（用于验证索引是否就绪）
#   $5: 额外的 dfm-searcher 选项（可选，作为单一字符串传递）
#   $6: 自定义超时秒数（可选，用于压力测试等需要更长等待的场景）
# 返回: 0 表示索引就绪，1 表示超时
wait_for_index() {
    local search_type="$1"
    local keyword="$2"
    local search_path="$3"
    local expected_fragment="$4"
    local extra_opts="${5:-}"

    local custom_timeout="${6:-}"
    local timeout
    if [[ -n "$custom_timeout" ]]; then
        timeout="$custom_timeout"
    elif [[ "$search_type" == "content" ]]; then
        timeout="$CONTENT_INDEX_TIMEOUT"
    else
        timeout="$FILENAME_INDEX_TIMEOUT"
    fi

    local elapsed=0
    local result

    while [[ $elapsed -lt $timeout ]]; do
        if [[ -n "$extra_opts" ]]; then
            result=$("$TEST_SEARCHER" --type="$search_type" -j "$keyword" "$search_path" $extra_opts 2>/dev/null)
        else
            result=$("$TEST_SEARCHER" --type="$search_type" -j "$keyword" "$search_path" 2>/dev/null)
        fi

        if json_contains "$result" "$expected_fragment"; then
            return 0
        fi

        sleep "$INDEX_POLL_INTERVAL"
        elapsed=$((elapsed + INDEX_POLL_INTERVAL))
    done

    return 1
}

# ---------- JSON 解析 ----------

# json_contains — 检查 JSON 输出中是否包含指定路径片段
# 优先使用 jq，不可用时降级为 grep
# 参数:
#   $1: JSON 输出字符串
#   $2: 期望匹配的文件路径片段
json_contains() {
    local json="$1"
    local fragment="$2"

    if [[ -z "$json" || -z "$fragment" ]]; then
        return 1
    fi

    if command -v jq &>/dev/null; then
        # 兼容两种输出格式：
        # 1. 普通 JSON：results 为路径字符串数组或含 path 字段的对象数组
        # 2. 实时搜索 NDJSON 流式输出：{"type":"result","data":"<路径>"} 事件行
        echo "$json" | jq -r '
            (select(.type? == "result") | .data? // empty),
            (.results[]? | if type == "string" then . elif type == "object" then (.path? // empty) else empty end)
        ' 2>/dev/null | grep -qF "$fragment"
        return $?
    else
        # 降级为 grep
        echo "$json" | grep -qF "$fragment"
        return $?
    fi
}

# json_count_results — 统计 JSON 输出中的结果数量
# 参数:
#   $1: JSON 输出字符串
json_count_results() {
    local json="$1"

    if [[ -z "$json" ]]; then
        echo 0
        return
    fi

    if command -v jq &>/dev/null; then
        local count
        count=$(echo "$json" | jq -r '.status.totalResults // (.results | length) // 0' 2>/dev/null)
        echo "${count:-0}"
    else
        # 降级：统计 "path" 出现次数
        echo "$json" | grep -o '"path"' | wc -l
    fi
}

# ---------- 搜索执行 ----------

# run_searcher — 执行 dfm-searcher 并返回 JSON 输出
# 参数:
#   $1: 搜索类型 (filename | content)
#   $2: 关键词
#   $3: 搜索路径
#   $@: 额外选项（直接传递给 dfm-searcher）
run_searcher() {
    local search_type="$1"
    local keyword="$2"
    local search_path="$3"
    shift 3

    "$TEST_SEARCHER" --type="$search_type" -j "$keyword" "$search_path" "$@" 2>/dev/null
}

# ---------- 断言函数 ----------

# assert_found — 断言搜索结果包含指定文件路径片段
# 参数:
#   $1: 测试用例名称
#   $2: JSON 输出
#   $3: 期望匹配的文件路径片段
assert_found() {
    local test_name="$1"
    local json="$2"
    local expected="$3"

    TEST_TOTAL=$((TEST_TOTAL + 1))
    if json_contains "$json" "$expected"; then
        pass "$test_name"
    else
        fail "$test_name" "Expected to find '$expected' in search results"
    fi
}

# assert_not_found — 断言搜索结果不包含指定文件路径片段
# 参数:
#   $1: 测试用例名称
#   $2: JSON 输出
#   $3: 不期望匹配的文件路径片段
assert_not_found() {
    local test_name="$1"
    local json="$2"
    local unexpected="$3"

    TEST_TOTAL=$((TEST_TOTAL + 1))
    if json_contains "$json" "$unexpected"; then
        fail "$test_name" "Unexpectedly found '$unexpected' in search results"
    else
        pass "$test_name"
    fi
}

# assert_count — 断言搜索结果数量等于指定值
# 参数:
#   $1: 测试用例名称
#   $2: JSON 输出
#   $3: 期望数量
assert_count() {
    local test_name="$1"
    local json="$2"
    local expected_count="$3"

    TEST_TOTAL=$((TEST_TOTAL + 1))
    local actual_count
    actual_count=$(json_count_results "$json")
    if [[ "$actual_count" -eq "$expected_count" ]]; then
        pass "$test_name"
    else
        fail "$test_name" "Expected $expected_count results, got $actual_count"
    fi
}

# assert_le_count — 断言搜索结果数量不超过指定值
# 参数:
#   $1: 测试用例名称
#   $2: JSON 输出
#   $3: 最大数量
assert_le_count() {
    local test_name="$1"
    local json="$2"
    local max_count="$3"

    TEST_TOTAL=$((TEST_TOTAL + 1))
    local actual_count
    actual_count=$(json_count_results "$json")
    if [[ "$actual_count" -le "$max_count" ]]; then
        pass "$test_name"
    else
        fail "$test_name" "Expected <= $max_count results, got $actual_count"
    fi
}

# ---------- 测试报告 ----------

# record_result — 将单条测试结果记录到临时结果文件（用于最终生成报告）
# 参数:
#   $1: 状态 (PASS | FAIL | SKIP)
#   $2: 测试用例名称
#   $3: 附加消息（可选）
record_result() {
    local status="$1"
    local name="$2"
    local message="${3:-}"

    # 懒初始化临时结果文件
    if [[ -z "$TEST_REPORT_TMP" ]]; then
        TEST_REPORT_TMP=$(mktemp) || return 0
    fi

    printf '%s\t%s\t%s\n' "$status" "$name" "$message" >> "$TEST_REPORT_TMP" 2>/dev/null || true
}

pass() {
    TEST_PASS=$((TEST_PASS + 1))
    echo -e "${COLOR_PASS}[PASS]${COLOR_RESET} $1"
    record_result "PASS" "$1"
}

fail() {
    TEST_FAIL=$((TEST_FAIL + 1))
    echo -e "${COLOR_FAIL}[FAIL]${COLOR_RESET} $1 — $2"
    record_result "FAIL" "$1" "$2"
}

skip() {
    TEST_SKIP=$((TEST_SKIP + 1))
    echo -e "${COLOR_SKIP}[SKIP]${COLOR_RESET} $1 — $2"
    record_result "SKIP" "$1" "$2"
}

# generate_report — 生成测试报告文件
# 报告写入 TEST_REPORT_DIR（默认 <脚本目录>/reports），文件名含脚本名与时间戳，
# 生成的路径保存在 TEST_REPORT_FILE 中
generate_report() {
    local script_name
    script_name=$(basename "${0%.sh}")

    local report_dir="${TEST_REPORT_DIR:-${SCRIPT_DIR:-.}/reports}"
    if ! mkdir -p "$report_dir" 2>/dev/null; then
        echo "WARNING: Failed to create report dir: $report_dir, report not saved" >&2
        return 1
    fi

    TEST_REPORT_FILE="$report_dir/report_${script_name}_$(date '+%Y%m%d_%H%M%S').txt"

    local duration="N/A"
    if [[ "$TEST_START_TS" -gt 0 ]]; then
        duration="$(( $(date +%s) - TEST_START_TS ))s"
    fi

    {
        echo "========================================="
        echo " dfm-search 集成测试报告"
        echo "========================================="
        echo " 脚本:        $(basename "$0")"
        echo " 日期:        $(date '+%Y-%m-%d %H:%M:%S')"
        echo " 耗时:        $duration"
        echo " 主机:        $(hostname 2>/dev/null || echo unknown)"
        echo " dfm-searcher: $TEST_SEARCHER"
        echo " 临时目录:    ${TEST_TEMP_DIR:-N/A}"
        echo "-----------------------------------------"
        echo " 用例结果"
        echo "-----------------------------------------"
        if [[ -n "$TEST_REPORT_TMP" && -f "$TEST_REPORT_TMP" ]]; then
            while IFS=$'\t' read -r status name message; do
                if [[ -n "$message" ]]; then
                    printf '[%s] %s — %s\n' "$status" "$name" "$message"
                else
                    printf '[%s] %s\n' "$status" "$name"
                fi
            done < "$TEST_REPORT_TMP"
        fi
        echo "-----------------------------------------"
        echo " 汇总"
        echo "-----------------------------------------"
        echo " Total: $TEST_TOTAL"
        echo " Pass:  $TEST_PASS"
        echo " Fail:  $TEST_FAIL"
        echo " Skip:  $TEST_SKIP"
        if [[ $TEST_FAIL -gt 0 ]]; then
            echo " 结果:  FAILED"
        else
            echo " 结果:  PASSED"
        fi
        echo "========================================="
    } > "$TEST_REPORT_FILE"

    return 0
}

# print_summary — 输出测试汇总、生成报告文件并设置退出码
# 返回: 0（全 PASS 或仅有 SKIP），1（有 FAIL）
print_summary() {
    echo ""
    echo "========================================="
    echo " Test Summary"
    echo "========================================="
    echo " Total: $TEST_TOTAL"
    echo " Pass:  $TEST_PASS"
    echo " Fail:  $TEST_FAIL"
    echo " Skip:  $TEST_SKIP"
    echo "========================================="

    if generate_report; then
        echo ""
        echo "Test report: $TEST_REPORT_FILE"
    fi

    if [[ $TEST_FAIL -gt 0 ]]; then
        return 1
    else
        return 0
    fi
}
