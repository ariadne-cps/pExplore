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
#include "utility/macros.hpp"
#include "task_manager.hpp"

namespace pExplore {

using std::make_pair;

TaskManager::TaskManager() : _maximum_concurrency(std::thread::hardware_concurrency()), _concurrency(1), _exploration(new ShiftAndKeepBestHalfExploration()) {}

unsigned int TaskManager::maximum_concurrency() const {
    return _maximum_concurrency;
}

unsigned int TaskManager::concurrency() const {
    return _concurrency;
}

void TaskManager::set_concurrency(unsigned int value) {
    UTILITY_PRECONDITION(value <= _maximum_concurrency and value > 0);
    std::lock_guard<std::mutex> lock(_data_mutex);
    _concurrency = value;
}

void TaskManager::set_exploration(ExplorationInterface const& exploration) {
    _exploration.reset(exploration.clone());
}

List<PointRanking> TaskManager::best_rankings() const {
    return _best_rankings;
}

void TaskManager::append_best_ranking(PointRanking const& ranking) {
    std::lock_guard<std::mutex> lock(_data_mutex);
    _best_rankings.push_back(ranking);
}

void TaskManager::clear_best_rankings() {
    _best_rankings.clear();
}

List<int> TaskManager::optimal_point() const {
    List<int> result;
    if (not _best_rankings.empty()) {
        auto space = _best_rankings.front().point().space();
        auto dimension = space.dimension();

        List<Map<int,size_t>> frequencies;
        for (size_t i=0; i<dimension; ++i) frequencies.push_back(Map<int,size_t>());
        for (auto ranking : _best_rankings) {
            auto coordinates = ranking.point().coordinates();
            for (size_t i=0; i<dimension; ++i) {
                auto iter = frequencies[i].find(coordinates[i]);
                if (iter == frequencies[i].end()) frequencies[i].insert(make_pair(coordinates[i],1));
                else frequencies[i].at(coordinates[i])++;
            }
        }

        for (size_t i=0; i<dimension; ++i) {
            auto freq_it = frequencies[i].begin();
            int best_value = freq_it->first;
            size_t best_frequency = freq_it->second;
            ++freq_it;
            while(freq_it != frequencies[i].end()) {
                if (freq_it->second >best_frequency) {
                    best_value = freq_it->first;
                    best_frequency = freq_it->second;
                }
                ++freq_it;
            }
            result.push_back(best_value);
        }
    }
    return result;
}

void TaskManager::print_best_rankings() const {
    if (not _best_rankings.empty()) {
        std::ofstream file;
        file.open("points.m");
        auto space = _best_rankings.front().point().space();
        auto dimension = space.dimension();
        auto size = _best_rankings.size();
        file << "x = [1:" << size << "];\n";
        Map<size_t,List<int>> values;
        List<size_t> soft_failures;
        for (size_t i=0; i<dimension; ++i) values.insert(make_pair(i,List<int>()));
        for (auto ranking : _best_rankings) {
            auto point = ranking.point();
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