// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef NGRAMANALYZER_H
#define NGRAMANALYZER_H

#include <lucene++/LuceneHeaders.h>

namespace Lucene {

class NGramAnalyzer : public Analyzer
{
public:
    explicit NGramAnalyzer(int32_t minGram, int32_t maxGram);
    virtual ~NGramAnalyzer();

    LUCENE_CLASS(NGramAnalyzer);

public:
    virtual TokenStreamPtr tokenStream(const String &fieldName, const ReaderPtr &reader);
    virtual TokenStreamPtr reusableTokenStream(const String &fieldName, const ReaderPtr &reader);

private:
    int32_t m_minGram;
    int32_t m_maxGram;
};

}   // namespace Lucene

#endif   // NGRAMANALYZER_H
