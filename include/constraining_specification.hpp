/***************************************************************************
 *            constraining_specification.hpp
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

/*! \file constraining_specification.hpp
 *  \brief Class for defining a time-varying constraining specification and a control strategy to enforce it.
 */

#ifndef PEXPLORE_CONSTRAINING_SPECIFICATION
#define PEXPLORE_CONSTRAINING_SPECIFICATION

#include "utility/container.hpp"
#include "utility/writable.hpp"
#include "constraint.hpp"

namespace pExplore {

using ProNest::ConfigurationSearchPoint;
using Utility::WritableInterface;
using std::to_string;
using std::ostream;
using std::min;

template<class R> class ConstrainingSpecification : public WritableInterface {
  public:
    typedef TaskInput<R> InputType;
    typedef TaskOutput<R> OutputType;

    ConstrainingSpecification(List<Constraint<R>> const& constraints) : _constraint_states(List<ConstraintState<R>>()), _num_active_constraints(constraints.size()) {
        for (auto const& c : constraints) {
            _constraint_states.push_back(c);
        }
    }

    ConstrainingSpecification() : ConstrainingSpecification(List<Constraint<R>>()) { }

    PointScore evaluate(ConfigurationSearchPoint const& point, InputType const& input, OutputType const& output) const {
        return {point, evaluate(input,output)};
    }

    Score evaluate(InputType const& input, OutputType const& output) const {
        UTILITY_PRECONDITION(_num_active_constraints > 0)
        double objective = 0.0;
        Set<size_t> successes;
        Set<size_t> hard_failures;
        Set<size_t> soft_failures;
        for (size_t i=0; i<_constraint_states.size(); ++i) {
            auto const& s = _constraint_states.at(i);
            if (not s.has_succeeded() and not s.has_failed()) {
                auto const& c = s.constraint();
                auto robustness = c.robustness(input,output);
                switch (c.objective_impact()) {
                    case ConstraintObjectiveImpact::UNSIGNED :
                        objective += abs(robustness);
                        break;
                    case ConstraintObjectiveImpact::SIGNED :
                        objective += robustness;
                        break;
                    case ConstraintObjectiveImpact::NONE :
                        break;
                    default : UTILITY_FAIL_MSG("Unhandled ConstraintObjectiveImpact for score evaluation.")
                }
                if (robustness < 0) {
                    switch (c.failure_kind()) {
                        case ConstraintFailureKind::HARD :
                            hard_failures.insert(i);
                            break;
                        case ConstraintFailureKind::SOFT :
                            soft_failures.insert(i);
                            break;
                        case ConstraintFailureKind::NONE :
                            break;
                        default : UTILITY_FAIL_MSG("Unhandled ConstraintFailureKind for score evaluation.")
                    }
                } else {
                    successes.insert(i);
                }
            }
        }
        return {successes, hard_failures, soft_failures, objective};
    }

    //! \brief Update all constraints according to \a input and \a output, setting failures and successes,
    //! and if necessary deactivating the constraint
    //! \details The group_id from a deactivated constraint is used to deactivate other constraints
    void update_from(InputType const& input, OutputType const& output) {
        Set<size_t> group_ids_to_deactivate;
        auto eval = evaluate(input,output);

        for (size_t i=0; i<_constraint_states.size(); ++i) {
            auto& s = _constraint_states.at(i);

            if (eval.successes().contains(i)) {
                s.set_success();
                if (s.constraint().success_action() == ConstraintSuccessAction::DEACTIVATE) {
                    group_ids_to_deactivate.insert(s.constraint().group_id());
                }
            }

            if (eval.hard_failures().contains(i)) {
                s.set_failure();
                group_ids_to_deactivate.insert(s.constraint().group_id());
            }

            if (group_ids_to_deactivate.contains(s.constraint().group_id())) {
                s.deactivate();
                --_num_active_constraints;
            }
        }
    }

    List<Constraint<R>> active_constraints() const {
        List<Constraint<R>> result;
        for (auto const& s : _constraint_states)
            if (s.is_active())
                result.push_back(s);
        return result;
    }

    size_t num_active_constraints() const { return _num_active_constraints; }

    bool is_inactive() const { return _num_active_constraints == 0; }

    List<ConstraintState<R>> const& constraint_states() const {
        return _constraint_states;
    }

    virtual ostream& _write(ostream& os) const {
        return os << "{" << _constraint_states << ": " << "}";
    }

  private:
    List<ConstraintState<R>> _constraint_states;
    size_t _num_active_constraints;
};

template<class R> struct NoActiveConstraintsException : public std::runtime_error {
    NoActiveConstraintsException(List<ConstraintState<R>> const& cs) : std::runtime_error("No more active constraints are present"), constraint_states(cs) { }
    List<ConstraintState<R>> const constraint_states;
};

} // namespace pExplore

#endif // PEXPLORE_CONSTRAINING_SPECIFICATION
