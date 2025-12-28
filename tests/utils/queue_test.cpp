#include "utils/queue.hpp"

#include <gtest/gtest.h>

#include <string>

struct MockCallTracker {
  static void invokeMockCreation() {
    std::scoped_lock lk{m};
    allocCounter++;
  }
  static void invokeMockDestruction() {
    std::scoped_lock lk{m};
    deallocCounter++;
  }
  static void resetCounter() { allocCounter = deallocCounter = 0; }
  static std::mutex m;
  static int32_t allocCounter, deallocCounter;
};

int32_t MockCallTracker::allocCounter = 0;
int32_t MockCallTracker::deallocCounter = 0;
std::mutex MockCallTracker::m{};

class MockObject : public MockCallTracker {
 public:
  MockObject() = default;
  MockObject(const MockObject&) { invokeMockCreation(); }
  ~MockObject() { invokeMockDestruction(); }
};

TEST(SPSCQueueTest, BasicDataType) {
  mvplayer::utils::SPSCQueue<int> queue{};
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
  mvplayer::utils::SPSCQueue<int> queue{};
  for (int i = 0; i < 3; i++) {
    EXPECT_EQ(queue.push(i), true);
  }
  EXPECT_EQ(queue.push(3), false);
}

TEST(SPSCQueueTest, PopEmptyQueue) {
  mvplayer::utils::SPSCQueue<int> queue{};
  int val;
  EXPECT_EQ(queue.pop(val), false);
}

TEST(SPSCQueueTest, LargeDataStream) {
  std::vector<int> randomValues(100'000);
  for (auto& elem : randomValues) {
    elem = rand();
  }
  mvplayer::utils::SPSCQueue<int, 131071> queue{};
  std::thread producer{[&queue, &randomValues]() {
    for (size_t i{}; i < randomValues.size(); i++) {
      EXPECT_EQ(queue.push(randomValues[i]), true);
    }
  }};
  std::thread consumer{[&queue, &randomValues]() {
    int val;
    for (size_t i{}; i < randomValues.size(); i++) {
      while (!queue.pop(val)) {
      }
      EXPECT_EQ(val, randomValues[i]);
    }
  }};
  if (producer.joinable()) producer.join();
  if (consumer.joinable()) consumer.join();
}

TEST(SPSCQueueTest, ComplexDataType) {
  mvplayer::utils::SPSCQueue<std::string> queue{};
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
  MockCallTracker::resetCounter();
  mvplayer::utils::SPSCQueue<MockObject> queue{};
  std::thread producer{
      [&queue]() { EXPECT_EQ(queue.push(MockObject{}), true); }};
  std::thread consumer{[&queue]() {
    MockObject val;
    while (!queue.pop(val)) {
    }
  }};
  if (producer.joinable()) producer.join();
  if (consumer.joinable()) consumer.join();
  EXPECT_EQ(MockObject::allocCounter, 2);
}

TEST(SPSCQueueTest, TestDestructorInvocation) {
  MockCallTracker::resetCounter();
  mvplayer::utils::SPSCQueue<MockObject> queue{};
  std::thread producer{
      [&queue]() { EXPECT_EQ(queue.push(MockObject{}), true); }};
  std::thread consumer{[&queue]() {
    MockObject val;
    while (!queue.pop(val)) {
    }
  }};
  if (producer.joinable()) producer.join();
  if (consumer.joinable()) consumer.join();
  EXPECT_EQ(MockObject::deallocCounter, 3);
}
