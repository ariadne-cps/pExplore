/***************************************************************************
 *            robustness_controller.hpp
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

/*! \file robustness_controller.hpp
 *  \brief Class for controllers for the robustness of a constraint.
 */

#ifndef PEXPLORE_ROBUSTNESS_CONTROLLER
#define PEXPLORE_ROBUSTNESS_CONTROLLER

#include <functional>

namespace pExplore {

template<class R> struct TaskInput;
template<class R> struct TaskOutput;

template<class R> class RobustnessControllerInterface {
  public:

    //! \brief Apply the control to the \a robustness value from a constraint, returning the controlled value
    //! \details The application may change the state of the controller, this is why the method is not const
    virtual double apply(double robustness, TaskInput<R> const& input, TaskOutput<R> const& output) = 0;

    virtual RobustnessControllerInterface<R>* clone() const = 0;
};

//! \brief Return the original robustness
template<class R> class IdentityRobustnessController : public RobustnessControllerInterface<R> {
  public:
    double apply(double robustness, TaskInput<R> const&, TaskOutput<R> const&) override { return robustness; }
    RobustnessControllerInterface<R>* clone() const override { return new IdentityRobustnessController(); }
};

//! \brief Spread the error in a linearly proportional way with respect to the time progressed
template<class R> class TimeProgressLinearRobustnessController : public RobustnessControllerInterface<R> {
  public:
    typedef std::function<double(TaskInput<R> const&, TaskOutput<R> const&)> TimeFunction;

    TimeProgressLinearRobustnessController(TimeFunction func, double final_time) : _t_func(func), _final_time(final_time), _accumulated_value(0.0) { }

    double apply(double robustness, TaskInput<R> const& input, TaskOutput<R> const& output) override {
        double current_time = _t_func(input,output);
        double result = robustness - (current_time-_previous_time) * _accumulated_value;
        _previous_time = current_time;
        _accumulated_value += result/(_final_time-current_time);
        return result;
    }
    RobustnessControllerInterface<R>* clone() const override { return new TimeProgressLinearRobustnessController(_t_func,_final_time); }

  private:
    TimeFunction _t_func;
    double _final_time;
    double _previous_time;
    double _accumulated_value;
};

} // namespace pExplore

#endif // PEXPLORE_ROBUSTNESS_CONTROLLER
