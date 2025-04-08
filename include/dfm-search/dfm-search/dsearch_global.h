// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DSEARCH_GLOBAL_H
#define DSEARCH_GLOBAL_H

#include <QObject>

#define DFMSEARCH dfmsearch
#define DFM_SEARCH_BEGIN_NS namespace DFMSEARCH {
#define DFM_SEARCH_END_NS }
#define DFM_SEARCH_USE_NS using namespace DFMSEARCH;

DFM_SEARCH_BEGIN_NS
Q_NAMESPACE

// Enumeration for different types of search methods
enum SearchType {
    FileName,   // Search by file name
    Content,    // Search by content within files
    Custom = 50 // User-defined search type
};
Q_ENUM_NS(SearchType)

// Enumeration for the status of the search operation
enum SearchStatus {
    Ready,      // The search engine is ready to perform a search
    Searching,  // The search operation is currently in progress
    Finished,   // The search operation has completed
    Cancelled,  // The search operation has been cancelled
    Error       // An error occurred during the search operation
    // Pause ?   // Optional: Indicates if the search can be paused
};
Q_ENUM_NS(SearchStatus)

// Enumeration for the method of searching
enum SearchMethod {
    Indexed,    // Search using pre-built indexes for faster results
    Realtime    // Search the file system in real-time for the most current results
};
Q_ENUM_NS(SearchMethod)

DFM_SEARCH_END_NS

#endif   // DSEARCH_GLOBAL_H
