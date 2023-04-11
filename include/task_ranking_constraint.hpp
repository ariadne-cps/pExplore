/***************************************************************************
 *            task_ranking_constraint.hpp
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

/*! \file task_ranking_constraint.hpp
 *  \brief Classes for handling a constraint for ranking the results of a task.
 */

#ifndef PEXPLORE_TASK_RANKING_CONSTRAINT_HPP
#define PEXPLORE_TASK_RANKING_CONSTRAINT_HPP

#include <functional>
#include <chrono>
#include "utility/container.hpp"
#include "utility/string.hpp"
#include "utility/writable.hpp"
#include "utility/macros.hpp"
#include "pronest/configuration_search_point.hpp"
#include "task_execution_ranking.hpp"

namespace pExplore {

using Utility::WritableInterface;
using Utility::String;
using Utility::Set;
using Utility::Map;
using ProNest::ConfigurationSearchPoint;
using std::ostream;

template<class R> struct TaskInput;
template<class R> struct TaskOutput;

enum class OptimisationCriterion { MINIMISE, MAXIMISE };
inline std::ostream& operator<<(std::ostream& os, const OptimisationCriterion opt) {
    switch (opt) {
        case OptimisationCriterion::MAXIMISE: os << "MAXIMISE"; break;
        case OptimisationCriterion::MINIMISE: os << "MINIMISE"; break;
        default: UTILITY_FAIL_MSG("Unhandled OptimisationCriterion value.");
    }
    return os;
}

//! \brief Enumeration for the severity of satisfying a constraint
//! \details PERMISSIVE: satisfying the constraint is only desired
//!          CRITICAL: satisfying the constraint is mandatory
enum class RankingConstraintSeverity { PERMISSIVE, CRITICAL };
inline std::ostream& operator<<(std::ostream& os, const RankingConstraintSeverity severity) {
    switch (severity) {
        case RankingConstraintSeverity::PERMISSIVE: os << "PERMISSIVE"; break;
        case RankingConstraintSeverity::CRITICAL: os << "CRITICAL"; break;
        default: UTILITY_FAIL_MSG("Unhandled RankingConstraintSeverity value.");
    }
    return os;
}

template<class R> class TaskRankingConstraint : public WritableInterface {
  public:
    typedef TaskInput<R> InputType;
    typedef TaskOutput<R> OutputType;

    TaskRankingConstraint(String const& name, OptimisationCriterion const& opt, RankingConstraintSeverity const& severity, std::function<double(InputType const&, OutputType const&)> func)
            : _name(name), _optimisation(opt), _severity(severity), _func(func) { }
    TaskRankingConstraint(OptimisationCriterion const& opt, RankingConstraintSeverity const& severity, std::function<double(InputType const&, OutputType const&)> func)
            : TaskRankingConstraint(std::string(), opt, severity, func) { }
    TaskRankingConstraint()
            : TaskRankingConstraint(OptimisationCriterion::MAXIMISE, RankingConstraintSeverity::PERMISSIVE, [](InputType const&, OutputType const&){ return 0.0; }) { }

    String const& name() const { return _name; }
    OptimisationCriterion optimisation() const { return _optimisation; }
    RankingConstraintSeverity severity() const { return _severity; }

    double rank(InputType const& input, OutputType const& output) const { return _func(input, output); }

    Set<TaskExecutionRanking> rank(Map<ConfigurationSearchPoint,OutputType> const& data, InputType const& input) const {
        Set<TaskExecutionRanking> result;
        for (auto entry : data) {
            auto val = rank(input,entry.second);
            result.insert({entry.first,(_optimisation == OptimisationCriterion::MAXIMISE ? val : -val)});
        }
        return result;
    }

    ostream& _write(ostream& os) const override { return os << *this; }

    friend ostream& operator<<(ostream& os, TaskRankingConstraint<R> const& p) {
        os << "{'" << p.name() << "'," << p.optimisation() << "," << p.severity() << "}"; return os; }

  private:
    String _name;
    OptimisationCriterion _optimisation;
    RankingConstraintSeverity _severity;
    std::function<double(InputType const&, OutputType const&)> _func;
};

} // namespace pExplore

#endif // PEXPLORE_TASK_RANKING_CONSTRAINT_HPP
