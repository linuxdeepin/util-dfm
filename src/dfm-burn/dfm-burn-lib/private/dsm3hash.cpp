// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dsm3hash.h"

#include <QFile>

#include <openssl/evp.h>

DFM_BURN_USE_NS

static constexpr int kSM3HashLen = 32;   // SM3 produces 256-bit (32-byte) hash
static constexpr int kBufferSize = 8192;

static QString toHex(const unsigned char *data, int len)
{
    QString result;
    result.reserve(len * 2);
    static const char kHex[] = "0123456789abcdef";
    for (int i = 0; i < len; ++i) {
        result.append(kHex[(data[i] >> 4) & 0x0F]);
        result.append(kHex[data[i] & 0x0F]);
    }
    return result;
}

QString DSM3Hash::sm3File(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
        return {};

    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    if (!ctx)
        return {};

    const EVP_MD *md = EVP_sm3();
    if (!md || EVP_DigestInit_ex(ctx, md, nullptr) != 1) {
        EVP_MD_CTX_free(ctx);
        return {};
    }

    char buf[kBufferSize];
    while (!file.atEnd()) {
        qint64 n = file.read(buf, kBufferSize);
        if (n <= 0)
            break;
        if (EVP_DigestUpdate(ctx, buf, static_cast<size_t>(n)) != 1) {
            EVP_MD_CTX_free(ctx);
            return {};
        }
    }

    unsigned char hash[kSM3HashLen];
    unsigned int hashLen = 0;
    if (EVP_DigestFinal_ex(ctx, hash, &hashLen) != 1) {
        EVP_MD_CTX_free(ctx);
        return {};
    }

    EVP_MD_CTX_free(ctx);
    return toHex(hash, hashLen);
}
