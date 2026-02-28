// SPDX-FileCopyrightText: 2025 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef FILENAMESEARCHAPI_H
#define FILENAMESEARCHAPI_H

#include <dfm-search/dsearch_global.h>
#include <dfm-search/searchoptions.h>
#include <dfm-search/searchresult.h>

DFM_SEARCH_BEGIN_NS

/**
 * @brief The FileNameOptionsAPI class provides file name search specific options
 *
 * This class extends the base SearchOptions with file name search specific settings,
 * such as pinyin search and file type filtering.
 */
class FileNameOptionsAPI
{
public:
    /**
     * @brief Constructor
     * @param options The SearchOptions object to operate on
     */
    explicit FileNameOptionsAPI(SearchOptions &options);

    /**
     * @brief Enable or disable pinyin search
     * @param enabled true to enable pinyin search, false to disable
     */
    void setPinyinEnabled(bool enabled);

    /**
     * @brief Check if pinyin search is enabled
     * @return true if pinyin search is enabled, false otherwise
     */
    bool pinyinEnabled() const;

    /**
     * @brief Enable or disable pinyin acronym search
     * @param enabled true to enable pinyin acronym search, false to disable
     */
    void setPinyinAcronymEnabled(bool enabled);

    /**
     * @brief Check if pinyin acronym search is enabled
     * @return true if pinyin acronym search is enabled, false otherwise
     */
    bool pinyinAcronymEnabled() const;

    /**
     * @brief Set file type filters
     * @param types List of file types to include in search
     *
     * Supported file types:
     * - app: Application files
     * - archive: Archive files
     * - audio: Audio files
     * - doc: Document files
     * - pic: Picture files
     * - video: Video files
     * - dir: folder
     * - other: other files
     */
    void setFileTypes(const QStringList &types);

    /**
     * @brief Get the current file type filters
     * @return List of file types
     */
    QStringList fileTypes() const;

    /**
     * @brief Set file extension filters
     * @param extensions List of file extensions to include in search
     * 
     * Example: "txt", "pdf", "docx"
     */
    void setFileExtensions(const QStringList &extensions);

    /**
     * @brief Get the current file extension filters
     * @return List of file extensions
     */
    QStringList fileExtensions() const;

private:
    SearchOptions &m_options;
};

/**
 * @brief The FileNameResultAPI class provides file name search specific result handling
 *
 * This class extends the base SearchResult with file name search specific features,
 * such as file size, modification time, and file type information.
 */
class FileNameResultAPI
{
public:
    /**
     * @brief Constructor
     * @param result The SearchResult object to operate on
     */
    explicit FileNameResultAPI(SearchResult &result);

    /**
     * @brief Get the file size
     * @return The file size as formatted string
     */
    QString size() const;

    /**
     * @brief Set the file size
     * @param size The file size as formatted string
     */
    void setSize(const QString &size);

    /**
     * @brief Get the file modification time
     * @return The modification time as formatted string
     */
    QString modifiedTime() const;

    /**
     * @brief Set the file modification time
     * @param time The modification time as formatted string
     */
    void setModifiedTime(const QString &time);

    /**
     * @brief Check if the result is a directory
     * @return true if the result is a directory, false otherwise
     */
    bool isDirectory() const;

    /**
     * @brief Set whether the result is a directory
     * @param isDir true if the result is a directory, false otherwise
     */
    void setIsDirectory(bool isDir);

    /**
     * @brief Get the file type
     * @return The file type as string
     */
    QString fileType() const;

    /**
     * @brief Set the file type
     * @param type The file type to set
     */
    void setFileType(const QString &type) const;

private:
    SearchResult &m_result;
};

DFM_SEARCH_END_NS

#endif   // FILENAMESEARCHAPI_H
