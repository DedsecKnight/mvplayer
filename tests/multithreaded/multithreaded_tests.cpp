#include <gtest/gtest.h>

#include <variant>

#include "connector/connector.hpp"
#include "engine.hpp"
#include "events/envelope.hpp"
#include "events/handler.hpp"

template <typename event_t>
struct mock_event {
  event_t x{};
};

struct mock_processor
    : public multithreaded::events::handlers<mock_event<int32_t>> {
  template <typename event_t>
  using envelope = typename multithreaded::events::envelope<event_t>;

  void operator()(const envelope<mock_event<int32_t>>& e) override {
    event_received = true;
    senders.push_back(e.sender_name);
    received_payload.push_back(e.payload.x);
  }
  void on_startup(std::span<char* const>) const noexcept {}

  static bool event_received;
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
    received_payload.emplace_back(e.payload.x);
  }
  void operator()(const envelope<mock_event<double>>& e) override {
    event_received = true;
    received_payload.emplace_back(e.payload.x);
  }
  void on_startup(std::span<char* const>) const noexcept {}

  static bool event_received;
  static std::vector<std::variant<int32_t, double>> received_payload;
};

bool mock_processor::event_received = false;
std::vector<int32_t> mock_processor::received_payload{};
std::vector<std::string_view> mock_processor::senders{};

bool variadic_mock_processor::event_received = false;
std::vector<std::variant<int32_t, double>>
    variadic_mock_processor::received_payload{};

TEST(MultithreadedTests, HandleEmptyMessage) {
  using event_t = mock_event<int32_t>;
  {
    multithreaded::engine e{};
    using namespace std::literals;
    multithreaded::connector conn{
        multithreaded::connector_event_set<event_t>{}};
    auto read_port = conn.as_connector_of<event_t>().get_read_port();
    auto write_port = conn.as_connector_of<event_t>().get_write_port();

    auto p1 = e.create_processor<mock_processor>("p1");
    auto& casted_p1 = p1.get().as<mock_processor>();
    casted_p1.add_read_port("", std::move(read_port));

    std::vector<char*> args{nullptr};
    e.start(args);

    std::variant<event_t> event = event_t{};
    EXPECT_EQ(write_port.push(event), true);

    using namespace std::chrono_literals;
    std::this_thread::sleep_for(5ms);
  }

  EXPECT_EQ(mock_processor::event_received, true);
}

TEST(MultithreadedTests, HandleNonEmptyMessage) {
  using event_t = mock_event<int32_t>;
  {
    mock_processor::event_received = false;
    mock_processor::received_payload.clear();
    multithreaded::engine e{};
    using namespace std::literals;
    multithreaded::connector conn{
        multithreaded::connector_event_set<event_t>{}};
    auto read_port = conn.as_connector_of<event_t>().get_read_port();
    auto write_port = conn.as_connector_of<event_t>().get_write_port();

    auto p1 = e.create_processor<mock_processor>("p1");
    auto& casted_p1 = p1.get().as<mock_processor>();
    casted_p1.add_read_port("", std::move(read_port));

    std::vector<char*> args{nullptr};
    e.start(args);

    std::variant<event_t> event = event_t{5};
    EXPECT_EQ(write_port.push(event), true);

    using namespace std::chrono_literals;
    std::this_thread::sleep_for(5ms);
  }

  EXPECT_EQ(mock_processor::event_received, true);
  EXPECT_EQ(mock_processor::received_payload.size(), 1);
  EXPECT_EQ(mock_processor::received_payload[0], 5);
}

TEST(MultithreadedTests, ProcessorWithMultipleEventHandlers) {
  {
    variadic_mock_processor::event_received = false;
    variadic_mock_processor::received_payload.clear();
    multithreaded::engine e{};
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

    std::vector<char*> args{nullptr};
    e.start(args);

    std::variant<mock_event<int32_t>, mock_event<double>> event =
        mock_event<int32_t>{5};
    EXPECT_EQ(write_port.push(event), true);
    event = mock_event<double>{2.0};
    EXPECT_EQ(std::holds_alternative<mock_event<double>>(event), true);
    EXPECT_EQ(write_port.push(event), true);

    using namespace std::chrono_literals;
    std::this_thread::sleep_for(10ms);
  }

  EXPECT_EQ(variadic_mock_processor::event_received, true);
  EXPECT_EQ(variadic_mock_processor::received_payload.size(), 2);
  EXPECT_EQ(std::get<int32_t>(variadic_mock_processor::received_payload[0]), 5);
  EXPECT_EQ(std::get<double>(variadic_mock_processor::received_payload[1]),
            2.0);
}

TEST(MultithreadedTests, TestRecipientName) {
  using event_t = mock_event<int32_t>;
  {
    mock_processor::event_received = false;
    mock_processor::received_payload.clear();
    mock_processor::senders.clear();
    multithreaded::engine e{};
    using namespace std::literals;
    multithreaded::connector conn{
        multithreaded::connector_event_set<event_t>{}};
    auto read_port = conn.as_connector_of<event_t>().get_read_port();
    auto write_port = conn.as_connector_of<event_t>().get_write_port();

    auto p1 = e.create_processor<mock_processor>("p1");
    auto& casted_p1 = p1.get().as<mock_processor>();
    casted_p1.add_read_port("main-thread", std::move(read_port));

    std::vector<char*> args{nullptr};
    e.start(args);

    std::variant<event_t> event = event_t{5};
    EXPECT_EQ(write_port.push(event), true);

    using namespace std::chrono_literals;
    std::this_thread::sleep_for(5ms);
  }

  EXPECT_EQ(mock_processor::senders.size(), 1);
  EXPECT_EQ(mock_processor::senders[0], "main-thread");
}
