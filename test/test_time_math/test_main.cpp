#include <stdint.h>
#include <unity.h>

#include "../../lib/fw_core/src/core/TimeMath.h"

namespace {

void test_deadline_is_not_reached_before_target()
{
  TEST_ASSERT_FALSE(deadlineReached(999U, 1000U));
}

void test_deadline_is_reached_at_and_after_target()
{
  TEST_ASSERT_TRUE(deadlineReached(1000U, 1000U));
  TEST_ASSERT_TRUE(deadlineReached(1001U, 1000U));
}

void test_deadline_comparison_is_overflow_safe()
{
  const uint32_t deadline = 10U;
  TEST_ASSERT_FALSE(deadlineReached(UINT32_MAX - 5U, deadline));
  TEST_ASSERT_TRUE(deadlineReached(10U, deadline));
  TEST_ASSERT_TRUE(deadlineReached(11U, deadline));
}

} // namespace

int main()
{
  UNITY_BEGIN();
  RUN_TEST(test_deadline_is_not_reached_before_target);
  RUN_TEST(test_deadline_is_reached_at_and_after_target);
  RUN_TEST(test_deadline_comparison_is_overflow_safe);
  return UNITY_END();
}
