#include "utils/queue.hpp"

#include <gtest/gtest.h>

#include <string>

struct mock_call_tracker {
  static void invoke_mock_creation() {
    std::scoped_lock lk{m};
    alloc_counter++;
  }
  static void invoke_mock_destruction() {
    std::scoped_lock lk{m};
    dealloc_counter++;
  }
  static void reset_counter() { alloc_counter = dealloc_counter = 0; }
  static std::mutex m;
  static int32_t alloc_counter, dealloc_counter;
};

int32_t mock_call_tracker::alloc_counter = 0;
int32_t mock_call_tracker::dealloc_counter = 0;
std::mutex mock_call_tracker::m{};

class mock_object : public mock_call_tracker {
 public:
  mock_object() = default;
  mock_object(const mock_object&) { invoke_mock_creation(); }
  ~mock_object() { invoke_mock_destruction(); }
};

TEST(SPSCQueueTest, BasicDataType) {
  multithreaded::utils::spsc_queue<int> queue{};
  std::thread producer{[&queue]() {
    for (int i = 0; i < 3; i++) {
      EXPECT_EQ(queue.push(i), true);
    }
  }};
  std::thread consumer{[&queue]() {
    int val;
    for (int i = 0; i < 3; i++) {
      while (!queue.pop(val)) {
      }
      EXPECT_EQ(val, i);
    }
  }};
  if (producer.joinable()) producer.join();
  if (consumer.joinable()) consumer.join();
}

TEST(SPSCQueueTest, PushFullQueue) {
  multithreaded::utils::spsc_queue<int> queue{};
  for (int i = 0; i < 3; i++) {
    EXPECT_EQ(queue.push(i), true);
  }
  EXPECT_EQ(queue.push(3), false);
}

TEST(SPSCQueueTest, PopEmptyQueue) {
  multithreaded::utils::spsc_queue<int> queue{};
  int val;
  EXPECT_EQ(queue.pop(val), false);
}

TEST(SPSCQueueTest, LargeDataStream) {
  std::vector<int> random_values(100'000);
  for (auto& elem : random_values) {
    elem = rand();
  }
  multithreaded::utils::spsc_queue<int> queue{131071};
  std::thread producer{[&queue, &random_values]() {
    for (size_t i{}; i < random_values.size(); i++) {
      EXPECT_EQ(queue.push(random_values[i]), true);
    }
  }};
  std::thread consumer{[&queue, &random_values]() {
    int val;
    for (size_t i{}; i < random_values.size(); i++) {
      while (!queue.pop(val)) {
      }
      EXPECT_EQ(val, random_values[i]);
    }
  }};
  if (producer.joinable()) producer.join();
  if (consumer.joinable()) consumer.join();
}

TEST(SPSCQueueTest, ComplexDataType) {
  multithreaded::utils::spsc_queue<std::string> queue{};
  std::thread producer{[&queue]() {
    EXPECT_EQ(queue.push("hello"), true);
    EXPECT_EQ(queue.push("world"), true);
  }};
  std::thread consumer{[&queue]() {
    std::string val;
    while (!queue.pop(val)) {
    }
    EXPECT_EQ(val, "hello");
    while (!queue.pop(val)) {
    }
    EXPECT_EQ(val, "world");
  }};
  if (producer.joinable()) producer.join();
  if (consumer.joinable()) consumer.join();
}

TEST(SPSCQueueTest, TestCopyConstructorInvocation) {
  mock_call_tracker::reset_counter();
  multithreaded::utils::spsc_queue<mock_object> queue{};
  std::thread producer{
      [&queue]() { EXPECT_EQ(queue.push(mock_object{}), true); }};
  std::thread consumer{[&queue]() {
    mock_object val;
    while (!queue.pop(val)) {
    }
  }};
  if (producer.joinable()) producer.join();
  if (consumer.joinable()) consumer.join();
  EXPECT_EQ(mock_object::alloc_counter, 2);
}

TEST(SPSCQueueTest, TestDestructorInvocation) {
  mock_call_tracker::reset_counter();
  multithreaded::utils::spsc_queue<mock_object> queue{};
  std::thread producer{
      [&queue]() { EXPECT_EQ(queue.push(mock_object{}), true); }};
  std::thread consumer{[&queue]() {
    mock_object val;
    while (!queue.pop(val)) {
    }
  }};
  if (producer.joinable()) producer.join();
  if (consumer.joinable()) consumer.join();
  EXPECT_EQ(mock_object::dealloc_counter, 3);
}
