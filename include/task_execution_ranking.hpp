/***************************************************************************
 *            task_execution_ranking.hpp
 *
 *  Copyright  2007-20  Luca Geretti
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

/*! \file task_execution_ranking.hpp
 *  \brief Class for defining the score for a point.
 */

#ifndef PEXPLORE_TASK_EXECUTION_RANKING_HPP
#define PEXPLORE_TASK_EXECUTION_RANKING_HPP

#include "utility/container.hpp"
#include "utility/writable.hpp"
#include "pronest/configuration_search_point.hpp"

namespace pExplore {

using ProNest::ConfigurationSearchPoint;
using Utility::WritableInterface;
using std::to_string;
using std::ostream;

class TaskExecutionRanking;

template<class R> class CriticalRankingFailureException : public std::runtime_error {
public:
    CriticalRankingFailureException(double score) : std::runtime_error("The execution has critical failure with the following score: " + to_string(score)) { }
};

class TaskExecutionRanking : public WritableInterface {
public:
    TaskExecutionRanking(ConfigurationSearchPoint const& p, double s);
    ConfigurationSearchPoint const& point() const;
    double score() const;
    //! \brief Ordering is based on score
    bool operator<(TaskExecutionRanking const& s) const;

    virtual ostream& _write(ostream& os) const;
private:
    ConfigurationSearchPoint _point;
    double _score;
};

} // namespace pExplore

#endif // PEXPLORE_TASK_EXECUTION_RANKING_HPP
