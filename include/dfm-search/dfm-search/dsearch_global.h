// SPDX-FileCopyrightText: 2025 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DSEARCH_GLOBAL_H
#define DSEARCH_GLOBAL_H

#include <optional>
#include <memory>

#include <QObject>

#define DFMSEARCH dfmsearch
#define DFM_SEARCH_BEGIN_NS namespace DFMSEARCH {
#define DFM_SEARCH_END_NS }
#define DFM_SEARCH_USE_NS using namespace DFMSEARCH;

DFM_SEARCH_BEGIN_NS
Q_NAMESPACE

namespace Global {
inline constexpr int kMinContentSearchKeywordLength = 2;

/**
 * @brief Check if a string contains only pinyin characters
 * @param input The string to check
 * @return true if the string contains only pinyin, false otherwise
 */
bool isPinyinSequence(const QString &input);

/**
 * @brief Check if a string is a valid pinyin acronym sequence
 * @param input The string to check
 * @return true if the string is a valid pinyin acronym, false otherwise
 */
bool isPinyinAcronymSequence(const QString &input);

/**
 * Determines if the given absolute path represents a hidden file/directory
 * or is contained within a hidden directory.
 *
 * A path is considered hidden if any component (except root) starts with '.'
 * and is not the special "." (current) or ".." (parent) directory.
 *
 * @param absolutePath The absolute filesystem path to check (must be normalized)
 * @return true if the path is hidden or inside hidden directories, false otherwise
 */
bool isHiddenPathOrInHiddenDir(const QString &absolutePath);

/**
 * @brief Check if the given file extension is supported for full-text search.
 * This function checks if the specified file extension is included in the list of supported extensions
 * for performing full-text searches within the application.
 * @param suffix The file extension to check.
 * @return True if the extension is supported, false otherwise.
 */
bool isSupportedContentSearchExtension(const QString &suffix);

/**
 * @brief Get a list of default file extensions that are supported for full-text search.
 * This function returns a list of file extensions that the application can process for full-text search,
 * ensuring that users can search through various document types.
 * @return A QStringList containing the supported file extensions.
 */
QStringList defaultContentSearchExtensions();

/**
 * @brief Gets the list of default indexed directories with deduplicated parent/child paths.
 *
 * If the list contains both a parent directory (e.g. "/home") and its child ("/home/test"),
 * only the parent directory is kept. Paths are normalized using QDir::cleanPath.
 *
 * @return QStringList Deduplicated directory list (sorted alphabetically)
 */
QStringList defaultIndexedDirectory();

/**
 * @brief Get the list of blacklist paths from DConfig.
 * This function reads the blacklist_paths configuration from the anything DConfig
 * and returns it as-is without additional processing.
 * @return A QStringList containing the blacklist paths, or empty list if reading fails.
 */
QStringList defaultBlacklistPaths();

/**
 * @brief Check if the specified path is within the content index directory.
 * This function verifies whether a given file path is located within the designated content index directory,
 * which is important for ensuring that only relevant files are included in search operations.
 * @param path The file path to check.
 * @return True if the path is within the content index directory, false otherwise.
 */
bool isPathInContentIndexDirectory(const QString &path);

/**
 * @brief Check if the content index is available.
 * This function checks the status of the content index to determine if it is accessible and ready for search operations.
 * @return True if the content index is available, false otherwise.
 */
bool isContentIndexAvailable();

/**
 * @brief Get the content index directory path.
 * This function provides the path to the directory where the content index is stored,
 * which is essential for performing searches on indexed content.
 * @return The path to the content index directory.
 */
QString contentIndexDirectory();

/**
 * @brief Check if the specified path is within the filename index directory.
 * This function verifies whether a given file path is located within the designated filename index directory,
 * ensuring that only relevant files are included in filename search operations.
 * @param path The file path to check.
 * @return True if the path is within the filename index directory, false otherwise.
 */
bool isPathInFileNameIndexDirectory(const QString &path);

/**
 * @brief Check if the filename index directory is available.
 * This function checks if the filename index directory physically exists.
 * @return True if the filename index directory is available, false otherwise.
 */
bool isFileNameIndexDirectoryAvailable();

/**
 * @brief Check if the filename index is ready for search operations.
 * This function checks both the physical existence of the index and its status to ensure
 * it's in the "monitoring" state and ready for search operations.
 * @return True if the filename index is ready for search, false otherwise.
 */
bool isFileNameIndexReadyForSearch();

/**
 * @brief Returns the current indexing status of the file name database.
 *
 * Possible status values:
 * - "loading"     : Initial loading of existing index
 * - "scanning"    : Actively scanning filesystem for changes
 * - "monitoring"  : Scan complete, now watching for filesystem events
 * - "closed"      : Normal shutdown state (anything terminated properly)
 *
 * @return QString The current status string (lowercase)
 */
std::optional<QString> fileNameIndexStatus();

/**
 * @brief Get the filename index directory path.
 * This function provides the path to the directory where the filename index is stored,
 * which is essential for performing searches on indexed filenames.
 * @return The path to the filename index directory.
 */
QString fileNameIndexDirectory();

/**
 * @brief Get the version of the filename index from the JSON configuration file.
 * This function reads the version field from the filename index JSON file and returns it as an integer.
 * @return The version number of the filename index, or -1 if the version cannot be retrieved.
 */
int fileNameIndexVersion();

/**
 * @brief Get the version of the content index from the JSON configuration file.
 * This function reads the version field from the content index JSON file and returns it as an integer.
 * @return The version number of the content index, or -1 if the version cannot be retrieved.
 */
int contentIndexVersion();

}   // namespace Global

// Enumeration for different types of search methods
enum SearchType {
    FileName,   // Search by file name
    Content,   // Search by content within files
    Custom = 50   // User-defined search type
};
Q_ENUM_NS(SearchType)

// Enumeration for the status of the search operation
enum SearchStatus {
    Ready,   // The search engine is ready to perform a search
    Searching,   // The search operation is currently in progress
    Finished,   // The search operation has completed
    Cancelled,   // The search operation has been cancelled
    Error   // An error occurred during the search operation
    // Pause ?   // Optional: Indicates if the search can be paused
};
Q_ENUM_NS(SearchStatus)

// Enumeration for the method of searching
enum SearchMethod {
    Indexed,   // Search using pre-built indexes for faster results
    Realtime   // Search the file system in real-time for the most current results
};
Q_ENUM_NS(SearchMethod)

DFM_SEARCH_END_NS

Q_DECLARE_METATYPE(DFMSEARCH::SearchType);

#endif   // DSEARCH_GLOBAL_H
