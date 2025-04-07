#include "filenamesearchengine.h"
#include <QDebug>
#include <QDirIterator>
#include <QFileInfo>
#include <QDateTime>
#include <QMutexLocker>
#include "filenamestrategies/realtimestrategy.h"
#include "filenamestrategies/indexedstrategy.h"

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

SearchResultExpected FileNameSearchEngine::validateSearchConditions(const SearchQuery &query)
{
    // 先执行基类验证
    auto result = GenericSearchEngine::validateSearchConditions(query);
    if (!result.hasValue()) {
        return result;
    }

    // 文件名搜索特定验证
    if (query.keyword().isEmpty()) {
        return DUnexpected<DFMSEARCH::SearchError> { SearchError(FileNameSearchErrorCode::InvalidFileName) };
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
