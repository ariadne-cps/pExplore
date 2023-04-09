/***************************************************************************
 *            task_execution_ranking.cpp
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
#include "task_execution_ranking.hpp"

namespace pExplore {

using Utility::to_string;

TaskExecutionRanking::TaskExecutionRanking(ConfigurationSearchPoint const& p,
                                           ScoreType const& s,
                                           size_t const& permissive_failures,
                                           size_t const& critical_failures)
           : _point(p), _score(s), _permissive_failures(permissive_failures), _critical_failures(critical_failures) { }

ConfigurationSearchPoint const&
TaskExecutionRanking::point() const {
    return _point;
}

ScoreType const&
TaskExecutionRanking::score() const {
    return _score;
}

size_t const&
TaskExecutionRanking::permissive_failures() const {
    return _permissive_failures;
}

size_t const&
TaskExecutionRanking::critical_failures() const {
    return _critical_failures;
}

bool TaskExecutionRanking::operator<(TaskExecutionRanking const& s) const {
    if (this->_critical_failures > s._critical_failures) return true;
    else if (this->_critical_failures < s._critical_failures) return false;
    else if (this->_permissive_failures > s._permissive_failures) return true;
    else if (this->_permissive_failures < s._permissive_failures) return false;
    else return this->_score < s._score;
}

ostream& TaskExecutionRanking::_write(ostream& os) const {
    return os << "{" << _point << ":" << _score << (_permissive_failures > 0 ? (",P:" + to_string(_permissive_failures)) : "")
              << (_critical_failures > 0 ? (",C:" + to_string(_critical_failures)) : "") << "}";
}
} // namespace pExplore
