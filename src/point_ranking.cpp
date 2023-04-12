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
#include "point_ranking.hpp"

namespace pExplore {

using Utility::to_string;

PointRanking::PointRanking(ConfigurationSearchPoint const& p, double s, RankingCriterion const& criterion)
           : _point(p), _score(s), _criterion(criterion) { }

ConfigurationSearchPoint const& PointRanking::point() const {
    return _point;
}

double PointRanking::score() const {
    return _score;
}

bool PointRanking::operator<(PointRanking const& s) const {
    UTILITY_PRECONDITION(_criterion == s._criterion)
    if (_score == s._score)
        return _point < s._point;

    switch (_criterion) {
        case RankingCriterion::MAXIMISE :
            return _score < s._score;
        case RankingCriterion::MINIMISE_POSITIVE :
            return ((_score >= 0 and s._score >= 0) ? _score > s._score : _score < s._score);
        default :
            UTILITY_FAIL_MSG("Unhandled RankingCriterion value.")
    }
}

ostream& PointRanking::_write(ostream& os) const {
    return os << "{" << _point << ":" << _score << "}";
}
} // namespace pExplore
