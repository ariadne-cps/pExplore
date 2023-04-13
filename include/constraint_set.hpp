/***************************************************************************
 *            constraint_set.hpp
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

/*! \file constraint_set.hpp
 *  \brief Class for defining a set of constraints.
 */

#ifndef PEXPLORE_CONSTRAINT_SET
#define PEXPLORE_CONSTRAINT_SET

#include "utility/container.hpp"
#include "utility/writable.hpp"
#include "constraint.hpp"

namespace pExplore {

using ProNest::ConfigurationSearchPoint;
using Utility::WritableInterface;
using std::to_string;
using std::ostream;
using std::min;

template<class R> class ConstraintSet : public WritableInterface {
public:
    typedef TaskInput<R> InputType;
    typedef TaskOutput<R> OutputType;

    ConstraintSet(List<Constraint<R>> const& constraints, RankingCriterion const& criterion) :
        _constraints(constraints), _criterion(criterion), _has_critical_constraints(false) {
        for (auto const& c : constraints) {
            if (c.severity() == ConstraintSeverity::CRITICAL) {
                _has_critical_constraints = true;
                break;
            }
        }
    }

    ConstraintSet() : _constraints(List<Constraint<R>>()), _criterion(RankingCriterion::MAXIMISE), _has_critical_constraints(false) { }

    PointRanking robustness(ConfigurationSearchPoint const& point, InputType const& input, OutputType const& output) const {
        double rob = std::numeric_limits<double>::max();
        for (auto const& c : _constraints) {
            rob = min(rob, c.robustness(input, output));
        }
        return {point, rob, _criterion};
    }

    bool has_critical_constraints() const {
        return _has_critical_constraints;
    }

    List<Constraint<R>> const& constraints() const { return _constraints; }

    RankingCriterion const& criterion() const { return _criterion; }

    virtual ostream& _write(ostream& os) const {
        os << "{" << _constraints << ": " << _criterion << "}";
        return os;
    }

  private:
    List<Constraint<R>> _constraints;
    RankingCriterion _criterion;
    bool _has_critical_constraints;
};

} // namespace pExplore

#endif // PEXPLORE_CONSTRAINT_SET
