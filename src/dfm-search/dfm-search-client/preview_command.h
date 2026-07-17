// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef PREVIEW_COMMAND_H
#define PREVIEW_COMMAND_H

#include "cli_options.h"

#include <dfm-search/contentretriever.h>

#include <QString>

namespace dfmsearch {

struct PreviewCommandResult
{
    int exitCode = 0;
    QString stdoutText;
    QString stderrText;
};

PreviewCommandResult runPreviewCommand(const SearchCliConfig &config,
                                       DFMSEARCH::ContentRetriever &retriever);

}   // namespace dfmsearch

#endif   // PREVIEW_COMMAND_H
