// SPDX-FileCopyrightText: 2025 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include <dfm-search/filenamesearchapi.h>

DFM_SEARCH_BEGIN_NS

FileNameOptionsAPI::FileNameOptionsAPI(SearchOptions &options)
    : m_options(options)
{
}

void FileNameOptionsAPI::setPinyinEnabled(bool enabled)
{
    m_options.setCustomOption("pinyinEnabled", enabled);
}

bool FileNameOptionsAPI::pinyinEnabled() const
{
    return m_options.customOption("pinyinEnabled").toBool();
}

void FileNameOptionsAPI::setPinyinAcronymEnabled(bool enabled)
{
    m_options.setCustomOption("pinyinAcronymEnabled", enabled);
}

bool FileNameOptionsAPI::pinyinAcronymEnabled() const
{
    return m_options.customOption("pinyinAcronymEnabled").toBool();
}

void FileNameOptionsAPI::setFileTypes(const QStringList &types)
{
    m_options.setCustomOption("fileTypes", types);
}

QStringList FileNameOptionsAPI::fileTypes() const
{
    return m_options.customOption("fileTypes").toStringList();
}

void FileNameOptionsAPI::setFileExtensions(const QStringList &extensions)
{
    m_options.setCustomOption("fileExtensions", extensions);
}

QStringList FileNameOptionsAPI::fileExtensions() const
{
    return m_options.customOption("fileExtensions").toStringList();
}

//////////

FileNameResultAPI::FileNameResultAPI(SearchResult &result)
    : m_result(result)
{
}

QString FileNameResultAPI::size() const
{
    return m_result.customAttribute("size").toString();
}

void FileNameResultAPI::setSize(const QString &size)
{
    m_result.setCustomAttribute("size", size);
}

QString FileNameResultAPI::modifiedTime() const
{
    return m_result.customAttribute("modifiedTime").toString();
}

void FileNameResultAPI::setModifiedTime(const QString &time)
{
    m_result.setCustomAttribute("modifiedTime", time);
}

bool FileNameResultAPI::isDirectory() const
{
    return m_result.customAttribute("isDirectory").toBool();
}

void FileNameResultAPI::setIsDirectory(bool isDir)
{
    m_result.setCustomAttribute("isDirectory", isDir);
}

QString FileNameResultAPI::fileType() const
{
    return m_result.customAttribute("fileType").toString();
}

void FileNameResultAPI::setFileType(const QString &type) const
{
    m_result.setCustomAttribute("fileType", type);
}

DFM_SEARCH_END_NS
