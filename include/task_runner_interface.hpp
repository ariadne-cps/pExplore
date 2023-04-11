/***************************************************************************
 *            task_runner_interface.hpp
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

/*! \file task_runner_interface.hpp
 *  \brief The interfaces for generic runners.
 */

#ifndef PEXPLORE_TASK_RUNNER_INTERFACE_HPP
#define PEXPLORE_TASK_RUNNER_INTERFACE_HPP

#include "pronest/configurable.hpp"
#include "task_interface.hpp"

namespace pExplore {

using std::shared_ptr;
using ProNest::Configurable;

//! \brief Interface for the runner of a task.
//! \details Takes the runnable class as template argument.
template<class C>
class TaskRunnerInterface {
  public:
    typedef Task<C> TaskType;
    typedef TaskInput<C> InputType;
    typedef TaskOutput<C> OutputType;
    typedef Configuration<C> ConfigurationType;

    //! \brief Return the task
    virtual TaskType& task() = 0;
    virtual TaskType const& task() const = 0;

    //! \brief Return the configuration
    virtual ConfigurationType const& configuration() const = 0;

    //! \brief Push input
    virtual void push(InputType const& input) = 0;
    //! \brief Pull output from the runner
    virtual OutputType pull() = 0;
};

//! \brief Interface for a class that supports a runnable task.
template<class C>
class TaskRunnable : public Configurable<C> {
    friend class TaskManager;
    typedef Configuration<C> ConfigurationType;
  public:
    //! \brief Set the ranking cosntraint for this runnable, to rank results from multiple configurations
    void set_ranking_constraint(TaskRankingConstraint<C> const& constraint);
  protected:
    TaskRunnable(ConfigurationType const& configuration);
    //! \brief Set a new runner, useful to override the default runner
    void set_runner(shared_ptr<TaskRunnerInterface<C>> const& runner);
    //! \brief Get the runner
    TaskRunnerInterface<C>& runner();
    TaskRunnerInterface<C> const& runner() const;
  private:
    shared_ptr<TaskRunnerInterface<C>> _runner;
};

} // namespace pExplore

#endif // PEXPLORE_TASK_RUNNER_INTERFACE_HPP
