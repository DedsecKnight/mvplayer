#include <print>

#include "engine.hpp"

struct ping_event {};
struct pong_event {};

class ping_processor {
 public:
  void handle_event(const ping_event&) { std::println("Received ping event"); }
  void on_startup(std::span<char* const>) const noexcept {
    std::println("Hi from ping");
  }
};

class pong_processor {
 public:
  void handle_event(const pong_event&) { std::println("Received pong event "); }
  void on_startup(std::span<char* const>) const noexcept {
    std::println("Hi from pong");
  }
};

int main(int argc, char** argv) {
  multithreaded::engine e{};
  using namespace std::literals;

  auto p1 = e.create_processor<ping_processor>("p1"sv);
  auto p2 = e.create_processor<pong_processor>("p2"sv);

  p1.subscribe_to<pong_event>(p2);
  p2.subscribe_to<ping_event>(p1);

  e.start(std::span(argv, argc));

  // TODO: implement stop mechanism
  while (true) {
  }
}
