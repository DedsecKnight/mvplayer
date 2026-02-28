#include <gtest/gtest.h>

#include <thread>
#include <variant>

#include "connector/connector.hpp"
#include "engine/engine.hpp"
#include "events/envelope.hpp"
#include "events/handler.hpp"
#include "processor/pre_event_handler.hpp"
#include "processor/termination_handler.hpp"

template <typename event_t>
struct mock_event {
  event_t x{};
};

struct mock_terminate_event {};

struct mock_processor
    : public multithreaded::events::handlers<mock_event<int32_t>,
                                             mock_terminate_event>,
      public multithreaded::processor::pre_event_handler,
      public multithreaded::processor::termination_handler {
  template <typename event_t>
  using envelope = typename multithreaded::events::envelope<event_t>;

  void operator()(const envelope<mock_event<int32_t>>& e) override {
    event_received = true;
    senders.push_back(e.sender().name());
    received_payload.push_back(e.payload().x);
  }

  void operator()(const envelope<mock_terminate_event>& e) override {
    event_received = true;
    EXPECT_EQ(request_termination(), true);
  }

  void on_startup(std::span<char* const>) const noexcept {}

  void pre_event() noexcept override { pre_event_invoked = true; }

  void handle_termination_signal() noexcept override {
    termination_handler_invoked = true;
  }

  static bool event_received, pre_event_invoked, termination_handler_invoked;
  static std::vector<int32_t> received_payload;
  static std::vector<std::string_view> senders;
};

struct variadic_mock_processor
    : public multithreaded::events::handlers<mock_event<int32_t>,
                                             mock_event<double>> {
  template <typename event_t>
  using envelope = typename multithreaded::events::envelope<event_t>;

  void operator()(const envelope<mock_event<int32_t>>& e) override {
    event_received = true;
    received_payload.emplace_back(e.payload().x);
  }
  void operator()(const envelope<mock_event<double>>& e) override {
    event_received = true;
    received_payload.emplace_back(e.payload().x);
  }
  void on_startup(std::span<char* const>) const noexcept {}

  static bool event_received;
  static std::vector<std::variant<int32_t, double>> received_payload;
};

bool mock_processor::event_received = false;
bool mock_processor::pre_event_invoked = false;
bool mock_processor::termination_handler_invoked = false;
std::vector<int32_t> mock_processor::received_payload{};
std::vector<std::string_view> mock_processor::senders{};

bool variadic_mock_processor::event_received = false;
std::vector<std::variant<int32_t, double>>
    variadic_mock_processor::received_payload{};

struct ping_event {};
struct pong_event {};

class ping_processor : public multithreaded::events::handlers<ping_event> {
 public:
  void operator()(
      const multithreaded::events::envelope<ping_event>& e) override {
    received_event = true;
    e.reply(pong_event{});
  }
  void on_startup(std::span<char* const>) const noexcept {}

  static bool received_event;
};

struct pong_processor : public multithreaded::events::handlers<pong_event> {
  void operator()(const multithreaded::events::envelope<pong_event>&) {
    received_event = true;
  }
  void on_startup(std::span<char* const> args) noexcept {
    if (args.empty()) {
      broadcast(ping_event{});
    }
  }
  static bool received_event;
};

bool ping_processor::received_event = false;
bool pong_processor::received_event = false;

TEST(MultithreadedTests, HandleEmptyMessage) {
  using event_t = mock_event<int32_t>;
  multithreaded::engine e{};
  std::thread engine_stopper{[&e]() {
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(5ms);
    e.terminate();
  }};
  using namespace std::literals;
  multithreaded::connector conn{multithreaded::connector_event_set<event_t>{}};
  auto read_port = conn.as_connector_of<event_t>().get_read_port();
  auto write_port = conn.as_connector_of<event_t>().get_write_port();

  auto p1 = e.create_processor<mock_processor>("p1");
  auto& casted_p1 = p1.get().as<mock_processor>();
  casted_p1.add_read_port("", std::move(read_port));

  std::vector<char*> args{nullptr};
  EXPECT_EQ(write_port.push(event_t{}), true);

  e.start(args);
  if (engine_stopper.joinable()) {
    engine_stopper.join();
  }

  EXPECT_EQ(mock_processor::event_received, true);
}

TEST(MultithreadedTests, HandleNonEmptyMessage) {
  using event_t = mock_event<int32_t>;
  mock_processor::event_received = false;
  mock_processor::received_payload.clear();
  multithreaded::engine e{};
  std::thread engine_stopper{[&e]() {
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(5ms);
    e.terminate();
  }};
  using namespace std::literals;
  multithreaded::connector conn{multithreaded::connector_event_set<event_t>{}};
  auto read_port = conn.as_connector_of<event_t>().get_read_port();
  auto write_port = conn.as_connector_of<event_t>().get_write_port();

  auto p1 = e.create_processor<mock_processor>("p1");
  auto& casted_p1 = p1.get().as<mock_processor>();
  casted_p1.add_read_port("", std::move(read_port));

  std::vector<char*> args{nullptr};
  EXPECT_EQ(write_port.push(event_t{5}), true);
  e.start(args);
  if (engine_stopper.joinable()) {
    engine_stopper.join();
  }

  EXPECT_EQ(mock_processor::event_received, true);
  EXPECT_EQ(mock_processor::received_payload.size(), 1);
  EXPECT_EQ(mock_processor::received_payload[0], 5);
}

TEST(MultithreadedTests, ProcessorWithMultipleEventHandlers) {
  variadic_mock_processor::event_received = false;
  variadic_mock_processor::received_payload.clear();
  multithreaded::engine e{};
  std::thread engine_stopper{[&e]() {
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(10ms);
    e.terminate();
  }};
  using namespace std::literals;
  multithreaded::connector conn{
      multithreaded::connector_event_set<mock_event<int32_t>,
                                         mock_event<double>>{}};
  auto read_port =
      conn.as_connector_of<mock_event<int32_t>, mock_event<double>>()
          .get_read_port();
  auto write_port =
      conn.as_connector_of<mock_event<int32_t>, mock_event<double>>()
          .get_write_port();

  auto p1 = e.create_processor<variadic_mock_processor>("p1");
  auto& casted_p1 = p1.get().as<variadic_mock_processor>();
  casted_p1.add_read_port("", std::move(read_port));

  EXPECT_EQ(write_port.push(mock_event<int32_t>{5}), true);
  EXPECT_EQ(write_port.push(mock_event<double>{2.0}), true);

  std::vector<char*> args{nullptr};
  e.start(args);

  if (engine_stopper.joinable()) {
    engine_stopper.join();
  }

  EXPECT_EQ(variadic_mock_processor::event_received, true);
  EXPECT_EQ(variadic_mock_processor::received_payload.size(), 2);
  EXPECT_EQ(std::get<int32_t>(variadic_mock_processor::received_payload[0]), 5);
  EXPECT_EQ(std::get<double>(variadic_mock_processor::received_payload[1]),
            2.0);
}

TEST(MultithreadedTests, TestRecipientName) {
  using event_t = mock_event<int32_t>;
  mock_processor::event_received = false;
  mock_processor::received_payload.clear();
  mock_processor::senders.clear();
  multithreaded::engine e{};
  std::thread engine_stopper{[&e]() {
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(5ms);
    e.terminate();
  }};
  using namespace std::literals;
  multithreaded::connector conn{multithreaded::connector_event_set<event_t>{}};
  auto read_port = conn.as_connector_of<event_t>().get_read_port();
  auto write_port = conn.as_connector_of<event_t>().get_write_port();

  auto p1 = e.create_processor<mock_processor>("p1");
  auto& casted_p1 = p1.get().as<mock_processor>();
  casted_p1.add_read_port("main-thread", std::move(read_port));

  std::vector<char*> args{nullptr};
  EXPECT_EQ(write_port.push(event_t{5}), true);
  e.start(args);
  if (engine_stopper.joinable()) {
    engine_stopper.join();
  }

  EXPECT_EQ(mock_processor::senders.size(), 1);
  EXPECT_EQ(mock_processor::senders[0], "main-thread");
}

TEST(MultithreadedTests, TestReplyMechanism) {
  ping_processor::received_event = false;
  pong_processor::received_event = false;

  multithreaded::engine e{};
  std::thread engine_stopper{[&e]() {
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(10ms);
    e.terminate();
  }};
  using namespace std::literals;

  auto ping = e.create_processor<ping_processor>("ping");
  auto pong = e.create_processor<pong_processor>("pong");
  auto& casted_ping = ping.get().as<ping_processor>();
  auto& casted_pong = pong.get().as<pong_processor>();

  multithreaded::connector ping_conn{
      multithreaded::connector_event_set<ping_event>{}};
  auto ping_read_port = ping_conn.as_connector_of<ping_event>().get_read_port();
  auto ping_write_port =
      ping_conn.as_connector_of<ping_event>().get_write_port();

  multithreaded::connector pong_conn{
      multithreaded::connector_event_set<pong_event>{}};
  auto pong_read_port = pong_conn.as_connector_of<pong_event>().get_read_port();
  auto pong_write_port =
      pong_conn.as_connector_of<pong_event>().get_write_port();

  EXPECT_EQ(ping_write_port.push(ping_event{}), true);

  casted_ping.add_read_port("pong", std::move(ping_read_port));
  casted_ping.add_write_port("pong", std::move(pong_write_port));

  casted_pong.add_read_port("ping", std::move(pong_read_port));
  casted_pong.add_write_port("ping", std::move(ping_write_port));

  std::vector<char*> args{nullptr};
  e.start(args);
  if (engine_stopper.joinable()) {
    engine_stopper.join();
  }

  EXPECT_EQ(ping_processor::received_event, true);
  EXPECT_EQ(pong_processor::received_event, true);
}

TEST(MultithreadedTests, TestBroadcastMechanism) {
  ping_processor::received_event = false;
  pong_processor::received_event = false;
  multithreaded::engine e{};
  std::thread engine_stopper{[&e]() {
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(10ms);
    e.terminate();
  }};
  using namespace std::literals;

  auto ping = e.create_processor<ping_processor>("ping");
  auto pong = e.create_processor<pong_processor>("pong");
  auto& casted_ping = ping.get().as<ping_processor>();
  auto& casted_pong = pong.get().as<pong_processor>();

  multithreaded::connector ping_conn{
      multithreaded::connector_event_set<ping_event>{}};
  auto ping_read_port = ping_conn.as_connector_of<ping_event>().get_read_port();
  auto ping_write_port =
      ping_conn.as_connector_of<ping_event>().get_write_port();

  multithreaded::connector pong_conn{
      multithreaded::connector_event_set<pong_event>{}};
  auto pong_read_port = pong_conn.as_connector_of<pong_event>().get_read_port();
  auto pong_write_port =
      pong_conn.as_connector_of<pong_event>().get_write_port();

  casted_ping.add_read_port("pong", std::move(ping_read_port));
  casted_ping.add_write_port("pong", std::move(pong_write_port));

  casted_pong.add_read_port("ping", std::move(pong_read_port));
  casted_pong.add_write_port("ping", std::move(ping_write_port));

  std::vector<char*> args{};
  e.start(args);
  if (engine_stopper.joinable()) {
    engine_stopper.join();
  }

  EXPECT_EQ(ping_processor::received_event, true);
  EXPECT_EQ(pong_processor::received_event, true);
}

TEST(MultithreadedTests, TestRequestTermination) {
  mock_processor::event_received = false;
  mock_processor::termination_handler_invoked = false;
  mock_processor::received_payload.clear();
  mock_processor::senders.clear();
  multithreaded::engine e{};
  using namespace std::literals;
  multithreaded::connector conn{
      multithreaded::connector_event_set<mock_terminate_event>{}};
  auto read_port = conn.as_connector_of<mock_terminate_event>().get_read_port();
  auto write_port =
      conn.as_connector_of<mock_terminate_event>().get_write_port();

  auto p1 = e.create_processor<mock_processor>("p1");
  auto& casted_p1 = p1.get().as<mock_processor>();
  casted_p1.add_read_port("main-thread", std::move(read_port));

  std::vector<char*> args{nullptr};
  EXPECT_EQ(write_port.push(mock_terminate_event{}), true);

  e.start(args);
  EXPECT_EQ(e.terminated(), true);
  EXPECT_EQ(mock_processor::event_received, true);
  EXPECT_EQ(mock_processor::termination_handler_invoked, true);
}

TEST(MultithreadedTests, TestPreEventHandler) {
  mock_processor::pre_event_invoked = false;
  multithreaded::engine e{};
  using namespace std::literals;
  multithreaded::connector conn{
      multithreaded::connector_event_set<mock_terminate_event>{}};
  auto read_port = conn.as_connector_of<mock_terminate_event>().get_read_port();
  auto write_port =
      conn.as_connector_of<mock_terminate_event>().get_write_port();

  auto p1 = e.create_processor<mock_processor>("p1");
  auto& casted_p1 = p1.get().as<mock_processor>();
  casted_p1.add_read_port("main-thread", std::move(read_port));

  std::vector<char*> args{nullptr};
  EXPECT_EQ(write_port.push(mock_terminate_event{}), true);

  e.start(args);
  EXPECT_EQ(mock_processor::pre_event_invoked, true);
}
