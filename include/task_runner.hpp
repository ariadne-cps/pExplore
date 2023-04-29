/***************************************************************************
 *            task_runner.hpp
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

/*! \file task_runner.hpp
 *  \brief Runner classes.
 */

#ifndef PEXPLORE_TASK_RUNNER_HPP
#define PEXPLORE_TASK_RUNNER_HPP

#include "betterthreads/thread.hpp"
#include "betterthreads/buffer.hpp"
#include "pronest/configuration_search_point.hpp"
#include "pronest/configuration_search_space.hpp"
#include "pronest/configuration_property.tpl.hpp"
#include "helper/container.hpp"
#include "task_runner_interface.hpp"
#include "score.hpp"
#include "exploration.hpp"

namespace pExplore {

using BetterThreads::Buffer;
using BetterThreads::Thread;
using Helper::List;
using std::shared_ptr;


template<class C> class TaskRunnerBase;

//! \brief Run a task sequentially.
//! \details Used to provide a sequential alternative to any thread-based implementation.
template<class C>
class SequentialRunner final : public TaskRunnerBase<C> {
    friend class TaskManager;
    typedef typename TaskRunnerBase<C>::InputType InputType;
    typedef typename TaskRunnerBase<C>::OutputType OutputType;
    typedef typename TaskRunnerBase<C>::ConfigurationType ConfigurationType;
  protected:
    SequentialRunner(ConfigurationType const& configuration);
  public:
    void push(InputType const& input) override final;
    OutputType pull() override final;

private:
    shared_ptr<OutputType> _last_output;
};

//! \brief Run a task in a detached thread, allowing other processing between pushing and pulling.
template<class C>
class DetachedRunner final : public TaskRunnerBase<C> {
    friend class TaskManager;
    typedef typename TaskRunnerBase<C>::InputType InputType;
    typedef typename TaskRunnerBase<C>::OutputType OutputType;
    typedef typename TaskRunnerBase<C>::ConfigurationType ConfigurationType;
    typedef Buffer<InputType> InputBufferType;
    typedef Buffer<OutputType> OutputBufferType;
  protected:
    DetachedRunner(ConfigurationType const& configuration);
  public:
    virtual ~DetachedRunner();

    void push(InputType const& input) override final;
    OutputType pull() override final;

private:
    void _loop();
private:
    Thread _thread;
    InputBufferType _input_buffer;
    OutputBufferType _output_buffer;
    Buffer<InputType> _last_used_input;
    std::atomic<bool> _active;
    std::atomic<bool> _terminate;
    std::mutex _input_mutex;
    std::condition_variable _input_availability;
    std::mutex _output_mutex;
    std::condition_variable _output_availability;
};

template<class O> class OutputPointScore;

//! \brief Run a task by detached concurrent search into the parameter space.
template<class C> class ParameterSearchRunner final : public TaskRunnerBase<C> {
    friend class TaskManager;
    typedef typename TaskRunnerBase<C>::InputType InputType;
    typedef typename TaskRunnerBase<C>::OutputType OutputType;
    typedef typename TaskRunnerBase<C>::ConfigurationType ConfigurationType;
    typedef Pair<InputType,ConfigurationSearchPoint> InputBufferContentType;
    typedef OutputPointScore<C> OutputBufferContentType;
    typedef Buffer<InputBufferContentType> InputBufferType;
    typedef Buffer<OutputBufferContentType> OutputBufferType;
  protected:
    ParameterSearchRunner(ConfigurationType const& configuration, ExplorationInterface const& exploration, size_t concurrency);
  public:
    virtual ~ParameterSearchRunner();

    void push(InputType const& input) override final;
    OutputType pull() override final;

private:
    void _loop();
private:
    size_t const _concurrency; // Number of threads to be used
    std::atomic<unsigned int> _failures; // Number of task failures after a given push, reset during pulling
    Buffer<InputType> _last_used_input;
    std::queue<ConfigurationSearchPoint> _points;
    std::shared_ptr<ExplorationInterface> _exploration;
    // Synchronization
    List<shared_ptr<Thread>> _threads;
    InputBufferType _input_buffer;
    OutputBufferType _output_buffer;
    std::atomic<bool> _active;
    std::atomic<bool> _terminate;
    std::mutex _input_mutex;
    std::condition_variable _input_availability;
    std::mutex _output_mutex;
    std::condition_variable _output_availability;
};

} // namespace pExplore

#endif // PEXPLORE_TASK_RUNNER_HPP
