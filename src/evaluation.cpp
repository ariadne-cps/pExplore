/***************************************************************************
 *            point_ranking.cpp
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

#include "utility/string.hpp"
#include "evaluation.hpp"

namespace pExplore {

using Utility::Set;
using Utility::to_string;

ConstraintEvaluation::ConstraintEvaluation(Set<size_t> const& successes, Set<size_t> const& hard_failures, Set<size_t> const& soft_failures, double objective)
        : _successes(successes), _hard_failures(hard_failures), _soft_failures(soft_failures), _objective(objective) { }

Set<size_t> const& ConstraintEvaluation::successes() const {
    return _successes;
}

Set<size_t> const& ConstraintEvaluation::hard_failures() const {
    return _hard_failures;
}

Set<size_t> const& ConstraintEvaluation::soft_failures() const {
    return _soft_failures;
}

double ConstraintEvaluation::objective() const {
    return _objective;
}

bool ConstraintEvaluation::operator<(ConstraintEvaluation const& e) const {
    if (_hard_failures < e.hard_failures())
        return true;
    else if (_hard_failures > e.hard_failures())
        return false;
    else if (_soft_failures < e.soft_failures())
        return true;
    else if (_soft_failures > e.soft_failures())
        return false;
    else return _objective < e.objective();
}

bool ConstraintEvaluation::operator==(ConstraintEvaluation const& e) const {
    return _successes == e.successes() and _hard_failures == e.hard_failures() and _soft_failures == e.soft_failures() and _objective == e.objective();
}

ostream& ConstraintEvaluation::_write(ostream& os) const {
    return os << "{ hard_failures " << hard_failures() << ", soft_failures " << soft_failures() << ", objective " << objective() << "}";
}

PointEvaluation::PointEvaluation(ConfigurationSearchPoint const& p, ConstraintEvaluation const& evaluation)
           : _point(p), _evaluation(evaluation) { }

ConfigurationSearchPoint const& PointEvaluation::point() const {
    return _point;
}

ConstraintEvaluation const& PointEvaluation::evaluation() const {
    return _evaluation;
}

bool PointEvaluation::operator<(PointEvaluation const& e) const {
    if (_evaluation == e.evaluation()) return _point < e._point;
    return _evaluation < e.evaluation();
}

ostream& PointEvaluation::_write(ostream& os) const {
    return os << "{" << _point << ": " << _evaluation << "}";
}
} // namespace pExplore
