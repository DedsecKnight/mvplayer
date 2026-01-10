#include <print>
#include <thread>

#include "engine/engine.hpp"
#include "events/envelope.hpp"
#include "events/handler.hpp"

using namespace std::chrono_literals;

struct ping_event {};
struct pong_event {};

class ping_processor : public multithreaded::events::handlers<ping_event> {
 public:
  void operator()(
      const multithreaded::events::envelope<ping_event>& e) override {
    std::println("[{}] Received ping event from {}", std::this_thread::get_id(),
                 e.sender().name());
    e.reply(pong_event{});
  }
  void on_startup(std::span<char* const>) noexcept {
    std::println("Hi from ping");
  }
};

class pong_processor : public multithreaded::events::handlers<pong_event> {
 public:
  void operator()(const multithreaded::events::envelope<pong_event>& e) {
    std::println("[{}] Received pong event from {}", std::this_thread::get_id(),
                 e.sender().name());
    std::this_thread::sleep_for(5s);
    e.reply(ping_event{});
  }
  void on_startup(std::span<char* const>) noexcept {
    std::println("Hi from pong");
    broadcast(ping_event{});
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
