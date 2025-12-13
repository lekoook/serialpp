#include <serialpp.hpp>
#include <gtest/gtest.h>

using namespace spp;

class TaskQueueTest : public testing::Test {

protected:
    TaskQueue tq_;
    static constexpr int x1_ = 22;
    static constexpr int x2_ = 11;
    static constexpr int xa3_ = x1_ + x2_;
    static constexpr int xs3_ = x1_ - x2_;
    static constexpr int xm3_ = x1_ * x2_;

    static constexpr double y1_ = 321.321;
    static constexpr double y2_ = 123.123;
    static constexpr double ya3_ = y1_ + y2_;
    static constexpr double ys3_ = y1_ - y2_;
    static constexpr double ym3_ = y1_ * y2_;

    template<typename T>
    static T add_(T t1, T t2)
    {
        return t1 + t2;
    }

    template<typename T>
    static T sub_(T t1, T t2)
    {
        return t1 - t2;
    }

    template<typename T>
    static T mul_(T t1, T t2)
    {
        return t1 * t2;
    }
};

TEST_F(TaskQueueTest, EnqueueTest)
{
    auto fut1 = tq_.enqueue(&TaskQueueTest::add_<decltype(x1_)>, x1_, x2_);
    ASSERT_EQ(fut1.get(), xa3_);
}

TEST_F(TaskQueueTest, MultipleEnqueueTest)
{
    auto fut1 = tq_.enqueue(&TaskQueueTest::add_<decltype(y1_)>, y1_, y2_);
    auto fut2 = tq_.enqueue(&TaskQueueTest::sub_<decltype(y1_)>, y1_, y2_);
    auto fut3 = tq_.enqueue(&TaskQueueTest::mul_<decltype(y1_)>, y1_, y2_);
    ASSERT_EQ(fut1.get(), ya3_);
    ASSERT_EQ(fut2.get(), ys3_);
    ASSERT_EQ(fut3.get(), ym3_);
}

TEST_F(TaskQueueTest, ManyMultipleEnqueueTest)
{
    int iterations = 10000;
    int sum = 0;
    std::future<void> fut;
    for (int i = 0; i < iterations; i++) {
        fut = tq_.enqueue([&]() {
            sum++;
        });
    }
    fut.get();
    ASSERT_EQ(sum, iterations);
}

TEST_F(TaskQueueTest, RepeatTest)
{
    int iterations = 10000;
    int sum = 0;
    tq_.startRepeat([&]() {
        sum++;
        if (sum == iterations) {
            tq_.stopRepeat();
        }
    });
    // NOTE: This can block the testing indefinitely if repeat test fails entirely.
    while (sum != iterations){}
    ASSERT_EQ(sum, iterations);
}
