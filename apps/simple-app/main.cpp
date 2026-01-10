#include <print>

#include "engine/engine.hpp"
#include "events/envelope.hpp"
#include "events/handler.hpp"

struct ping_event {};
struct pong_event {};

class ping_processor : public multithreaded::events::handlers<ping_event> {
 public:
  void operator()(
      const multithreaded::events::envelope<ping_event>& e) override {
    std::println("Received ping event from {}", e.sender().name());
    e.reply(pong_event{});
  }
  void on_startup(std::span<char* const>) noexcept {
    std::println("Hi from ping");
    event_handler_t::broadcast(pong_event{});
  }
};

class pong_processor : public multithreaded::events::handlers<pong_event> {
 public:
  void operator()(const multithreaded::events::envelope<pong_event>& e) {
    std::println("Received pong event from {}", e.sender().name());
    // TODO: send ping event
  }
  void on_startup(std::span<char* const>) const noexcept {
    // TODO: send ping event
    std::println("Hi from pong");
  }
};

int main(int argc, char** argv) {
  multithreaded::engine e{};

  auto p1 = e.create_processor<ping_processor>("p1");
  auto p2 = e.create_processor<pong_processor>("p2");

  p2.subscribe_to<pong_event>(p1);
  p1.subscribe_to<ping_event>(p2);

  e.start(std::span(argv, argc));

  // TODO: implement stop mechanism
  while (true) {
  }
}
