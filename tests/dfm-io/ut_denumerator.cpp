/*
 * Copyright (C) 2020 ~ 2021 Uniontech Software Technology Co., Ltd.
 *
 * Author:     lanxuesong<luzhen@uniontech.com>
 *
 * Maintainer: lanxuesong<zhengyouge@uniontech.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

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
