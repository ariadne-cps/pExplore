/***************************************************************************
 *            test_task_manager.cpp
 *
 *  Copyright  2023  Luca Geretti
 *
 ****************************************************************************/

/*
 * This file is part of pExplore, under the MIT license.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "utility/test.hpp"
#include "task_manager.hpp"

using namespace pExplore;

class TestTaskManager {
  public:

    void test_check_nonzero_maximum_concurrency() {
        UTILITY_TEST_ASSERT(TaskManager::instance().maximum_concurrency()>0);
    }

    void test_set_concurrency() {
        auto max_concurrency = TaskManager::instance().maximum_concurrency();
        TaskManager::instance().set_concurrency(max_concurrency);
        UTILITY_TEST_EQUALS(TaskManager::instance().concurrency(),max_concurrency);
        UTILITY_TEST_FAIL(TaskManager::instance().set_concurrency(0));
        UTILITY_TEST_FAIL(TaskManager::instance().set_concurrency(1+max_concurrency));
    }

    void test() {
        UTILITY_TEST_CALL(test_check_nonzero_maximum_concurrency());
        UTILITY_TEST_CALL(test_set_concurrency());
    }
};

int main() {
    TestTaskManager().test();
    return UTILITY_TEST_FAILURES;
}
