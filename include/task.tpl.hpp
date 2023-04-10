/***************************************************************************
 *            task.tpl.hpp
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

/*! \file task.tpl.hpp
 *  \brief Base code for a task.
 */

#ifndef PEXPLORE_TASK_TPL_HPP
#define PEXPLORE_TASK_TPL_HPP

#include "utility/container.hpp"
#include "utility/string.hpp"
#include "pronest/configuration_search_point.hpp"
#include "task_interface.hpp"
#include "task_ranking_space.hpp"


namespace pExplore {

using ProNest::ConfigurationSearchPoint;
using std::shared_ptr;

class TaskExecutionRanking;
class ConfigurationSearchSpace;

//! \brief The base for parameter search tasks
//! \details Useful to streamline task construction
template<class R>
class ParameterSearchTaskBase : public TaskInterface<R> {
  public:
    typedef TaskInput<R> InputType;
    typedef TaskOutput<R> OutputType;
  protected:
    ParameterSearchTaskBase(String const& name)
        : _name(name), _ranking_space(shared_ptr<TaskRankingSpace<R>>(TaskRankingSpaceBuilder<R>().build().clone())) {}
  public:
    String name() const override { return _name; }
    TaskRankingSpace<R> const& ranking_space() const override { return *_ranking_space; }
    void set_ranking_space(TaskRankingSpace<R> const& space) override { _ranking_space.reset(space.clone()); }

    Set<TaskExecutionRanking> rank(Map<ConfigurationSearchPoint,Pair<OutputType,DurationType>> const& data, InputType const& input) const override { return _ranking_space->rank(data, input); }

  private:
    String const _name;
    shared_ptr<TaskRankingSpace<R>> _ranking_space;
};

} // namespace pExplore

#endif // PEXPLORE_TASK_TPL_HPP
