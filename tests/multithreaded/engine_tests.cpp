#include <gtest/gtest.h>

#include "engine.hpp"
#include "events/handler.hpp"

TEST(EngineTest, ProcessorRefName) {
  struct mock_processor : public multithreaded::events::handlers<int32_t> {
    void operator()(const int32_t&) override {}
    void on_startup(std::span<char* const>) const noexcept {}
  };
  multithreaded::engine e{};
  auto p1 = e.create_processor<mock_processor>("p1");
  EXPECT_EQ(p1.name(), "p1");
}
