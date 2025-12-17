/////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2024 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Lucene++ SearchCancellation Compatibility Layer
//
// This header provides compatibility for Lucene++ search cancellation feature.
// It automatically detects whether the installed Lucene++ version supports
// SearchCancellation and provides appropriate implementation.
/////////////////////////////////////////////////////////////////////////////

#ifndef LUCENE_CANCELLATION_COMPAT_H
#define LUCENE_CANCELLATION_COMPAT_H

#include <atomic>

// 自动检测 Lucene++ 是否支持 SearchCancellation
// 使用 C++17 的 __has_include 特性进行编译时检测
#if defined(__has_include)
#    if __has_include(<lucene++/SearchCancellation.h>)
#        define LUCENE_HAS_SEARCH_CANCELLATION 1
#    else
#        define LUCENE_HAS_SEARCH_CANCELLATION 0
#    endif
#else
// 如果编译器不支持 __has_include，默认假设没有
#    define LUCENE_HAS_SEARCH_CANCELLATION 0
#endif

// ============================================================================
// Deepin 环境：使用 Lucene++ 官方实现
// ============================================================================
#if LUCENE_HAS_SEARCH_CANCELLATION

#    include <lucene++/SearchCancellation.h>

// 使用 Lucene++ 的完整实现
// - SearchCancellation::setFlag()
// - SearchCancellation::shouldCancel()
// - SearchCancellationGuard (RAII)

// ============================================================================
// 其他发行版：提供兼容层空实现
// ============================================================================
#else

#    include <lucene++/LuceneHeaders.h>

namespace Lucene {

/**
 * @brief 兼容层：搜索取消上下文（空实现）
 *
 * 在不支持 SearchCancellation 的 Lucene++ 版本中，
 * 此类提供空操作，确保代码可以编译通过。
 *
 * 注意：取消功能将不可用，但不会影响基本搜索功能。
 * 用户在这种环境下仍可通过 CancellableCollector 实现部分取消功能。
 */
class SearchCancellation
{
public:
    /**
     * @brief 设置取消标志（空实现）
     * @param flag 取消标志指针
     */
    static void setFlag(std::atomic<bool> *flag)
    {
        // 空实现：不执行任何操作
        (void)flag;   // 避免未使用参数警告
    }

    /**
     * @brief 获取取消标志（空实现）
     * @return nullptr
     */
    static std::atomic<bool> *getFlag()
    {
        return nullptr;
    }

    /**
     * @brief 清除取消标志（空实现）
     */
    static void clear()
    {
        // 空实现：不执行任何操作
    }

    /**
     * @brief 检查是否应该取消（空实现）
     * @return false（永远不取消）
     */
    static bool shouldCancel()
    {
        return false;
    }
};

/**
 * @brief 兼容层：RAII 取消守护类（空实现）
 *
 * 此类提供与 Lucene++ 官方实现相同的接口，
 * 但所有操作都是空操作，确保代码可以编译。
 *
 * 使用示例：
 * @code
 * void performSearch() {
 *     Lucene::SearchCancellationGuard guard(&m_cancelled);
 *     // ... 执行搜索 ...
 * } // 自动清理（空操作）
 * @endcode
 */
class SearchCancellationGuard
{
public:
    /**
     * @brief 构造函数（空实现）
     * @param flag 取消标志指针
     */
    explicit SearchCancellationGuard(std::atomic<bool> *flag)
    {
        // 空实现：不执行任何操作
        (void)flag;   // 避免未使用参数警告
    }

    /**
     * @brief 析构函数（空实现）
     */
    ~SearchCancellationGuard()
    {
        // 空实现：不执行任何操作
    }

    // 禁用拷贝和移动，保持与官方实现一致的接口
    SearchCancellationGuard(const SearchCancellationGuard &) = delete;
    SearchCancellationGuard &operator=(const SearchCancellationGuard &) = delete;
    SearchCancellationGuard(SearchCancellationGuard &&) = delete;
    SearchCancellationGuard &operator=(SearchCancellationGuard &&) = delete;
};

}   // namespace Lucene

// 编译时提示信息
#    pragma message("Lucene++ SearchCancellation not available. Search cancellation in phraseFreq() disabled, but CancellableCollector still works.")

#endif   // LUCENE_HAS_SEARCH_CANCELLATION

#endif   // LUCENE_CANCELLATION_COMPAT_H
