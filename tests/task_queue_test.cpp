#include <serialpp.hpp>
#include <gtest/gtest.h>

using namespace spp;

TEST(TaskQueueTest, EnqueueTest)
{
    TaskQueue tq;
    int x1 = 11;
    int x2 = 22;
    auto fut1 = tq.enqueue([](int x1, int x2) -> int {
        return x1 + x2;
    }, x1, x2);
    int sum = fut1.get();
    ASSERT_EQ(sum, x1 + x2);
}
