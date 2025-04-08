// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include "filenamesearchengine.h"

#include <QDebug>
#include <QDirIterator>
#include <QFileInfo>
#include <QDateTime>
#include <QMutexLocker>

#include "filenamestrategies/realtimestrategy.h"
#include "filenamestrategies/indexedstrategy.h"
#include "utils/searchutility.h"

DFM_SEARCH_BEGIN_NS
DCORE_USE_NAMESPACE

FileNameSearchEngine::FileNameSearchEngine(QObject *parent)
    : GenericSearchEngine(parent)
{
}

FileNameSearchEngine::~FileNameSearchEngine() = default;

void FileNameSearchEngine::setupStrategyFactory()
{
    // 设置文件名搜索策略工厂
    auto factory = std::make_unique<FileNameSearchStrategyFactory>();
    m_worker->setStrategyFactory(std::move(factory));
}

SearchError FileNameSearchEngine::validateSearchConditions()
{
    // 先执行基类验证
    auto result = GenericSearchEngine::validateSearchConditions();
    if (result.isError()) {
        return result;
    }

    FileNameOptionsAPI api(m_options);
    const auto &fileTypes = api.fileTypes();

    // Validate each file type in fileTypes against deepinAnythingFileTypes using std algorithms
    if (!fileTypes.isEmpty()) {
        auto invalidType = std::find_if(fileTypes.begin(), fileTypes.end(), [](const QString &type) {
            return !SearchUtility::deepinAnythingFileTypes().contains(type.trimmed().toLower());
        });

        if (invalidType != fileTypes.end()) {
            return SearchError(FileNameSearchErrorCode::InvalidFileTypes);
        }
    }

    // pinyin
    if (api.pinyinEnabled() && !SearchUtility::isPurePinyin(m_currentQuery.keyword())) {
        return SearchError(FileNameSearchErrorCode::InvalidPinyinFormat);
    }

    // 文件名搜索特定验证
    if (m_currentQuery.type() == SearchQuery::Type::Simple
        && m_currentQuery.keyword().isEmpty() && fileTypes.isEmpty()) {
        return SearchError(FileNameSearchErrorCode::KeywordIsEmpty);
    }

    return result;
}

std::unique_ptr<BaseSearchStrategy> FileNameSearchStrategyFactory::createStrategy(
        SearchType searchType, const SearchOptions &options)
{
    // 确保搜索类型正确
    if (searchType != SearchType::FileName) {
        return nullptr;
    }

    // 根据搜索方法创建对应的策略
    if (options.method() == SearchMethod::Indexed) {
        return std::make_unique<FileNameIndexedStrategy>(options);
    } else {
        return std::make_unique<FileNameRealTimeStrategy>(options);
    }
}

DFM_SEARCH_END_NS
