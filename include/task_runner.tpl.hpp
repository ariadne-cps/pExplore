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
#include "helper/string.hpp"
#include "task.tpl.hpp"
#include "task_runner.hpp"
#include "task_interface.hpp"
#include "task_manager.hpp"
#include "exploration.hpp"

namespace pExplore {

using Helper::to_string;

template<class R> class OutputPointScore {
public:
    typedef TaskOutput<R> O;
public:
    OutputPointScore(O const& output, PointScore const& point_score) : _output(output), _point_score(point_score) { }
    OutputPointScore(OutputPointScore<R> const& p) : _output(p._output), _point_score(p._point_score) { }
    OutputPointScore& operator=(OutputPointScore<R> const& p) {
        _output = p._output;
        _point_score = p._point_score;
        return *this;
    };
    O const& output() const { return _output; }
    PointScore const& point_score() const { return _point_score; }
private:
    O _output;
    PointScore _point_score;
};

template<class C> TaskRunnable<C>::TaskRunnable(ConfigurationType const& configuration) : Configurable<C>(configuration) {
    TaskManager::instance().choose_runner_for(*this);
}

template<class C> void TaskRunnable<C>::set_runner(shared_ptr<TaskRunnerInterface<C>> const& runner) {
    this->_runner = runner;
}

template<class C> void TaskRunnable<C>::set_constraints(List<Constraint<C>> const& constraints) {
    TaskManager::instance().choose_runner_for(*this,constraints,this->configuration().search_space().initial_point());
}

template<class C> void TaskRunnable<C>::set_initial_point(ConfigurationSearchPoint const& initial_point) {
    TaskManager::instance().choose_runner_for(*this,this->runner()->task().constraining_state().constraints(),initial_point);
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
        : _task({}), _configuration(configuration) { }

    TaskType& task() override { return _task; };
    TaskType const& task() const override { return _task; };
    ConfigurationType const& configuration() const override { return _configuration; }

    virtual ~TaskRunnerBase() = default;

  private:

    TaskType _task;
    ConfigurationType const _configuration;
};

template<class C> SequentialRunner<C>::SequentialRunner(ConfigurationType const& configuration) : TaskRunnerBase<C>(configuration) { }

template<class C> void SequentialRunner<C>::push(InputType const& input) {
    auto result = this->task().run(input,this->configuration());

    this->task().update_constraining_state(input,result);

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
        auto output = this->task().run(input,this->configuration());
        _output_buffer.push(output);
        _output_availability.notify_all();
    }
}

template<class C> DetachedRunner<C>::DetachedRunner(ConfigurationType const& configuration)
        : TaskRunnerBase<C>(configuration),
          _thread([this]() { _loop(); }, this->task().name(), false),
          _input_buffer({1}),_output_buffer({1}),
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

    this->task().update_constraining_state(_last_used_input,result);

    return result;
}

template<class C> void ParameterSearchRunner<C>::_loop() {
    while(true) {
        std::unique_lock<std::mutex> locker(_input_mutex);
        _input_availability.wait(locker, [this]() { return _input_buffer.size()>0 || _terminate; });
        if (_terminate) break;
        auto pkg = _input_buffer.pull();
        locker.unlock();
        auto cfg = make_singleton(this->configuration(),pkg.second);
        try {
            auto output = this->task().run(pkg.first,cfg);
            auto point_score = this->task().constraining_state().evaluate(pkg.second,pkg.first,output);
            _output_buffer.push({output,point_score});
        } catch (std::exception& e) {
            ++_failures;
            CONCLOG_PRINTLN("task failed: " << e.what());
        }
        _output_availability.notify_all();
    }
}

template<class C> ParameterSearchRunner<C>::ParameterSearchRunner(ConfigurationType const& configuration, ExplorationInterface const& exploration, ConfigurationSearchPoint const& initial_point, size_t concurrency)
        : TaskRunnerBase<C>(configuration), _concurrency(concurrency),
          _failures(0), _last_used_input({1}), _initial_point(initial_point), _points(), _exploration(exploration.clone()),
          _input_buffer({concurrency}),_output_buffer({concurrency}),
          _active(false), _terminate(false) {
    for (unsigned int i=0; i<concurrency; ++i)
        _threads.append(shared_ptr<Thread>(new Thread([this]() { _loop(); }, this->task().name() + (concurrency>=10 and i<10 ? "0" : "") + to_string(i), false)));
}

template<class C> ParameterSearchRunner<C>::~ParameterSearchRunner() {
    _terminate = true;
    _input_availability.notify_all();
}

template<class C> void ParameterSearchRunner<C>::push(InputType const& input) {
    if (not _active) {
        _active = true;
        auto shifted = _initial_point.make_random_shifted(_concurrency);
        for (auto const& point : shifted) _points.push(point);
        for (auto& thread : _threads) thread->activate();
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

    auto input = _last_used_input.pull();
    Map<ConfigurationSearchPoint,OutputType> point_outputs;
    Set<PointScore> point_scores;
    Set<ConfigurationSearchPoint> points;
    HELPER_ASSERT_EQUAL(_output_buffer.size(),_concurrency)
    while (_output_buffer.size() > 0) {
        auto data = _output_buffer.pull();
        point_scores.insert(data.point_score());
        points.insert(data.point_score().point());
        point_outputs.insert(Pair<ConfigurationSearchPoint,OutputType>(data.point_score().point(),data.output()));
    }
    HELPER_ASSERT_EQUAL(points.size(),_concurrency)
    HELPER_ASSERT_EQUAL(point_scores.size(),_concurrency)

    auto new_points = _exploration->next_points_from(point_scores);
    for (auto const& p : new_points) _points.push(p);
    CONCLOG_PRINTLN_VAR(new_points);

    HELPER_ASSERT_EQUAL(_points.size(),_concurrency)

    auto best_point_score = *point_scores.begin();
    auto best_output = point_outputs.get(best_point_score.point());

    this->task().update_constraining_state(input,best_output);

    if (this->task().constraining_state().has_no_active_constraints())
        throw new NoActiveConstraintsException(this->task().constraining_state().states());

    TaskManager::instance().append_scores(point_scores);

    return best_output;
}

} // namespace pExplore

#endif // PEXPLORE_TASK_RUNNER_TPL_HPP
