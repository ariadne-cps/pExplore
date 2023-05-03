/***************************************************************************
 *            task_manager.cpp
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

#include "conclog/logging.hpp"
#include "helper/macros.hpp"
#include "betterthreads/thread_manager.hpp"
#include "task_manager.hpp"

namespace pExplore {

using std::make_pair;

TaskManager::TaskManager() : _exploration(new ShiftAndKeepBestHalfExploration()) {}

void TaskManager::set_exploration(ExplorationInterface const& exploration) {
    _exploration.reset(exploration.clone());
}

List<Set<PointScore>> const& TaskManager::scores() const {
    return _scores;
}

List<PointScore> TaskManager::best_scores() const {
    List<PointScore> result;
    for (auto const& e : _scores)
        result.append(*e.begin());
    return result;
}

void TaskManager::append_scores(Set<PointScore> const& scores) {
    std::lock_guard<std::mutex> lock(_data_mutex);
    _scores.push_back(scores);
}

void TaskManager::clear_scores() {
    _scores.clear();
}

List<int> TaskManager::optimal_point() const {
    List<int> result;
    auto best = best_scores();
    if (not best.empty()) {
        auto space = best.front().point().space();
        auto dimension = space.dimension();

        Map<size_t,double> sums;
        for (size_t i=0; i<dimension; ++i) sums.insert(i,0.0);
        for (auto const& s : best) {
            auto coordinates = s.point().coordinates();
            for (size_t i=0; i<dimension; ++i) {
                sums[i] += static_cast<double>(coordinates[i]);
            }
        }

        for (size_t i=0; i<dimension; ++i) {
            result.push_back(static_cast<int>(round(sums[i]/best.size())));
        }
    }
    return result;
}

void TaskManager::print_best_scores() const {
    auto best = best_scores();
    if (not best.empty()) {
        std::ofstream file;
        file.open("points.m");
        auto space = best.front().point().space();
        auto dimension = space.dimension();
        auto size = best.size();
        file << "x = [1:" << size << "];\n";
        Map<size_t,List<int>> values;
        List<size_t> soft_failures;
        for (size_t i=0; i<dimension; ++i) values.insert(make_pair(i,List<int>()));
        for (auto const& ranking : best) {
            auto const& point = ranking.point();
            for (size_t i=0; i<dimension; ++i) values.at(i).push_back(point.coordinates()[i]);
        }
        file << "figure(1);\n";
        file << "hold on;\n";
        for (size_t i=0; i<dimension; ++i) {
            file << "y" << i << " = " << values.at(i) << ";\n";
            auto name = space.parameters()[i].path().last();
            std::replace(name.begin(), name.end(), '_', ' ');
            file << "plot(x,y" << i << ",'DisplayName','" << name << "');\n";
        }

        file << "legend;\n";
        file << "hold off;\n";
        file.close();
    }
}

}