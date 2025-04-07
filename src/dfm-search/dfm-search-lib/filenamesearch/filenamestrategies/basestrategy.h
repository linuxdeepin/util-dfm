#ifndef FILENAME_BASE_STRATEGY_H
#define FILENAME_BASE_STRATEGY_H

#include "core/searchstrategy/basesearchstrategy.h"
#include <dfm-search/filenamesearchapi.h>

DFM_SEARCH_BEGIN_NS

/**
 * @brief 文件名搜索策略基类
 */
class FileNameBaseStrategy : public BaseSearchStrategy
{
    Q_OBJECT
    
public:
    explicit FileNameBaseStrategy(const SearchOptions &options, QObject *parent = nullptr)
        : BaseSearchStrategy(options, parent) {}
    
    SearchType searchType() const override { return SearchType::FileName; }
    
protected:
    // 公共工具方法
    bool matchesQuery(const QString &fileName, const SearchQuery &query, 
                     bool caseSensitive, bool pinyinEnabled);
    bool pinyinMatch(const QString &fileName, const QString &keyword);
};

DFM_SEARCH_END_NS

#endif // FILENAME_BASE_STRATEGY_H 