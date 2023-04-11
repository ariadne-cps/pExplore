/***************************************************************************
 *            task_manager.hpp
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

/*! \file task_manager.hpp
 *  \brief Singleton class for managing tasks across the library
 */

#ifndef PEXPLORE_CONCURRENCY_MANAGER_HPP
#define PEXPLORE_CONCURRENCY_MANAGER_HPP

#include <thread>
#include <algorithm>
#include "pronest/configuration_property_path.hpp"
#include "conclog/logging.hpp"
#include "utility/container.hpp"
#include "task_runner.hpp"
#include "task_execution_ranking.hpp"

namespace pExplore {

using ProNest::ConfigurationPropertyPath;
using ConcLog::Logger;

//! \brief Manages threads and sets runners based on concurrency availability.
class TaskManager {
  private:
    TaskManager();
  public:
    TaskManager(TaskManager const&) = delete;
    void operator=(TaskManager const&) = delete;

    static TaskManager& instance() {
        static TaskManager instance;
        return instance;
    }

    //! \brief Choose the proper runner for \a runnable
    template<class T> void choose_runner_for(TaskRunnable<T>& runnable) const {
        std::shared_ptr<TaskRunnerInterface<T>> runner;
        auto const& cfg = runnable.configuration();
        if (_concurrency > 1 and not cfg.is_singleton())
            runner.reset(new ParameterSearchRunner<T>(cfg,std::min(_concurrency,static_cast<unsigned int>(cfg.search_space().total_points()))));
        else if (_concurrency == 1 and not cfg.is_singleton()) {
            auto point = cfg.search_space().initial_point();
            CONCLOG_PRINTLN_AT(1,"The configuration is not singleton: using point " << point << " for sequential running.");
            runner.reset(new SequentialRunner<T>(make_singleton(cfg,point)));
        } else
            runner.reset(new SequentialRunner<T>(cfg));
        runnable.set_runner(runner);
    }

    unsigned int maximum_concurrency() const;
    unsigned int concurrency() const;

    void set_concurrency(unsigned int value);

    //! \brief The best points saved
    List<TaskExecutionRanking> best_rankings() const;
    void append_best_ranking(TaskExecutionRanking const& point);
    void clear_best_rankings();

    //! \brief Print best rankings in a .m file for plotting
    void print_best_rankings() const;

    //! \brief Return the optimal point (i.e., the most common value for all dimensions)
    List<int> optimal_point() const;

  private:
    unsigned int const _maximum_concurrency;
    unsigned int _concurrency;
    std::mutex _data_mutex;
    List<TaskExecutionRanking> _best_rankings;
};

} // namespace pExplore

#endif // PEXPLORE_CONCURRENCY_MANAGER_HPP
