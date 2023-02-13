// SPDX-FileCopyrightText: 2020 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "stub.h"

#include <core/denumerator.h>

#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>

#include <QUrl>

#define private public
#define protected public

USING_IO_NAMESPACE

namespace  {
    class TestDEnumerator : public testing::Test
    {
    public:
        DEnumerator *enumerator = nullptr;

        virtual void SetUp() override
        {
            std::cout << "start TestDEnumerator SetUp" << std::endl;
            
            enumerator = new DEnumerator(QUrl());

            std::cout << "end TestDEnumerator SetUp" << std::endl;
        }

        virtual void TearDown() override
        {
            std::cout << "start TestDEnumerator TearDown" << std::endl;
            
            delete enumerator;
            enumerator = nullptr;

            std::cout << "end TestDEnumerator TearDown" << std::endl;
        }
    };
}

/**
 * @brief TEST_F uri
 */
TEST_F(TestDEnumerator, getUri)
{
    EXPECT_NO_FATAL_FAILURE(enumerator->uri());
}
