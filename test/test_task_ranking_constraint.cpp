/***************************************************************************
 *            test_task_ranking_constraint.cpp
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
#include "utility/array.hpp"
#include "task_ranking_constraint.hpp"
#include "task_runner_interface.hpp"

using namespace pExplore;
using namespace Utility;

class TestRunnable : public TaskRunnable<TestRunnable> { };
typedef TestRunnable R;

namespace pExplore {
template<> struct TaskInput<R> {
    TaskInput(int i1_, Array<int> i2_) : i1(i1_), i2(i2_) { }
    int i1;
    Array<int> i2;
};
template<> struct TaskOutput<R> {
    TaskOutput(int o_) : o(o_) { }
    int o;
};
}

typedef TaskInput<R> I;
typedef TaskOutput<R> O;

class TestTaskRankingConstraint {
  public:

    void test_scalar_ranking_parameter_creation() {
        TaskRankingConstraint<R> c("chosen_step_size", OptimisationCriterion::MAXIMISE, RankingConstraintSeverity::PERMISSIVE,
                                    [](I const& input, O const& output) { return static_cast<double>(output.o + input.i1); });
        auto input = I(2,{1,2});
        auto output = O(7);
        auto cost = c.rank(input, output);
        UTILITY_TEST_PRINT(c);
        UTILITY_TEST_EQUALS(cost,9);
        UTILITY_TEST_EQUALS(c.optimisation(), OptimisationCriterion::MAXIMISE);
        UTILITY_TEST_EQUALS(c.severity(), RankingConstraintSeverity::PERMISSIVE);
    }

    void test() {
        UTILITY_TEST_CALL(test_scalar_ranking_parameter_creation())
    }
};

int main() {
    TestTaskRankingConstraint().test();
    return UTILITY_TEST_FAILURES;
}
