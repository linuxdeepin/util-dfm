// SPDX-FileCopyrightText: 2025 - 2026 UnionTech Software Technology Co., Ltd.
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
    const auto &fileExts = api.fileExtensions();

    // Validate each file type in fileTypes against deepinAnythingFileTypes using std algorithms
    if (!fileTypes.isEmpty()) {
        auto invalidType = std::find_if(fileTypes.begin(), fileTypes.end(), [](const QString &type) {
            return !SearchUtility::deepinAnythingFileTypes().contains(type.trimmed().toLower());
        });

        if (invalidType != fileTypes.end()) {
            return SearchError(FileNameSearchErrorCode::InvalidFileTypes);
        }
    }

    // 文件名搜索特定验证
    if (m_currentQuery.type() == SearchQuery::Type::Simple
        || m_currentQuery.type() == SearchQuery::Type::Wildcard) {
        // 允许对一个类型, 后缀进行搜索，获取类型下所有文件
        if (m_currentQuery.keyword().isEmpty() && fileTypes.isEmpty() && fileExts.isEmpty()) {
            return SearchError(FileNameSearchErrorCode::KeywordIsEmpty);
        }

        // pinyin (wildcard类型不支持拼音搜索)
        if (m_currentQuery.type() == SearchQuery::Type::Simple
            && api.pinyinEnabled() && !Global::isPinyinSequence(m_currentQuery.keyword())) {
            qWarning() << SearchError(FileNameSearchErrorCode::InvalidPinyinFormat).message() << "key: " << m_currentQuery.keyword();
        }
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
