/***************************************************************************
 *            task_interface.hpp
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

/*! \file task_interface.hpp
 *  \brief The interface for tasks.
 */

#ifndef PEXPLORE_TASK_INTERFACE_HPP
#define PEXPLORE_TASK_INTERFACE_HPP

#include <chrono>
#include "utility/container.hpp"
#include "utility/tuple.hpp"
#include "utility/string.hpp"
#include "pronest/configuration_search_point.hpp"

namespace pExplore {

using Utility::String;
using Utility::Set;
using Utility::Pair;
using Utility::Map;
using Utility::List;
using ProNest::ConfigurationSearchPoint;
using ProNest::Configuration;

class PointRanking;
template<class R> class ConstraintSet;

template<class R> struct TaskInput;
template<class R> struct TaskOutput;
template<class R> struct Task;

template<class R>
class TaskInterface {
  public:
    typedef TaskInput<R> InputType;
    typedef TaskOutput<R> OutputType;
    typedef Configuration<R> ConfigurationType;

    //! \brief The name of the task, to be used for thread naming
    virtual String name() const = 0;
    //! \brief Return the constraint set for the task
    virtual ConstraintSet<R> const& constraint_set() const = 0;
    //! \brief Set the constraint set for the task
    virtual void set_constraint_set(ConstraintSet<R> const& constraint_set) = 0;

    //! \brief The task to be performed, taking \a in as input and \a cfg as a configuration of the parameters
    virtual OutputType run(InputType const& in, ConfigurationType const& cfg) const = 0;
    //! \brief Rank, using the constraints, the cost of points from output and execution time, possibly using the input \a in
    //! \details While the robustness of a given point is the minimum over the robustness of all constraints, the actual cost ordering on the set
    //! instead depends on the RankingCriterion
    virtual Set<PointRanking> rank(Map<ConfigurationSearchPoint,OutputType> const& data, InputType const& in) const = 0;
};

} // namespace pExplore

#endif // PEXPLORE_TASK_INTERFACE_HPP
