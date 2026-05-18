// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <dfm-search/lucene++/ngramtokenizer.h>

#include <algorithm>

#include <TermAttribute.h>
#include <OffsetAttribute.h>
#include <PositionIncrementAttribute.h>
#include <AttributeSource.h>
#include <Reader.h>
#include <CharFolder.h>

namespace Lucene {

const int32_t NGramTokenizer::kIoBufferSize;

NGramTokenizer::NGramTokenizer(const ReaderPtr &input, int32_t minGram, int32_t maxGram)
    : Tokenizer(input),
      m_minGram(std::min(minGram, maxGram)),
      m_maxGram(std::max(minGram, maxGram)),
      m_isFirstTokenAtPosition(true)
{
    init();
}

NGramTokenizer::NGramTokenizer(const AttributeFactoryPtr &factory, const ReaderPtr &input,
                               int32_t minGram, int32_t maxGram)
    : Tokenizer(factory, input),
      m_minGram(std::min(minGram, maxGram)),
      m_maxGram(std::max(minGram, maxGram)),
      m_isFirstTokenAtPosition(true)
{
    init();
}

NGramTokenizer::~NGramTokenizer()
{
}

void NGramTokenizer::init()
{
    m_ioBuffer = CharArray::newInstance(kIoBufferSize);
    memset(m_ioBuffer.get(), 0, kIoBufferSize);
    m_termBuffer = CharArray::newInstance(m_maxGram);
    memset(m_termBuffer.get(), 0, m_maxGram);

    m_termAtt = addAttribute<TermAttribute>();
    m_offsetAtt = addAttribute<OffsetAttribute>();
    m_posIncrAtt = addAttribute<PositionIncrementAttribute>();
}

void NGramTokenizer::reset()
{
    Tokenizer::reset();
    m_bufferIndex = 0;
    m_ioLen = 0;
    m_inputExhausted = false;
    m_offset = 0;
    m_gramSize = m_minGram;
    m_isFirstTokenAtPosition = true;
}

// Ensure at least 'need' chars are available starting from m_bufferIndex
bool NGramTokenizer::fillBuffer(int32_t need)
{
    if (need <= 0)
        return true;

    int32_t available = m_ioLen - m_bufferIndex;
    if (available >= need)
        return true;

    if (m_inputExhausted)
        return false;

    // Compact: shift unread data to front of buffer
    if (m_bufferIndex > 0) {
        int32_t remaining = m_ioLen - m_bufferIndex;
        if (remaining > 0)
            memmove(m_ioBuffer.get(), m_ioBuffer.get() + m_bufferIndex, remaining * sizeof(wchar_t));
        m_ioLen = remaining;
        m_bufferIndex = 0;
    }

    // Read more from input
    while (m_ioLen < kIoBufferSize) {
        int32_t read = input->read(m_ioBuffer.get(), m_ioLen, kIoBufferSize - m_ioLen);
        if (read == -1) {
            m_inputExhausted = true;
            break;
        }
        m_ioLen += read;
        if (m_ioLen - m_bufferIndex >= need)
            return true;
    }

    return (m_ioLen - m_bufferIndex) >= need;
}

bool NGramTokenizer::incrementToken()
{
    clearAttributes();

    while (true) {
        // Need m_gramSize chars starting from current buffer position
        if (!fillBuffer(m_gramSize)) {
            // Not enough chars left — advance to next offset, reset gram size
            m_bufferIndex++;
            m_offset++;
            m_gramSize = m_minGram;
            m_isFirstTokenAtPosition = true;
            if (!fillBuffer(m_minGram))
                return false;
            continue;
        }

        int32_t start = m_offset;

        // Emit the n-gram at current buffer position with current gram size
        for (int32_t i = 0; i < m_gramSize; ++i) {
            m_termBuffer[i] = CharFolder::toLower(m_ioBuffer[m_bufferIndex + i]);
        }
        m_termAtt->setTermBuffer(m_termBuffer.get(), 0, m_gramSize);
        m_offsetAtt->setOffset(correctOffset(start), correctOffset(start + m_gramSize));
        m_posIncrAtt->setPositionIncrement(m_isFirstTokenAtPosition ? 1 : 0);

        // Cycle gram size: 2 → 3 → 4, then advance offset
        m_gramSize++;
        if (m_gramSize > m_maxGram) {
            m_gramSize = m_minGram;
            m_bufferIndex++;
            m_offset++;
            m_isFirstTokenAtPosition = true;
        } else {
            m_isFirstTokenAtPosition = false;
        }

        return true;
    }
}

void NGramTokenizer::end()
{
    int32_t finalOffset = correctOffset(m_offset);
    m_offsetAtt->setOffset(finalOffset, finalOffset);
}

}   // namespace Lucene
