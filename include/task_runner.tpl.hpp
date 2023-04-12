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
#include "exploration.hpp"

namespace pExplore {

using Utility::to_string;

template<class C> TaskRunnable<C>::TaskRunnable(ConfigurationType const& configuration) : Configurable<C>(configuration) {
    TaskManager::instance().choose_runner_for(*this);
}

template<class C> void TaskRunnable<C>::set_runner(shared_ptr<TaskRunnerInterface<C>> const& runner) {
    this->_runner = runner;
}

template<class C> void TaskRunnable<C>::set_ranking_constraint(TaskRankingConstraint<C> const& constraint) {
    _runner->task().set_ranking_constraint(constraint);
}

template<class C> TaskRunnerInterface<C>& TaskRunnable<C>::runner() {
    return *_runner;
}

template<class C> TaskRunnerInterface<C> const& TaskRunnable<C>::runner() const {
    return *_runner;
}

template<class C> class TaskRunnerBase : public TaskRunnerInterface<C> {
  public:
    typedef typename TaskRunnerInterface<C>::TaskType TaskType;
    typedef typename TaskRunnerInterface<C>::InputType InputType;
    typedef typename TaskRunnerInterface<C>::OutputType OutputType;
    typedef typename TaskRunnerInterface<C>::ConfigurationType ConfigurationType;

    TaskRunnerBase(ConfigurationType const& configuration)
        : _task(TaskType()), _configuration(configuration) { }

    TaskType& task() override { return _task; };
    TaskType const& task() const override { return _task; };
    ConfigurationType const& configuration() const override { return _configuration; }

    virtual ~TaskRunnerBase() = default;

  protected:
    TaskType _task;
    ConfigurationType const _configuration;
};

template<class C> SequentialRunner<C>::SequentialRunner(ConfigurationType const& configuration) : TaskRunnerBase<C>(configuration) { }

template<class C> void SequentialRunner<C>::push(InputType const& input) {
    OutputType result = this->_task.run(input,this->configuration());
    auto const& constraint = this->_task.ranking_constraint();
    auto score = constraint.rank(input,result);
    if (constraint.severity() == ConstraintSeverity::CRITICAL) {
        if ((constraint.criterion() == RankingCriterion::MAXIMISE and score < 0) or (constraint.criterion() == RankingCriterion::MINIMISE_POSITIVE and score > 0)) {
            throw CriticalRankingFailureException<C>(score);
        }
    }
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
        OutputType output = this->_task.run(input,this->configuration());
        _output_buffer.push(output);
        _output_availability.notify_all();
    }
}

template<class C> DetachedRunner<C>::DetachedRunner(ConfigurationType const& configuration)
        : TaskRunnerBase<C>(configuration),
          _thread([this]() { _loop(); }, this->_task.name(), false),
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
    auto const& constraint = this->_task.ranking_constraint();
    auto score = constraint.rank(_last_used_input.pull(),result);
    if (constraint.severity() == ConstraintSeverity::CRITICAL) {
        if ((constraint.criterion() == RankingCriterion::MAXIMISE and score < 0) or (constraint.criterion() == RankingCriterion::MINIMISE_POSITIVE and score > 0)) {
            throw CriticalRankingFailureException<C>(score);
        }
    }
    return result;
}

template<class O> class ParameterSearchOutputBufferData {
  public:
    ParameterSearchOutputBufferData(O const& output, ConfigurationSearchPoint const& point) : _output(output), _point(point) { }
    ParameterSearchOutputBufferData(ParameterSearchOutputBufferData<O> const& p) : _output(p._output), _point(p._point) { }
    ParameterSearchOutputBufferData& operator=(ParameterSearchOutputBufferData<O> const& p) {
        _output = p._output;
        _point = p._point;
        return *this;
    };
    O const& output() const { return _output; }
    ConfigurationSearchPoint const& point() const { return _point; }
  private:
    O _output;
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
            auto output = this->_task.run(pkg.first,cfg);
            _output_buffer.push(OutputBufferContentType(output,pkg.second));
        } catch (std::exception& e) {
            ++_failures;
            CONCLOG_PRINTLN("task failed: " << e.what());
        }
        _output_availability.notify_all();
    }
}

template<class C> ParameterSearchRunner<C>::ParameterSearchRunner(ConfigurationType const& configuration, ExplorationInterface const& exploration, unsigned int concurrency)
        : TaskRunnerBase<C>(configuration), _concurrency(concurrency),
          _failures(0), _last_used_input(1), _points(), _exploration(exploration.clone()),
          _input_buffer(InputBufferType(concurrency)),_output_buffer(OutputBufferType(concurrency)),
          _active(false), _terminate(false) {
    for (unsigned int i=0; i<concurrency; ++i)
        _threads.append(shared_ptr<Thread>(new Thread([this]() { _loop(); }, this->_task.name() + (concurrency>=10 and i<10 ? "0" : "") + to_string(i), false)));
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
    Map<ConfigurationSearchPoint,OutputType> outputs;
    while (_output_buffer.size() > 0) {
        auto io_data = _output_buffer.pull();
        outputs.insert(Pair<ConfigurationSearchPoint,OutputType>(io_data.point(),io_data.output()));
    }
    auto rankings = this->_task.rank(outputs,input);
    CONCLOG_PRINTLN_VAR(rankings);

    Set<ConfigurationSearchPoint> new_points = _exploration->next_points_from(rankings);
    for (auto p : new_points) _points.push(p);
    CONCLOG_PRINTLN_VAR(new_points);

    auto best = *rankings.rbegin();

    auto const& constraint = this->_task.ranking_constraint();
    if (constraint.severity() == ConstraintSeverity::CRITICAL) {
        if ((constraint.criterion() == RankingCriterion::MAXIMISE and best.score() < 0) or (constraint.criterion() == RankingCriterion::MINIMISE_POSITIVE and best.score() > 0)) {
            throw CriticalRankingFailureException<C>(best.score());
        }
    }
    TaskManager::instance().append_best_ranking(best);

    return outputs.get(best.point());
}

} // namespace pExplore

#endif // PEXPLORE_TASK_RUNNER_TPL_HPP
