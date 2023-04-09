/***************************************************************************
 *            test_task_search_parameter.cpp
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
#include "pronest/configuration_search_point.hpp"
#include "pronest/configuration_search_space.hpp"
#include "task_execution_ranking.hpp"

using namespace pExplore;
using namespace ProNest;

class TestTaskSearchParameter {
  public:

    void test_task_parameter_creation() {
        ConfigurationSearchParameter p(ConfigurationPropertyPath("use_subdivisions"), false, List<int>({0, 1}));
        UTILITY_TEST_PRINT(p);
    }

    void test_task_parameter_randomise() {
        ConfigurationSearchParameter p(ConfigurationPropertyPath("use_subdivisions"), false, List<int>({0, 1}));
        UTILITY_TEST_PRINT(p);
        List<int> values;
        for (unsigned int i=0; i<16; ++i) values.push_back(p.random_value());
        UTILITY_TEST_PRINT(values);
    }

    void test_metric_task_parameter_shift() {
        ConfigurationSearchParameter metric(ConfigurationPropertyPath("sweep_threshold"), true, List<int>({8, 9, 10, 11}));
        UTILITY_TEST_EQUALS(metric.shifted_value_from(8),9);
        UTILITY_TEST_EQUALS(metric.shifted_value_from(11),10);
        auto from_1 = metric.shifted_value_from(10);
        UTILITY_TEST_ASSERT(from_1 == 9 or from_1 == 11);
    }

    void test_parameter_space() {
        ConfigurationPropertyPath use_subdivisions("use_subdivisions");
        ConfigurationPropertyPath sweep_threshold("sweep_threshold");
        ConfigurationSearchParameter bp(use_subdivisions, false, List<int>({0, 1}));
        ConfigurationSearchParameter mp(sweep_threshold, true, List<int>({3, 4, 5}));
        ConfigurationSearchSpace space({bp, mp});
        UTILITY_TEST_PRINT(space);
        UTILITY_TEST_PRINT(space.parameters());
        UTILITY_TEST_EQUALS(space.dimension(),2);
        UTILITY_TEST_EQUALS(space.index(bp),1);
        UTILITY_TEST_EQUALS(space.index(mp),0);
        UTILITY_TEST_EQUALS(space.total_points(),6);
    }

    void test_parameter_point_creation() {
        ConfigurationPropertyPath use_subdivisions("use_subdivisions");
        ConfigurationPropertyPath sweep_threshold("sweep_threshold");
        ConfigurationSearchParameter bp(use_subdivisions, false, List<int>({0, 1}));
        ConfigurationSearchParameter mp(sweep_threshold, true, List<int>({3, 4, 5}));
        ConfigurationSearchSpace space({bp, mp});
        UTILITY_TEST_PRINT(space.initial_point());
        Map<ConfigurationPropertyPath,int> bindings = {{use_subdivisions,1},{sweep_threshold,5}};
        UTILITY_TEST_PRINT(bindings);
        ConfigurationSearchPoint point = space.make_point(bindings);
        UTILITY_TEST_PRINT(point);
        UTILITY_TEST_PRINT(point.space());
    }

    void test_parameter_point_equality() {
        ConfigurationPropertyPath use_subdivisions("use_subdivisions");
        ConfigurationPropertyPath sweep_threshold("sweep_threshold");
        ConfigurationSearchParameter bp(use_subdivisions, false, List<int>({0, 1}));
        ConfigurationSearchParameter mp(sweep_threshold, true, List<int>({3, 4, 5}));
        ConfigurationSearchSpace space({bp, mp});

        ConfigurationSearchPoint point1 = space.make_point({{use_subdivisions, 1}, {sweep_threshold, 2}});
        ConfigurationSearchPoint point2 = space.make_point({{use_subdivisions, 1}, {sweep_threshold, 2}});
        ConfigurationSearchPoint point3 = space.make_point({{use_subdivisions, 1}, {sweep_threshold, 3}});
        UTILITY_TEST_EQUAL(point1,point2);
        UTILITY_TEST_NOT_EQUAL(point1,point3);
    }

    void test_parameter_point_distance() {
        ConfigurationPropertyPath use_subdivisions("use_subdivisions");
        ConfigurationPropertyPath sweep_threshold("sweep_threshold");
        ConfigurationSearchParameter bp(use_subdivisions, false, List<int>({0, 1}));
        ConfigurationSearchParameter mp(sweep_threshold, true, List<int>({3, 4, 5}));
        ConfigurationSearchSpace space({bp, mp});

        ConfigurationSearchPoint point1 = space.make_point({{use_subdivisions, 1}, {sweep_threshold, 2}});
        ConfigurationSearchPoint point2 = space.make_point({{use_subdivisions, 1}, {sweep_threshold, 2}});
        ConfigurationSearchPoint point3 = space.make_point({{use_subdivisions, 1}, {sweep_threshold, 3}});
        ConfigurationSearchPoint point4 = space.make_point({{use_subdivisions, 0}, {sweep_threshold, 5}});
        UTILITY_TEST_EQUALS(point1.distance(point2),0);
        UTILITY_TEST_EQUALS(point1.distance(point3),1);
        UTILITY_TEST_EQUALS(point3.distance(point4),3);
    }

    void test_parameter_point_adjacent_shift() {
        ConfigurationPropertyPath use_subdivisions("use_subdivisions");
        ConfigurationPropertyPath sweep_threshold("sweep_threshold");
        ConfigurationSearchParameter bp(use_subdivisions, false, List<int>({0, 1}));
        ConfigurationSearchParameter mp(sweep_threshold, true, List<int>({3, 4, 5}));
        ConfigurationSearchSpace space({bp, mp});

        ConfigurationSearchPoint point1 = space.make_point({{use_subdivisions, 1}, {sweep_threshold, 5}});
        UTILITY_TEST_PRINT(point1);
        auto point2 = point1.make_adjacent_shifted();
        UTILITY_TEST_PRINT(point2);
        UTILITY_TEST_NOT_EQUAL(point1,point2);
    }

    void test_parameter_point_random_shift() {
        ConfigurationPropertyPath use_subdivisions("use_subdivisions");
        ConfigurationPropertyPath sweep_threshold("sweep_threshold");
        ConfigurationSearchParameter bp(use_subdivisions, false, List<int>({0, 1}));
        ConfigurationSearchParameter mp(sweep_threshold, true, List<int>({3, 4, 5, 6, 7}));
        ConfigurationSearchSpace space({bp, mp});

        ConfigurationSearchPoint point = space.make_point({{use_subdivisions, 1}, {sweep_threshold, 5}});
        auto points = point.make_random_shifted(1);
        UTILITY_TEST_EQUALS(points.size(),1);
        points = point.make_random_shifted(3);
        UTILITY_TEST_EQUALS(points.size(),3);
        points = point.make_random_shifted(space.total_points());
        UTILITY_TEST_EQUALS(points.size(),space.total_points());
    }

    void test_parameter_point_adjacent_set_shift() {
        ConfigurationPropertyPath use_subdivisions("use_subdivisions");
        ConfigurationPropertyPath sweep_threshold("sweep_threshold");
        ConfigurationSearchParameter bp(use_subdivisions, false, List<int>({0, 1}));
        ConfigurationSearchParameter mp(sweep_threshold, true, List<int>({3, 4, 5, 6, 7, 8}));
        ConfigurationSearchSpace space({bp, mp});
        UTILITY_TEST_PRINT(space.total_points());

        ConfigurationSearchPoint point = space.make_point({{use_subdivisions, 1}, {sweep_threshold, 5}});
        Set<ConfigurationSearchPoint> points = point.make_random_shifted(3);
        UTILITY_TEST_PRINT(points);
        auto all_points = make_extended_set_by_shifting(points, 5);
        UTILITY_TEST_EQUALS(all_points.size(),5);
        UTILITY_TEST_PRINT(all_points);

        ConfigurationSearchPoint point1 = space.make_point({{use_subdivisions, 1}, {sweep_threshold, 8}});
        ConfigurationSearchPoint point2 = space.make_point({{use_subdivisions, 0}, {sweep_threshold, 3}});
        Set<ConfigurationSearchPoint> border_points = {point1, point2};
        UTILITY_TEST_PRINT(border_points);
        UTILITY_PRINT_TEST_COMMENT("Checking maximum number of single shift points including the original border points");
        auto six_points = make_extended_set_by_shifting(border_points, 6);
        UTILITY_TEST_PRINT(six_points);
        UTILITY_PRINT_TEST_COMMENT("Checking 1 point over the number of possible adjacent shiftings");
        auto seven_points = make_extended_set_by_shifting(border_points, 7);
        UTILITY_TEST_PRINT(seven_points);
        UTILITY_PRINT_TEST_COMMENT("Checking up to the maximum number");
        auto twelve_points = make_extended_set_by_shifting(border_points, 12);
        UTILITY_TEST_PRINT(twelve_points);
        UTILITY_PRINT_TEST_COMMENT("Checking 1 point over the maximum number");
        UTILITY_TEST_FAIL(make_extended_set_by_shifting(points, point.space().total_points()+1));
    }

    void test_parameter_point_ranking() {
        ConfigurationPropertyPath use_subdivisions("use_subdivisions");
        ConfigurationPropertyPath sweep_threshold("sweep_threshold");
        ConfigurationSearchParameter bp(use_subdivisions, false, List<int>({0, 1}));
        ConfigurationSearchParameter mp(sweep_threshold, true, List<int>({3, 4, 5, 6, 7}));
        ConfigurationSearchSpace space({bp, mp});

        ConfigurationSearchPoint point1 = space.make_point({{use_subdivisions, 1}, {sweep_threshold, 2}});
        ConfigurationSearchPoint point2 = space.make_point({{use_subdivisions, 1}, {sweep_threshold, 2}});
        ConfigurationSearchPoint point3 = space.make_point({{use_subdivisions, 1}, {sweep_threshold, 3}});
        ConfigurationSearchPoint point4 = space.make_point({{use_subdivisions, 0}, {sweep_threshold, 4}});
        TaskExecutionRanking a1(point1, 2, 0, 0);
        TaskExecutionRanking a2(point2, 4, 1, 0);
        TaskExecutionRanking a3(point3, 4, 0, 0);
        TaskExecutionRanking a4(point4, 4, 0, 1);
        Set<TaskExecutionRanking> as = {a1, a2, a3, a4};

        UTILITY_TEST_PRINT(as);
        UTILITY_TEST_ASSERT(a2 < a1);
        UTILITY_TEST_ASSERT(a1 < a3);
        UTILITY_TEST_ASSERT(a2 < a3);
        UTILITY_TEST_ASSERT(a4 < a2);
        UTILITY_TEST_ASSERT(a4 < a3);
    }

    void test() {
        UTILITY_TEST_CALL(test_task_parameter_creation());
        UTILITY_TEST_CALL(test_task_parameter_randomise());
        UTILITY_TEST_CALL(test_metric_task_parameter_shift());
        UTILITY_TEST_CALL(test_parameter_space());
        UTILITY_TEST_CALL(test_parameter_point_creation());
        UTILITY_TEST_CALL(test_parameter_point_equality());
        UTILITY_TEST_CALL(test_parameter_point_distance());
        UTILITY_TEST_CALL(test_parameter_point_adjacent_shift());
        UTILITY_TEST_CALL(test_parameter_point_random_shift());
        UTILITY_TEST_CALL(test_parameter_point_adjacent_set_shift());
        UTILITY_TEST_CALL(test_parameter_point_ranking());
    }
};

int main() {
    TestTaskSearchParameter().test();
    return UTILITY_TEST_FAILURES;
}
