// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef NGRAMTOKENIZER_H
#define NGRAMTOKENIZER_H

#include <lucene++/LuceneHeaders.h>

namespace Lucene {

class NGramTokenizer : public Tokenizer
{
public:
    NGramTokenizer(const ReaderPtr &input, int32_t minGram, int32_t maxGram);
    NGramTokenizer(const AttributeFactoryPtr &factory, const ReaderPtr &input,
                   int32_t minGram, int32_t maxGram);

    virtual ~NGramTokenizer();

    LUCENE_CLASS(NGramTokenizer);

public:
    virtual bool incrementToken();
    virtual void end();

    virtual void reset();
    virtual void reset(const ReaderPtr &input);

private:
    static int32_t normalizeGramSize(int32_t gramSize);

    void init();
    void resetState();
    bool fillBuffer(int32_t need);

    int32_t m_minGram;
    int32_t m_maxGram;

    static const int32_t kIoBufferSize = 1024;

    CharArray m_ioBuffer;
    int32_t m_ioLen;
    int32_t m_bufferIndex;
    bool m_inputExhausted;

    int32_t m_offset;   // current position in the logical input stream
    int32_t m_gramSize;   // current n-gram size being emitted
    CharArray m_termBuffer;

    TermAttributePtr m_termAtt;
    OffsetAttributePtr m_offsetAtt;
    PositionIncrementAttributePtr m_posIncrAtt;
    bool m_isFirstTokenAtPosition;   // true = positionIncrement=1, false = 0 (same position)
};

}   // namespace Lucene

#endif   // NGRAMTOKENIZER_H
