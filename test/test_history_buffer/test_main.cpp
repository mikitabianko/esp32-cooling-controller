#include <unity.h>

#include "../../lib/fw_telemetry/src/telemetry/HistoryBuffer.h"

namespace {

void test_history_buffer_retains_insertion_order()
{
  HistoryBuffer<int, 3> history;
  history.push(10);
  history.push(20);

  TEST_ASSERT_EQUAL_UINT32(3U, history.capacity());
  TEST_ASSERT_EQUAL_UINT32(2U, history.size());
  TEST_ASSERT_EQUAL_INT(10, history.at(0));
  TEST_ASSERT_EQUAL_INT(20, history.at(1));
}

void test_history_buffer_overwrites_oldest_sample()
{
  HistoryBuffer<int, 3> history;
  history.push(10);
  history.push(20);
  history.push(30);
  history.push(40);

  TEST_ASSERT_EQUAL_UINT32(3U, history.size());
  TEST_ASSERT_EQUAL_INT(20, history.at(0));
  TEST_ASSERT_EQUAL_INT(30, history.at(1));
  TEST_ASSERT_EQUAL_INT(40, history.at(2));
}

void test_history_buffer_can_be_cleared()
{
  HistoryBuffer<int, 2> history;
  history.push(10);
  history.clear();
  TEST_ASSERT_TRUE(history.empty());
  TEST_ASSERT_EQUAL_UINT32(0U, history.size());
}

} // namespace

int main()
{
  UNITY_BEGIN();
  RUN_TEST(test_history_buffer_retains_insertion_order);
  RUN_TEST(test_history_buffer_overwrites_oldest_sample);
  RUN_TEST(test_history_buffer_can_be_cleared);
  return UNITY_END();
}
