/***************************************************************************
 *            task_ranking_parameter.hpp
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

/*! \file task_ranking_parameter.hpp
 *  \brief Classes for handling parameters for ranking the results of a task.
 */

#ifndef PEXPLORE_TASK_RANKING_PARAMETER_HPP
#define PEXPLORE_TASK_RANKING_PARAMETER_HPP

#include <functional>
#include <chrono>
#include "utility/container.hpp"
#include "utility/string.hpp"
#include "utility/writable.hpp"
#include "utility/macros.hpp"
#include "utility/handle.hpp"

namespace pExplore {

using Utility::WritableInterface;
using Utility::String;
using Utility::Handle;
using std::ostream;

template<class R> struct TaskInput;
template<class R> struct TaskOutput;
template<class R> struct TaskObjective;

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
//! \details NONE: there actually is no constraint
//!          PERMISSIVE: satisfying the constraint is only desired
//!          CRITICAL: satisfying the constraint is mandatory
enum class RankingConstraintSeverity { NONE, PERMISSIVE, CRITICAL };
inline std::ostream& operator<<(std::ostream& os, const RankingConstraintSeverity severity) {
    switch (severity) {
        case RankingConstraintSeverity::NONE: os << "NONE"; break;
        case RankingConstraintSeverity::PERMISSIVE: os << "PERMISSIVE"; break;
        case RankingConstraintSeverity::CRITICAL: os << "CRITICAL"; break;
        default: UTILITY_FAIL_MSG("Unhandled RankingConstraintSeverity value.");
    }
    return os;
}

typedef double ScoreType;

template<class R> class TaskRankingParameterInterface : public WritableInterface {
public:
    typedef TaskInput<R> InputType;
    typedef TaskOutput<R> OutputType;

    virtual String const& name() const = 0;
    virtual OptimisationCriterion optimisation() const = 0;
    virtual RankingConstraintSeverity severity() const = 0;
    virtual bool is_scalar() const = 0;
    virtual bool uses_objective() const = 0;
    virtual bool discard(InputType const& input) const = 0;
    virtual ScoreType rank(InputType const& input, OutputType const& output, size_t const& idx = 0) const = 0;
    virtual ScoreType threshold(InputType const& input, OutputType const& output, size_t const& idx = 0) const = 0;
    virtual size_t dimension(InputType const& input) const = 0;

    virtual TaskRankingParameterInterface* clone() const = 0;
    virtual ~TaskRankingParameterInterface() = default;

    ostream& _write(ostream& os) const override { return os << *this; }

    friend ostream& operator<<(ostream& os, TaskRankingParameterInterface<R> const& p) {
        os << "{'" << p.name() << "'," << p.optimisation() << "," <<
           (p.is_scalar() ? "SCALAR":"VECTOR") << "," <<
           (p.uses_objective() ? "OBJECTIVE":"NO_OBJECTIVE") << "}"; return os; }
};

template<class R> class TaskRankingParameterBase : public TaskRankingParameterInterface<R> {
  public:
    TaskRankingParameterBase(String const& name, OptimisationCriterion const& opt, RankingConstraintSeverity const& severity)
            : _name(name), _optimisation(opt), _severity(severity) { }

    String const& name() const override { return _name; }
    OptimisationCriterion optimisation() const override { return _optimisation; }
    RankingConstraintSeverity severity() const override { return _severity; }

  private:
    String const _name;
    OptimisationCriterion const _optimisation;
    RankingConstraintSeverity const _severity;
};

template<class R> class ScalarRankingParameter : public TaskRankingParameterBase<R> {
  public:
    typedef TaskInput<R> InputType;
    typedef TaskOutput<R> OutputType;
    ScalarRankingParameter(String const& name, OptimisationCriterion const& opt, std::function<ScoreType(InputType const&, OutputType const&)> const rfunc)
        : TaskRankingParameterBase<R>(name, opt, RankingConstraintSeverity::NONE), _rfunc(rfunc) { }

    bool is_scalar() const override { return true; }
    bool uses_objective() const override { return false; }
    bool discard(InputType const& input) const override { return false; }
    size_t dimension(InputType const& input) const override { return 1; }
    ScoreType rank(InputType const& input, OutputType const& output, size_t const& idx = 0) const override { return _rfunc(input, output); }
    ScoreType threshold(InputType const& input, OutputType const& output, size_t const& idx = 0) const override { UTILITY_ERROR("Cannot compute threshold for non-objective scalar parameter"); return 0; }

    ScalarRankingParameter* clone() const override { return new ScalarRankingParameter(*this); }
  private:
    std::function<ScoreType(InputType const&, OutputType const&)> const _rfunc;
};

template<class R> class ScalarObjectiveRankingParameter : public TaskRankingParameterBase<R> {
public:
    typedef TaskInput<R> InputType;
    typedef TaskOutput<R> OutputType;
    typedef TaskObjective<R> ObjectiveType;
    typedef std::function<ScoreType(InputType const&, OutputType const&, ObjectiveType const&)> MeasureFunctionType;
    typedef std::function<bool(InputType const&, ObjectiveType const&)> DiscardFunctionType;
    ScalarObjectiveRankingParameter(String const& name, OptimisationCriterion const& opt, RankingConstraintSeverity const& severity, ObjectiveType const& objective,
                                    MeasureFunctionType const& score, MeasureFunctionType const& threshold, DiscardFunctionType const& discard)
            : TaskRankingParameterBase<R>(name, opt, severity), _objective(objective),  _sfunc(score), _tfunc(threshold), _dfunc(discard) { }

    bool is_scalar() const override { return true; }
    bool uses_objective() const override { return true; }
    size_t dimension(InputType const& input) const override { return 1; }
    bool discard(InputType const& input) const override { return _dfunc(input,_objective); }
    ScoreType rank(InputType const& input, OutputType const& output, size_t const& idx = 0) const override {
        auto score = _sfunc(input, output, _objective);
        return score;
    }
    ScoreType threshold(InputType const& input, OutputType const& output, size_t const& idx = 0) const override { return _tfunc(input,output,_objective); }

    ScalarObjectiveRankingParameter* clone() const override { return new ScalarObjectiveRankingParameter(*this); }
private:
    ObjectiveType const _objective;
    MeasureFunctionType const _sfunc;
    MeasureFunctionType const _tfunc;
    DiscardFunctionType const _dfunc;
};

template<class R> class VectorRankingParameter : public TaskRankingParameterBase<R> {
public:
    typedef TaskInput<R> InputType;
    typedef TaskOutput<R> OutputType;
    VectorRankingParameter(String const& name, OptimisationCriterion const& opt, std::function<ScoreType(InputType const&, OutputType const&, size_t const&)> const rfunc, std::function<size_t(InputType const&)> const dfunc)
            : TaskRankingParameterBase<R>(name, opt, RankingConstraintSeverity::NONE), _rfunc(rfunc), _dfunc(dfunc) { }

    bool is_scalar() const override { return false; };
    bool uses_objective() const override { return false; }

    size_t dimension(InputType const& input) const override { return _dfunc(input); }
    bool discard(InputType const& input) const override { return false; }
    ScoreType rank(InputType const& input, OutputType const& output, size_t const& idx) const override { return _rfunc(input, output, idx); }
    ScoreType threshold(InputType const& input, OutputType const& output, size_t const& idx = 0) const override { UTILITY_ERROR("Cannot compute threshold for non-objective vector parameter"); return 0; }

    VectorRankingParameter* clone() const override { return new VectorRankingParameter(*this); }
private:
    std::function<ScoreType(InputType const&, OutputType const&, size_t const&)> const _rfunc;
    std::function<size_t(InputType const&)> const _dfunc;
};

template<class R> class TaskRankingParameter : public Handle<TaskRankingParameterInterface<R>> {
public:
    typedef TaskInput<R> InputType;
    typedef TaskOutput<R> OutputType;
    using Handle<TaskRankingParameterInterface<R>>::Handle;
public:

    String const& name() const { return this->_ptr->name(); }
    OptimisationCriterion optimisation() const { return this->_ptr->optimisation(); }
    RankingConstraintSeverity severity() const { return this->_ptr->severity(); }
    bool is_scalar() const { return this->_ptr->is_scalar(); };
    bool uses_objective() const { return this->_ptr->uses_objective(); }
    bool discard(InputType const& input) const {
        return this->_ptr->discard(input); }
    ScoreType rank(InputType const& input, OutputType const& output, size_t const& idx = 0) const {
        return this->_ptr->rank(input, output, idx); }
    ScoreType threshold(InputType const& input, OutputType const& output, size_t const& idx = 0) const {
        return this->_ptr->threshold(input, output, idx); }

    size_t dimension(InputType const& input) const { return this->_ptr->dimension(input); }
    friend ostream& operator<<(ostream& os, const TaskRankingParameter<R>& p) { return os << *p._ptr; }
};

} // namespace pExplore

#endif // PEXPLORE_TASK_RANKING_PARAMETER_HPP
