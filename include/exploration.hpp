/***************************************************************************
 *            exploration.hpp
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

/*! \file exploration.hpp
 *  \brief Class for strategies to explore search points across iterations.
 */

#ifndef PEXPLORE_EXPLORATION_HPP
#define PEXPLORE_EXPLORATION_HPP

#include "pronest/configuration_search_point.hpp"
#include "utility/container.hpp"
#include "evaluation.hpp"

namespace pExplore {

using Utility::Set;

//! \brief Interface for search strategies
class ExplorationInterface {
  public:
    //! \brief Make the next points from set of points from \a rankings, preserving the size
    virtual Set<ConfigurationSearchPoint> next_points_from(Set<PointEvaluation> const& rankings) const = 0;

    virtual ExplorationInterface* clone() const = 0;
    virtual ~ExplorationInterface() = default;
};

//! \brief Keeps the best half points, to which we add the shifted points from each (with a distance 1 if possible)
class ShiftAndKeepBestHalfExploration : public ExplorationInterface {
  public:
    Set<ConfigurationSearchPoint> next_points_from(Set<PointEvaluation> const& rankings) const override;
    ExplorationInterface* clone() const override;
};

} // namespace pExplore

#endif // PEXPLORE_EXPLORATION_HPP
