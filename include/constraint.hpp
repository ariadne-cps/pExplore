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
 *  \brief Classes for handling a generic constraint
 */

#ifndef PEXPLORE_CONSTRAINT_HPP
#define PEXPLORE_CONSTRAINT_HPP

#include <functional>
#include <chrono>
#include "helper/container.hpp"
#include "helper/string.hpp"
#include "helper/writable.hpp"
#include "helper/macros.hpp"
#include "pronest/configuration_search_point.hpp"
#include "robustness_controller.hpp"
#include "score.hpp"

namespace pExplore {

using Helper::WritableInterface;
using Helper::String;
using Helper::Set;
using Helper::Map;
using ProNest::ConfigurationSearchPoint;
using std::ostream;
using std::shared_ptr;

template<class R> struct TaskInput;
template<class R> struct TaskOutput;

//! \brief Enumeration for the action with respect to success in evaluation of the constraint
//! \details NONE: no action performed
//!          DEACTIVATE: the constraint is deactivated from a set
enum class ConstraintSuccessAction { NONE, DEACTIVATE };
inline std::ostream& operator<<(std::ostream& os, const ConstraintSuccessAction success_action) {
    switch (success_action) {
        case ConstraintSuccessAction::NONE: os << "NONE"; break;
        case ConstraintSuccessAction::DEACTIVATE: os << "DEACTIVATE"; break;
        default: HELPER_FAIL_MSG("Unhandled ConstraintSuccessAction value.");
    }
    return os;
}

//! \brief Enumeration for the kind of failure in the evaluation of the constraint
//! \details NONE: no requirement on the sign of the constraint evaluation
//!          SOFT: satisfying the constraint is desired
//!          HARD: satisfying the constraint is mandatory, if all points fail then the constraint has failed
enum class ConstraintFailureKind { NONE, SOFT, HARD };
inline std::ostream& operator<<(std::ostream& os, const ConstraintFailureKind failure_kind) {
    switch (failure_kind) {
        case ConstraintFailureKind::NONE: os << "NONE"; break;
        case ConstraintFailureKind::SOFT: os << "SOFT"; break;
        case ConstraintFailureKind::HARD: os << "HARD"; break;
        default: HELPER_FAIL_MSG("Unhandled ConstraintFailureKind value.");
    }
    return os;
}

//! \brief Enumeration for the impact of the evaluation of the constraint on the global objective function
//! \details NONE: no impact
//!          SIGNED: the sign of the evaluation is considered
//!          UNSIGNED: the absolute value of the evaluation is considered
enum class ConstraintObjectiveImpact { NONE, SIGNED, UNSIGNED };
inline std::ostream& operator<<(std::ostream& os, const ConstraintObjectiveImpact objective_impact) {
    switch (objective_impact) {
        case ConstraintObjectiveImpact::NONE: os << "NONE"; break;
        case ConstraintObjectiveImpact::SIGNED: os << "SIGNED"; break;
        case ConstraintObjectiveImpact::UNSIGNED: os << "UNSIGNED"; break;
        default: HELPER_FAIL_MSG("Unhandled ConstraintObjectiveImpact value.");
    }
    return os;
}

template<class R> class ConstraintBuilder;

//! \brief A constraint in the input \a in and output \a out objects of the task
//! \details The constraint is expressed as f(in,out) > 0
template<class R> class Constraint : public WritableInterface {
    friend class ConstraintBuilder<R>;
  public:
    typedef TaskInput<R> InputType;
    typedef TaskOutput<R> OutputType;

  protected:
    Constraint(String const& name, size_t const& group_id, ConstraintSuccessAction const& success_action, ConstraintFailureKind const& failure_kind, ConstraintObjectiveImpact const& objective_impact,
               std::function<double(InputType const&, OutputType const&)> func, RobustnessControllerInterface<R> const& controller)
            : _name(name), _group_id(group_id), _success_action(success_action), _failure_kind(failure_kind), _objective_impact(objective_impact), _func(func), _controller_ptr(controller.clone()) { }

  public:
    String const& name() const { return _name; }
    size_t const& group_id() const { return _group_id; }
    ConstraintSuccessAction success_action() const { return _success_action; }
    ConstraintFailureKind failure_kind() const { return _failure_kind; }
    ConstraintObjectiveImpact objective_impact() const { return _objective_impact; }

    RobustnessControllerInterface<R> const& controller() const { return *_controller_ptr; }

    //! \brief Get the degree of satisfaction of the constraint given an \a input and \a output, optionally updating the robustness controller with \a update
    double robustness(InputType const& input, OutputType const& output, bool update_controller) const { return _controller_ptr->apply(_func(input, output),input,output,update_controller); }

    ostream& _write(ostream& os) const override {
        return os << "{'" << _name << "', group_id=" << _group_id << ", success_action=" << _success_action << ", failure_kind=" << _failure_kind << ", objective_impact=" << _objective_impact << "}";
    }

  private:
    String _name;
    size_t _group_id;
    ConstraintSuccessAction _success_action;
    ConstraintFailureKind _failure_kind;
    ConstraintObjectiveImpact _objective_impact;
    std::function<double(InputType const&, OutputType const&)> _func;
    shared_ptr<RobustnessControllerInterface<R>> _controller_ptr;
};

//! \brief A builder for Constraint, to account for the various optional arguments
template<class R> class ConstraintBuilder {
  public:
    typedef TaskInput<R> InputType;
    typedef TaskOutput<R> OutputType;

    ConstraintBuilder(std::function<double(InputType const&, OutputType const&)> func) :
        _name(std::string()), _group_id(0), _success_action(ConstraintSuccessAction::NONE), _failure_kind(ConstraintFailureKind::NONE), _objective_impact(ConstraintObjectiveImpact::NONE),
        _func(func), _controller_ptr(new IdentityRobustnessController<R>()) { }

    ConstraintBuilder& set_name(String name) { _name = name; return *this; }
    ConstraintBuilder& set_group_id(size_t group_id) { _group_id = group_id; return *this; }
    ConstraintBuilder& set_success_action(ConstraintSuccessAction const& success_action) { _success_action = success_action; return *this; }
    ConstraintBuilder& set_failure_kind(ConstraintFailureKind const& failure_kind) { _failure_kind = failure_kind; return *this; }
    ConstraintBuilder& set_objective_impact(ConstraintObjectiveImpact const& objective_impact) { _objective_impact = objective_impact; return *this; }
    ConstraintBuilder& set_controller(RobustnessControllerInterface<R> const& controller) { _controller_ptr.reset(controller.clone()); return *this; }

    Constraint<R> build() const { return {_name,_group_id,_success_action,_failure_kind,_objective_impact,_func,*_controller_ptr}; }

  private:
    String _name;
    size_t _group_id;
    ConstraintSuccessAction _success_action;
    ConstraintFailureKind _failure_kind;
    ConstraintObjectiveImpact _objective_impact;
    std::function<double(InputType const&, OutputType const&)> const _func;
    shared_ptr<RobustnessControllerInterface<R>> _controller_ptr;
};

//! \brief The state of a constraint as it is processed
template<class R> class ConstraintState : public WritableInterface {
  public:
    ConstraintState(Constraint<R> const& constraint) : _constraint(constraint), _active(true), _success(false), _failure(false) { }

    bool is_active() const { return _active; }
    bool has_succeeded() const { return _success; }
    bool has_failed() const { return _failure; }
    Constraint<R> const& constraint() const { return _constraint; }

    //! \brief Set the constraint as inactive
    //! \details Constraints with the same group_id as others may be deactivated as a result of success/failure on those,
    //! even if this one has no failure or success
    void deactivate() { _active = false; }
    void set_success() { HELPER_PRECONDITION(not _failure) _success = true; }
    void set_failure() { HELPER_PRECONDITION(not _success) _failure = true; }

    ostream& _write(ostream& os) const override {
        return os << "{" << _constraint << ", active=" << _active << ", has_succeeded=" << _success << ", has_failed=" << _failure << "}";
    }

  private:
    Constraint<R> _constraint;
    bool _active;
    bool _success;
    bool _failure;
};

} // namespace pExplore

#endif // PEXPLORE_CONSTRAINT_HPP
