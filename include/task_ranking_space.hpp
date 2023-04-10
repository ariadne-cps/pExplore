/***************************************************************************
 *            task_ranking_space.hpp
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

/*! \file task_ranking_space.hpp
 *  \brief Class for handling ranking of results from parameter search of a task.
 */

#ifndef PEXPLORE_TASK_RANKING_SPACE_HPP
#define PEXPLORE_TASK_RANKING_SPACE_HPP

#include "utility/array.hpp"
#include "task_ranking_parameter.hpp"
#include "task_execution_ranking.hpp"

namespace pExplore {

using std::min, std::max;
using Utility::Array;

template<class R>
class TaskRankingSpace : public WritableInterface {
  public:
    typedef TaskInput<R> InputType;
    typedef TaskOutput<R> OutputType;

    TaskRankingSpace(List<TaskRankingParameter<R>> const& parameters) : _parameters(parameters) { }

    List<TaskRankingParameter<R>> const& parameters() const { return _parameters; }

    TaskRankingSpace* clone() const { return new TaskRankingSpace(*this); }

    virtual ostream& _write(ostream& os) const { os << _parameters; return os; }

    bool has_critical_constraints() const {
        for (auto const& p : _parameters) if (p.severity() == RankingConstraintSeverity::CRITICAL) return true;
        return false;
    }

    List<TaskRankingParameter<R>> failed_critical_constraints(InputType const& input, OutputType const& output) const {
        List<TaskRankingParameter<R>> result;
        for (auto p : _parameters) {
            if (p.severity() == RankingConstraintSeverity::CRITICAL) {
                auto rank = p.rank(input,output);
                auto threshold = p.threshold(input,output);
                if ((p.optimisation() == OptimisationCriterion::MINIMISE and rank > threshold) or
                    (p.optimisation() == OptimisationCriterion::MAXIMISE and rank < threshold)) {
                    result.push_back(p);
                }
            }
        }
        return result;
    }

    List<TaskExecutionRanking> rank(Map<ConfigurationSearchPoint,OutputType> const& data, InputType const& input) const {
        List<TaskExecutionRanking> result;

        List<TaskRankingParameter<R>> kept_parameters;
        for (auto const& p : _parameters) if (not p.discard(input)) kept_parameters.push_back(p);

        // Compute the dimensions and initialise the min/max entries
        List<size_t> dimensions;
        List<Pair<ScoreType,ScoreType>> scalar_min_max;
        List<Array<Pair<ScoreType,ScoreType>>> vector_min_max;
        auto data_iter = data.cbegin();
        for (auto p : kept_parameters) {
            auto dim = p.dimension(input);
            dimensions.push_back(dim);
            if (dim == 1) {
                auto val = p.rank(input, data_iter->second);
                scalar_min_max.push_back(Pair<ScoreType,ScoreType>{val, val});
            } else {
                Array<Pair<ScoreType,ScoreType>> vals(dim);
                for (size_t i=0; i<dim; ++i) {
                    auto val = p.rank(input, data_iter->second, i);
                    vals[i] = {val,val};
                }
                vector_min_max.push_back(vals);
            }
        }

        // Update the min/max on the remaining data entries
        ++data_iter;
        while (data_iter != data.cend()) {
            for (size_t pw_idx=0; pw_idx < kept_parameters.size(); ++pw_idx) {
                auto p = kept_parameters[pw_idx];
                auto dim = dimensions[pw_idx];
                if (dim == 1) {
                    auto val = p.rank(input, data_iter->second);
                    scalar_min_max[pw_idx] = {min(scalar_min_max[pw_idx].first,val), max(scalar_min_max[pw_idx].second,val)};
                } else {
                    for (size_t i=0; i<dim; ++i) {
                        auto val = p.rank(input, data_iter->second, i);
                        vector_min_max[pw_idx][i] = {min(vector_min_max[pw_idx][i].first,val),max(vector_min_max[pw_idx][i].second,val)};
                    }
                }
            }
            ++data_iter;
        }

        // Compute the score
        for (auto entry : data) {
            // If max != min, compute (val-min)/(max-min), in the vector case also divide by the dimension
            // sum/subtract each weighted cost based on the minimise/maximise objective
            ScoreType score(0);
            size_t low_errors(0), high_errors(0);
            for (size_t pw_idx=0; pw_idx < kept_parameters.size(); ++pw_idx) {
                auto p = kept_parameters[pw_idx];
                auto dim = dimensions[pw_idx];
                ScoreType local_score(0);
                if (dim == 1) {
                    auto max_min_diff = scalar_min_max[pw_idx].second - scalar_min_max[pw_idx].first;
                    auto rank = p.rank(input, entry.second);
                    if (p.uses_objective()) {
                        auto threshold = p.threshold(input, entry.second);
                        if ((p.optimisation() == OptimisationCriterion::MINIMISE and rank > threshold) or
                            (p.optimisation() == OptimisationCriterion::MAXIMISE and rank < threshold)) {
                            if (p.severity() == RankingConstraintSeverity::PERMISSIVE) ++low_errors;
                            else if (p.severity() == RankingConstraintSeverity::CRITICAL) ++high_errors;
                        }
                    }
                    if (max_min_diff > 0) local_score = (rank-scalar_min_max[pw_idx].first)/max_min_diff;
                } else {
                    size_t effective_dim = dim;
                    for (size_t i=0; i<dim; ++i) {
                        auto max_min_diff = vector_min_max[pw_idx][i].second - vector_min_max[pw_idx][i].first;
                        auto rank = p.rank(input, entry.second, i);
                        if (max_min_diff > 0) local_score = (rank - vector_min_max[pw_idx][i].first)/max_min_diff;
                        else --effective_dim;
                        if (effective_dim > 0) local_score/=effective_dim;
                    }
                }

                if (p.optimisation() == OptimisationCriterion::MAXIMISE) score += local_score;
                else score -= local_score;
            }
            result.push_back(TaskExecutionRanking(entry.first, score, low_errors, high_errors));
        }

        return result;
    }

  private:
    List<TaskRankingParameter<R>> const _parameters;
};

} // namespace pExplore

#endif // PEXPLORE_TASK_RANKING_SPACE_HPP
