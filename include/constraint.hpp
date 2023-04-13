/***************************************************************************
 *            constraint.hpp
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

/*! \file constraint.hpp
 *  \brief Classes for handling a constraint for ranking the results of a task.
 */

#ifndef PEXPLORE_CONSTRAINT_HPP
#define PEXPLORE_CONSTRAINT_HPP

#include <functional>
#include <chrono>
#include "utility/container.hpp"
#include "utility/string.hpp"
#include "utility/writable.hpp"
#include "utility/macros.hpp"
#include "pronest/configuration_search_point.hpp"
#include "point_ranking.hpp"

namespace pExplore {

using Utility::WritableInterface;
using Utility::String;
using Utility::Set;
using Utility::Map;
using ProNest::ConfigurationSearchPoint;
using std::ostream;

template<class R> struct TaskInput;
template<class R> struct TaskOutput;

//! \brief Enumeration for the severity of satisfying a constraint
//! \details PERMISSIVE: satisfying the constraint is only desired
//!          CRITICAL: satisfying the constraint is mandatory
enum class ConstraintSeverity { PERMISSIVE, CRITICAL };
inline std::ostream& operator<<(std::ostream& os, const ConstraintSeverity severity) {
    switch (severity) {
        case ConstraintSeverity::PERMISSIVE: os << "PERMISSIVE"; break;
        case ConstraintSeverity::CRITICAL: os << "CRITICAL"; break;
        default: UTILITY_FAIL_MSG("Unhandled ConstraintSeverity value.");
    }
    return os;
}

template<class R> class CriticalRankingFailureException : public std::runtime_error {
public:
    CriticalRankingFailureException(double score) : std::runtime_error("The execution has critical failure with the following score: " + to_string(score)) { }
};

//! \brief A constraint in the input \a in and output \a out objects of the task
//! \details The constraint is expressed as f(in,out) > 0
template<class R> class Constraint : public WritableInterface {
  public:
    typedef TaskInput<R> InputType;
    typedef TaskOutput<R> OutputType;

    Constraint(String const& name, ConstraintSeverity const& severity, std::function<double(InputType const&, OutputType const&)> func)
            : _name(name), _severity(severity), _func(func) { }
    Constraint(ConstraintSeverity const& severity, std::function<double(InputType const&, OutputType const&)> func)
            : Constraint(std::string(), severity, func) { }
    Constraint()
            : Constraint(ConstraintSeverity::PERMISSIVE, [](InputType const&, OutputType const&){ return 0.0; }) { }

    String const& name() const { return _name; }
    ConstraintSeverity severity() const { return _severity; }

    //! \brief Get the robustness (i.e., the degree of satisfaction of the constraint) given an \a input and \a output
    double robustness(InputType const& input, OutputType const& output) const { return _func(input, output); }

    ostream& _write(ostream& os) const override { return os << *this; }

    friend ostream& operator<<(ostream& os, Constraint<R> const& p) {
        os << "{'" << p.name() << "," << p.severity() << "}"; return os; }

  private:
    String _name;
    ConstraintSeverity _severity;
    std::function<double(InputType const&, OutputType const&)> _func;
};

} // namespace pExplore

#endif // PEXPLORE_CONSTRAINT_HPP
