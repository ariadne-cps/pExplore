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
#include "helper/container.hpp"
#include "task_runner.hpp"
#include "score.hpp"

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
        if (not cfg.is_singleton()) {
            auto point = cfg.search_space().initial_point();
            runner.reset(new SequentialRunner<T>(make_singleton(cfg,point)));
        } else
            runner.reset(new SequentialRunner<T>(cfg));

        runnable.set_runner(runner);
    }

    //! \brief Choose the proper runner for \a runnable
    template<class T> void choose_runner_for(TaskRunnable<T>& runnable, List<Constraint<T>> const& constraints, ConfigurationSearchPoint const& initial_point) const {
        HELPER_PRECONDITION(not constraints.empty())
        auto concurrency = BetterThreads::ThreadManager::instance().concurrency();
        std::shared_ptr<TaskRunnerInterface<T>> runner;
        auto const& cfg = runnable.configuration();
        if (concurrency > 1 and not cfg.is_singleton()) {
            runner.reset(new ParameterSearchRunner<T>(cfg,*_exploration,initial_point,std::min(concurrency,cfg.search_space().total_points())));
        } else if (not cfg.is_singleton()) {
            CONCLOG_PRINTLN_AT(1,"The configuration is not singleton: using initial point " << initial_point << " for sequential running.");
            runner.reset(new SequentialRunner<T>(make_singleton(cfg,initial_point)));
        } else
            runner.reset(new SequentialRunner<T>(cfg));

        runner->task().set_constraints(constraints);
        runnable.set_runner(runner);
    }

    void set_exploration(ExplorationInterface const& exploration);

    //! \brief The best scores saved
    List<PointScore> best_scores() const;
    void append_scores(Set<PointScore> const& scores);
    List<Set<PointScore>> const& scores() const;
    void clear_scores();

    //! \brief Print best scores in a .m file for plotting
    void print_best_scores() const;

    //! \brief Return the optimal point (i.e., the most common value for all dimensions)
    List<int> optimal_point() const;

  private:
    std::shared_ptr<ExplorationInterface> _exploration;
    std::mutex _data_mutex;
    List<Set<PointScore>> _scores;
};

} // namespace pExplore

#endif // PEXPLORE_CONCURRENCY_MANAGER_HPP
