/***************************************************************************
 *            task_runner.tpl.hpp
 *
 *  Copyright  2007-20  Luca Geretti
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

/*! \file task_runner.tpl.hpp
 *  \brief Implementation code for runner classes.
 */

#ifndef PEXPLORE_TASK_RUNNER_TPL_HPP
#define PEXPLORE_TASK_RUNNER_TPL_HPP

#include "pronest/configurable.tpl.hpp"
#include "utility/string.hpp"
#include "task.tpl.hpp"
#include "task_runner.hpp"
#include "task_interface.hpp"
#include "task_manager.hpp"

namespace pExplore {

using Utility::to_string;

template<class C> TaskRunnable<C>::TaskRunnable(ConfigurationType const& configuration) : Configurable<C>(configuration) {
    TaskManager::instance().choose_runner_for(*this);
}

template<class C> void TaskRunnable<C>::set_runner(shared_ptr<TaskRunnerInterface<C>> const& runner) {
    this->_runner = runner;
}

template<class C> shared_ptr<TaskRunnerInterface<C>>& TaskRunnable<C>::runner() {
    return _runner;
}

template<class C> shared_ptr<TaskRunnerInterface<C>> const& TaskRunnable<C>::runner() const {
    return _runner;
}

template<class C> class TaskRunnerBase : public TaskRunnerInterface<C> {
  public:
    typedef typename TaskRunnerInterface<C>::TaskType TaskType;
    typedef typename TaskRunnerInterface<C>::InputType InputType;
    typedef typename TaskRunnerInterface<C>::OutputType OutputType;
    typedef typename TaskRunnerInterface<C>::ConfigurationType ConfigurationType;

    TaskRunnerBase(ConfigurationType const& configuration)
        : _task(new TaskType()), _configuration(configuration) { }

    TaskType& task() override { return *_task; };
    TaskType const& task() const override { return *_task; };
    ConfigurationType const& configuration() const override { return _configuration; }

    virtual ~TaskRunnerBase() = default;

  protected:
    shared_ptr<TaskType> const _task;
    ConfigurationType const _configuration;
};

template<class C> SequentialRunner<C>::SequentialRunner(ConfigurationType const& configuration) : TaskRunnerBase<C>(configuration) { }

template<class C> void SequentialRunner<C>::push(InputType const& input) {
    OutputType result = this->_task->run(input,this->configuration());
    auto failed_constraints = this->_task->ranking_space().failed_critical_constraints(input,result);
    if (not failed_constraints.empty()) throw CriticalRankingFailureException<C>(failed_constraints);
    _last_output.reset(new OutputType(result));
}

template<class C> auto SequentialRunner<C>::pull() -> OutputType {
    return *_last_output;
}

template<class C> void DetachedRunner<C>::_loop() {
    while(true) {
        std::unique_lock<std::mutex> locker(_input_mutex);
        _input_availability.wait(locker, [this]() { return _input_buffer.size()>0 || _terminate; });
        if (_terminate) break;
        auto input = _input_buffer.pull();
        OutputType output = this->_task->run(input,this->configuration());
        _output_buffer.push(output);
        _output_availability.notify_all();
    }
}

template<class C> DetachedRunner<C>::DetachedRunner(ConfigurationType const& configuration)
        : TaskRunnerBase<C>(configuration),
          _thread([this]() { _loop(); }, this->_task->name()),
          _input_buffer(InputBufferType(1)),_output_buffer(OutputBufferType(1)),
          _last_used_input(1), _active(false), _terminate(false) { }

template<class C> DetachedRunner<C>::~DetachedRunner() {
    _terminate = true;
    _input_availability.notify_all();
}

template<class C> void DetachedRunner<C>::push(InputType const& input) {
    if (not _active) {
        _active = true;
        _thread.activate();
    }
    _input_buffer.push(input);
    _last_used_input.push(input);
    _input_availability.notify_all();
}

template<class C> auto DetachedRunner<C>::pull() -> OutputType {
    std::unique_lock<std::mutex> locker(_output_mutex);
    _output_availability.wait(locker, [this]() { return _output_buffer.size()>0; });
    auto result = _output_buffer.pull();
    auto failed_constraints = this->_task->ranking_space().failed_critical_constraints(_last_used_input.pop(),result);
    if (not failed_constraints.empty()) throw CriticalRankingFailureException<C>(failed_constraints);
    return result;
}

template<class O> class ParameterSearchOutputBufferData {
  public:
    ParameterSearchOutputBufferData(O const& output, DurationType const& execution_time, ConfigurationSearchPoint const& point) : _output(output), _execution_time(execution_time), _point(point) { }
    ParameterSearchOutputBufferData(ParameterSearchOutputBufferData<O> const& p) : _output(p._output), _execution_time(p._execution_time), _point(p._point) { }
    ParameterSearchOutputBufferData& operator=(ParameterSearchOutputBufferData<O> const& p) {
        _output = p._output;
        _execution_time = p._execution_time;
        _point = p._point;
        return *this;
    };
    O const& output() const { return _output; }
    DurationType const& execution_time() const { return _execution_time; }
    ConfigurationSearchPoint const& point() const { return _point; }
  private:
    O _output;
    DurationType _execution_time;
    ConfigurationSearchPoint _point;
};

template<class C> void ParameterSearchRunner<C>::_loop() {
    while(true) {
        std::unique_lock<std::mutex> locker(_input_mutex);
        _input_availability.wait(locker, [this]() { return _input_buffer.size()>0 || _terminate; });
        locker.unlock();
        if (_terminate) break;
        if (_input_buffer.size() == 0) std::cout << "Input buffer is empty." << std::endl;
        auto pkg = _input_buffer.pull();
        auto cfg = make_singleton(this->configuration(),pkg.second);
        try {
            auto start = std::chrono::high_resolution_clock::now();
            auto output = this->_task->run(pkg.first,cfg);
            auto end = std::chrono::high_resolution_clock::now();
            auto execution_time = std::chrono::duration_cast<DurationType>(end-start);
            CONCLOG_PRINTLN("task for " << pkg.second << " completed in " << execution_time.count() << " us");
            _output_buffer.push(OutputBufferContentType(output,execution_time,pkg.second));
        } catch (std::exception& e) {
            ++_failures;
            CONCLOG_PRINTLN("task failed: " << e.what());
        }
        _output_availability.notify_all();
    }
}

template<class C> ParameterSearchRunner<C>::ParameterSearchRunner(ConfigurationType const& configuration, unsigned int concurrency)
        : TaskRunnerBase<C>(configuration), _concurrency(concurrency),
          _failures(0), _last_used_input(1),
          _input_buffer(InputBufferType(concurrency)),_output_buffer(OutputBufferType(concurrency)),
          _active(false), _terminate(false) {
    for (unsigned int i=0; i<concurrency; ++i)
        _threads.append(shared_ptr<Thread>(new Thread([this]() { _loop(); }, this->_task->name() + (concurrency>=10 and i<10 ? "0" : "") + to_string(i))));
}

template<class C> ParameterSearchRunner<C>::~ParameterSearchRunner() {
    _terminate = true;
    _input_availability.notify_all();
}

template<class C> void ParameterSearchRunner<C>::push(InputType const& input) {
    if (not _active) {
        _active = true;
        auto shifted = this->_configuration.search_space().initial_point().make_random_shifted(_concurrency);
        for (auto point : shifted) _points.push(point);
        for (auto thread : _threads) thread->activate();
    }
    for (size_t i=0; i<_concurrency; ++i) {
        _input_buffer.push({input,_points.front()});
        _points.pop();
    }
    _last_used_input.push(input);
    _input_availability.notify_all();
}

template<class C> auto ParameterSearchRunner<C>::pull() -> OutputType {
    std::unique_lock<std::mutex> locker(_output_mutex);
    _output_availability.wait(locker, [this]() { return _output_buffer.size()>=_concurrency-_failures; });
    CONCLOG_PRINTLN("received " << _concurrency-_failures << " completed tasks");
    _failures=0;

    InputType input = _last_used_input.pull();
    Map<ConfigurationSearchPoint,Pair<OutputType,DurationType>> outputs;
    while (_output_buffer.size() > 0) {
        auto io_data = _output_buffer.pull();
        outputs.insert(Pair<ConfigurationSearchPoint,Pair<OutputType,DurationType>>(
                io_data.point(),{io_data.output(),io_data.execution_time()}));
    }
    auto rankings = this->_task->rank(outputs,input);
    CONCLOG_PRINTLN_VAR(rankings);

    Set<ConfigurationSearchPoint> new_points;
    size_t cnt = 0;
    for (auto it = rankings.rbegin(); it != rankings.rend(); ++it) {
        new_points.insert(it->point());
        ++cnt;
        if (cnt >= _concurrency/2) break;
    }
    new_points = make_extended_set_by_shifting(new_points, _concurrency);
    for (auto p : new_points) _points.push(p);
    CONCLOG_PRINTLN_VAR(new_points);

    auto best = rankings.rbegin()->point();
    if (rankings.rbegin()->critical_failures() > 0) {
        throw CriticalRankingFailureException<C>(this->_task->ranking_space().failed_critical_constraints(input, outputs.get(best).first));
    }
    TaskManager::instance().append_best_ranking(*rankings.rbegin());
    auto best_output = outputs.get(best).first;

    return best_output;
}

} // namespace pExplore

#endif // PEXPLORE_TASK_RUNNER_TPL_HPP