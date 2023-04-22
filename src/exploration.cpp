/***************************************************************************
 *            exploration.cpp
 *
 *  Copyright  2023  Luca Geretti
 *
 ****************************************************************************/

/*
 * This file is part of ProNest, under the MIT license.
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

#include "exploration.hpp"

namespace pExplore {

Set<ConfigurationSearchPoint> ShiftAndKeepBestHalfExploration::next_points_from(Set<PointEvaluation> const& rankings) const {
    Set<ConfigurationSearchPoint> result;
    size_t cnt = 0;
    for (auto it = rankings.begin(); it != rankings.end(); ++it) {
        result.insert(it->point());
        ++cnt;
        if (cnt >= rankings.size()/2) break;
    }
    result = make_extended_set_by_shifting(result, rankings.size());

    return result;
}

ExplorationInterface* ShiftAndKeepBestHalfExploration::clone() const {
    return new ShiftAndKeepBestHalfExploration();
}

} // namespace pExplore
