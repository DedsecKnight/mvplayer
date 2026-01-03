#include <print>

#include "engine.hpp"
#include "events/handler.hpp"

struct ping_event {};
struct pong_event {};

class ping_processor : public multithreaded::events::handlers<ping_event> {
 public:
  void operator()(const ping_event&) override {
    std::println("Received ping event");
    // TODO: send back pong event
  }
  void on_startup(std::span<char* const>) const noexcept {
    std::println("Hi from ping");
  }
};

class pong_processor : public multithreaded::events::handlers<pong_event> {
 public:
  void operator()(const pong_event&) {
    std::println("Received pong event ");
    // TODO: send ping event
  }
  void on_startup(std::span<char* const>) const noexcept {
    // TODO: send ping event
    std::println("Hi from pong");
  }
};

int main(int argc, char** argv) {
  multithreaded::engine e{};
  using namespace std::literals;

  auto p1 = e.create_processor<ping_processor>("p1"sv);
  auto p2 = e.create_processor<pong_processor>("p2"sv);

  p2.subscribe_to<pong_event>(p1);
  p1.subscribe_to<ping_event>(p2);

  e.start(std::span(argv, argc));

  // TODO: implement stop mechanism
  while (true) {
  }
}
