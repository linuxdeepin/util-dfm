// SPDX-FileCopyrightText: 2025 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <dfm-search/dsearch_global.h>

#include <lucene++/LuceneHeaders.h>

DFM_SEARCH_BEGIN_NS

namespace LuceneFieldNames {

// File name index field names
namespace FileName {
constexpr const wchar_t kFileType[] = L"file_type";
constexpr const wchar_t kFileExt[] = L"file_ext";
constexpr const wchar_t kFileName[] = L"file_name";
constexpr const wchar_t kFileNameLower[] = L"file_name_lower";
constexpr const wchar_t kFullPath[] = L"full_path";
constexpr const wchar_t kIsHidden[] = L"is_hidden";
constexpr const wchar_t kModifyTimeStr[] = L"modify_time_str";
constexpr const wchar_t kFileSizeStr[] = L"file_size_str";
constexpr const wchar_t kPinyin[] = L"pinyin";
constexpr const wchar_t kPinyinAcronym[] = L"pinyin_acronym";
constexpr const wchar_t kAncestorPaths[] = L"ancestor_paths";
}   // namespace FileName

// Content index field names
namespace Content {
constexpr const wchar_t kContents[] = L"contents";
constexpr const wchar_t kFilename[] = L"filename";
constexpr const wchar_t kPath[] = L"path";
constexpr const wchar_t kIsHidden[] = L"is_hidden";
constexpr const wchar_t kAncestorPaths[] = L"ancestor_paths";
}   // namespace Content

}   // namespace LuceneFieldNames

DFM_SEARCH_END_NS
