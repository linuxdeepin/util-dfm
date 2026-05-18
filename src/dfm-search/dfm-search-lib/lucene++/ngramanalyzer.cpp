// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <dfm-search/lucene++/ngramanalyzer.h>
#include <dfm-search/lucene++/ngramtokenizer.h>

namespace Lucene {

NGramAnalyzer::NGramAnalyzer(int32_t minGram, int32_t maxGram)
    : m_minGram(minGram), m_maxGram(maxGram)
{
}

NGramAnalyzer::~NGramAnalyzer()
{
}

TokenStreamPtr NGramAnalyzer::tokenStream(const String &fieldName, const ReaderPtr &reader)
{
    return newLucene<NGramTokenizer>(reader, m_minGram, m_maxGram);
}

TokenStreamPtr NGramAnalyzer::reusableTokenStream(const String &fieldName, const ReaderPtr &reader)
{
    LuceneObjectPtr prev = getPreviousTokenStream();
    TokenizerPtr saved(boost::dynamic_pointer_cast<Tokenizer>(prev));
    if (!saved) {
        saved = newLucene<NGramTokenizer>(reader, m_minGram, m_maxGram);
        setPreviousTokenStream(saved);
    } else {
        saved->reset(reader);
    }
    return saved;
}

}   // namespace Lucene
