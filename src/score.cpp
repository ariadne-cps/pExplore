/***************************************************************************
 *            score.cpp
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

#include "helper/string.hpp"
#include "score.hpp"

namespace pExplore {

using Helper::Set;
using Helper::to_string;

Score::Score(Set<size_t> const& successes, Set<size_t> const& hard_failures, Set<size_t> const& soft_failures, double objective)
        : _successes(successes), _hard_failures(hard_failures), _soft_failures(soft_failures), _objective(objective) { }

Set<size_t> const& Score::successes() const {
    return _successes;
}

Set<size_t> const& Score::hard_failures() const {
    return _hard_failures;
}

Set<size_t> const& Score::soft_failures() const {
    return _soft_failures;
}

double Score::objective() const {
    return _objective;
}

bool Score::operator<(Score const& e) const {
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

bool Score::operator==(Score const& e) const {
    return _hard_failures == e.hard_failures() and _soft_failures == e.soft_failures() and (_objective == e.objective() or (std::isnan(_objective) and std::isnan(e.objective())));
}

ostream& Score::_write(ostream& os) const {
    return os << "{ successes " << successes() << ", hard_failures " << hard_failures() << ", soft_failures " << soft_failures() << ", objective " << objective() << "}";
}

PointScore::PointScore(ConfigurationSearchPoint const& p, Score const& score)
           : _point(p), _score(score) { }

ConfigurationSearchPoint const& PointScore::point() const {
    return _point;
}

Score const& PointScore::score() const {
    return _score;
}

bool PointScore::operator<(PointScore const& e) const {
    if (_score == e.score()) return _point < e._point;
    else return _score < e.score();
}

ostream& PointScore::_write(ostream& os) const {
    return os << "{" << _point << ": " << _score << "}";
}

} // namespace pExplore
