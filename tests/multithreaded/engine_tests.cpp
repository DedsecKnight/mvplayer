#include <gtest/gtest.h>

#include "engine/engine.hpp"
#include "events/handler.hpp"

template <typename event_t>
using envelope = typename multithreaded::events::envelope<event_t>;

TEST(EngineTest, ProcessorRefName) {
  struct mock_processor : public multithreaded::events::handlers<int32_t> {
    void operator()(const envelope<int32_t>&) override {}
    void on_startup(std::span<char* const>) const noexcept {}
  };
  multithreaded::engine e{};
  auto p1 = e.create_processor<mock_processor>("p1");
  EXPECT_EQ(p1.name(), "p1");
}
