/***************************************************************************
 *            test_task_runner.cpp
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

#include "utility/test.hpp"
#include "pronest/searchable_configuration.hpp"
#include "pronest/configuration_property.tpl.hpp"
#include "pronest/configuration_search_space.hpp"
#include "pronest/configurable.tpl.hpp"
#include "betterthreads/thread_manager.hpp"
#include "task_runner_interface.hpp"
#include "task.tpl.hpp"
#include "task_runner.tpl.hpp"

using namespace std;
using namespace ProNest;
using namespace Utility;
using namespace pExplore;

class A;

enum class LevelOptions { LOW, MEDIUM, HIGH };
std::ostream& operator<<(std::ostream& os, const LevelOptions level) {
    switch(level) {
        case LevelOptions::LOW: os << "LOW"; return os;
        case LevelOptions::MEDIUM: os << "MEDIUM"; return os;
        case LevelOptions::HIGH: os << "HIGH"; return os;
        default: UTILITY_FAIL_MSG("Unhandled LevelOptions value");
    }
}

class TestConfigurable;

template<> struct Configuration<TestConfigurable> : public SearchableConfiguration {
  public:
    Configuration() { add_property("use_something",BooleanConfigurationProperty(true)); }
    bool const& use_something() const { return dynamic_cast<BooleanConfigurationProperty const&>(*properties().get("use_something")).get(); }
    void set_both_use_something() { dynamic_cast<BooleanConfigurationProperty&>(*properties().get("use_something")).set_both(); }
    void set_use_something(bool const& value) { dynamic_cast<BooleanConfigurationProperty&>(*properties().get("use_something")).set(value); }
};

class TestConfigurableInterface : public WritableInterface {
public:
    virtual TestConfigurableInterface* clone() const = 0;
    virtual void set_value(String value) = 0;
    virtual ~TestConfigurableInterface() = default;
};
class TestConfigurable : public TestConfigurableInterface, public Configurable<TestConfigurable> {
public:
    TestConfigurable(String value, Configuration<TestConfigurable> const& configuration) : TestConfigurable(configuration) { _value = value; }
    TestConfigurable(Configuration<TestConfigurable> const& configuration) : Configurable<TestConfigurable>(configuration) { }
    void set_value(String value) override { _value = value; }
    ostream& _write(ostream& os) const override { os << "TestConfigurable(value="<<_value<<",configuration=" << configuration() <<")"; return os; }
    TestConfigurableInterface* clone() const override { auto cfg = configuration(); return new TestConfigurable(_value,cfg); }
private:
    String _value;
};

using DoubleConfigurationProperty = RangeConfigurationProperty<double>;
using IntegerConfigurationProperty = RangeConfigurationProperty<int>;
using LevelOptionsConfigurationProperty = EnumConfigurationProperty<LevelOptions>;
using TestConfigurableConfigurationProperty = InterfaceListConfigurationProperty<TestConfigurableInterface>;
using Log10Converter = Log10SearchSpaceConverter<double>;
using Log2Converter = Log2SearchSpaceConverter<double>;

template<> struct Configuration<A> : public SearchableConfiguration {
  public:
    Configuration() {
        add_property("use_reconditioning",BooleanConfigurationProperty(false));
        add_property("maximum_order",IntegerConfigurationProperty(5));
        add_property("maximum_step_size",DoubleConfigurationProperty(std::numeric_limits<double>::infinity(),Log2Converter()));
        add_property("level",LevelOptionsConfigurationProperty(LevelOptions::LOW));
        add_property("test_configurable",TestConfigurableConfigurationProperty(TestConfigurable(Configuration<TestConfigurable>())));
    }

    bool const& use_reconditioning() const { return at<BooleanConfigurationProperty>("use_reconditioning").get(); }
    void set_both_use_reconditioning() { at<BooleanConfigurationProperty>("use_reconditioning").set_both(); }
    void set_use_reconditioning(bool const& value) { at<BooleanConfigurationProperty>("use_reconditioning").set(value); }

    int const& maximum_order() const { return at<IntegerConfigurationProperty>("maximum_order").get(); }
    void set_maximum_order(int const& value) { at<IntegerConfigurationProperty>("maximum_order").set(value); }
    void set_maximum_order(int const& lower, int const& upper) { at<IntegerConfigurationProperty>("maximum_order").set(lower,upper); }

    double const& maximum_step_size() const { return at<DoubleConfigurationProperty>("maximum_step_size").get(); }
    void set_maximum_step_size(double const& value) { at<DoubleConfigurationProperty>("maximum_step_size").set(value); }
    void set_maximum_step_size(double const& lower, double const& upper) { at<DoubleConfigurationProperty>("maximum_step_size").set(lower,upper); }

    LevelOptions const& level() const { return at<LevelOptionsConfigurationProperty>("level").get(); }
    void set_level(LevelOptions const& level) { at<LevelOptionsConfigurationProperty>("level").set(level); }
    void set_level(List<LevelOptions> const& levels) { at<LevelOptionsConfigurationProperty>("level").set(levels); }

    TestConfigurableInterface const& test_configurable() const { return at<TestConfigurableConfigurationProperty>("test_configurable").get(); }
    void set_test_configurable(TestConfigurableInterface const& test_configurable) { at<TestConfigurableConfigurationProperty>("test_configurable").set(test_configurable); }
    void set_test_configurable(shared_ptr<TestConfigurableInterface> const& test_configurable) { at<TestConfigurableConfigurationProperty>("test_configurable").set(test_configurable); }
};

template<> struct TaskInput<A> {
    TaskInput(double const& x_) : x(x_) { }
    double const& x;
};

template<> struct TaskOutput<A> {
    TaskOutput(double const& y_) : y(y_) { }
    double const y;
};

template<> struct TaskObjective<A> {
    TaskObjective(double const& obj_) : obj(obj_) { }
    double const obj;
};

template<> struct Task<A> final: public ParameterSearchTaskBase<A> {
    Task() : ParameterSearchTaskBase<A>("test") { }
    TaskOutput<A> run(TaskInput<A> const& in, Configuration<A> const& cfg) const override {
        double level_value = -2.0;
        switch (cfg.level()) {
            case LevelOptions::HIGH : level_value++;
            case LevelOptions::MEDIUM : level_value++;
            default : level_value++;
        }
        return {in.x + level_value + cfg.maximum_order() + cfg.maximum_step_size() + (cfg.use_reconditioning() ? 1.0 : 0.0) + (dynamic_cast<TestConfigurable const&>(cfg.test_configurable()).configuration().use_something() ? 1.0 : 0.0)};
    }
};

class A : public TaskRunnable<A>, public WritableInterface {
public:
    A(Configuration<A> const& config) : TaskRunnable<A>(config) { }
    ostream& _write(ostream& os) const override { os << "configuration:" << configuration(); return os; }

    List<double> execute() {
        List<double> result;
        for (size_t i=0; i<10; ++i) {
            runner()->push(TaskInput<A>(1.0));
            result.push_back(runner()->pull().y);
        }
        return result;
    }
};

class TestTaskRunner {
  public:

    void test_run() {

        BetterThreads::ThreadManager::instance();

        Configuration<A> ca;
        Configuration<TestConfigurable> ctc;
        ctc.set_both_use_something();
        TestConfigurable tc(ctc);
        ca.set_test_configurable(tc);
        ca.set_both_use_reconditioning();
        ca.set_maximum_order(1,5);
        ca.set_maximum_step_size(0.001,0.1);
        ca.set_level({LevelOptions::LOW,LevelOptions::MEDIUM});
        auto search_space = ca.search_space();
        UTILITY_TEST_PRINT(ca);
        UTILITY_TEST_PRINT(search_space);

        UTILITY_TEST_PRINT(TaskManager::instance().maximum_concurrency())
        TaskManager::instance().set_concurrency(8);

        A a(ca);

        List<Pair<TaskRankingParameter<A>,double>> specification;
        using I = TaskInput<A>;
        using O = TaskOutput<A>;
        using OBJ = TaskObjective<A>;
        OBJ empty_obj(1.0);
        auto constraint = ScalarObjectiveRankingParameter<A>("c", OptimisationCriterion::MAXIMISE, RankingConstraintSeverity::PERMISSIVE, empty_obj,
                                                                 [](I const&, O const& o, OBJ const&) { return o.y; },
                                                                 [](I const&, O const&, OBJ const&) { return 0.0; },
                                                                 [](I const&, OBJ const&) { return false; }
        );
        specification.push_back({constraint,1.0});
        TaskManager::instance().set_ranking_space_for(a,specification);

        auto result = a.execute();
        UTILITY_TEST_PRINT(result)
    }

    void test() {
        UTILITY_TEST_CALL(test_run());
    }
};

int main() {

    TestTaskRunner().test();
    return UTILITY_TEST_FAILURES;
}
