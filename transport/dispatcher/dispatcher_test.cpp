// #include <catch2/catch_test_macros.hpp>
// #include <optional>
// #include <vector>
//
// #include "dispatcher.hpp"
//
// using namespace transport;
//
// struct MockTransporter {
//         MTU mtu = 64;
//         std::vector<Data> sent;
//         std::function<void(result::Result<Data>)> receiver;
//
//         result::Result<bool> send(Data &&data) {
//                 sent.push_back(data);
//                 return result::ok(true);
//         }
//
//         void set_receiver(std::function<void(result::Result<Data>)> cb) {
//                 receiver = std::move(cb);
//         }
//
//         MTU get_mtu() const {
//                 return mtu;
//         }
//
//         void deliver(Data data) {
//                 if (receiver) {
//                         receiver(result::ok(std::move(data)));
//                 }
//         }
// };
//
// TEST_CASE("Dispatcher send wraps and forwards data") {
//         MockTransporter mock;
//         Dispatcher<MockTransporter> dispatcher(mock);
//
//         Data data = {0xAA, 0xBB};
//         const auto result = dispatcher.send(0x0001, std::move(data));
//
//         REQUIRE(!result.failed());
//         REQUIRE(mock.sent.size() == 1);
//         REQUIRE(mock.sent[0].size() == sizeof(CommandId) + 2);
//         REQUIRE(mock.sent[0][0] == 0x01);
//         REQUIRE(mock.sent[0][1] == 0x00);
//         REQUIRE(mock.sent[0][2] == 0xAA);
//         REQUIRE(mock.sent[0][3] == 0xBB);
// }
//
// TEST_CASE("Dispatcher receive dispatches to registered handler") {
//         MockTransporter mock;
//         Dispatcher<MockTransporter> dispatcher(mock);
//
//         std::optional<Data> received;
//         dispatcher.register_handler(0x0001, [&](result::Result<Data> result)
//         {
//                 if (!result.failed()) {
//                         received = std::move(result).value();
//                 }
//         });
//
//         mock.deliver({0x01, 0x00, 0xAA, 0xBB});
//
//         REQUIRE(received.has_value());
//         REQUIRE(received.value() == Data{0xAA, 0xBB});
// }
//
// TEST_CASE("Dispatcher receive with no handler does not crash") {
//         MockTransporter mock;
//         Dispatcher<MockTransporter> dispatcher(mock);
//
//         REQUIRE_NOTHROW(mock.deliver({0x01, 0x00, 0xAA, 0xBB}));
// }
//
// TEST_CASE("Dispatcher receive with data too small does not crash") {
//         MockTransporter mock;
//         Dispatcher<MockTransporter> dispatcher(mock);
//
//         REQUIRE_NOTHROW(mock.deliver({0x01}));
// }
//
// TEST_CASE("Dispatcher receive with empty data does not crash") {
//         MockTransporter mock;
//         Dispatcher<MockTransporter> dispatcher(mock);
//
//         REQUIRE_NOTHROW(mock.deliver({}));
// }
//
// TEST_CASE("Dispatcher register_handler replaces existing handler") {
//         MockTransporter mock;
//         Dispatcher<MockTransporter> dispatcher(mock);
//
//         int call_count = 0;
//         dispatcher.register_handler(
//             0x0001, [&](result::Result<Data>) { call_count++; });
//         dispatcher.register_handler(
//             0x0001, [&](result::Result<Data>) { call_count += 10; });
//
//         mock.deliver({0x01, 0x00, 0xAA});
//
//         REQUIRE(call_count == 10);
// }
//
// TEST_CASE("Dispatcher dispatches to correct handler by command id") {
//         MockTransporter mock;
//         Dispatcher<MockTransporter> dispatcher(mock);
//
//         std::optional<Data> received_a;
//         std::optional<Data> received_b;
//         dispatcher.register_handler(0x0001, [&](result::Result<Data> result)
//         {
//                 if (!result.failed())
//                         received_a = std::move(result).value();
//         });
//         dispatcher.register_handler(0x0002, [&](result::Result<Data> result)
//         {
//                 if (!result.failed())
//                         received_b = std::move(result).value();
//         });
//
//         mock.deliver({0x02, 0x00, 0xFF});
//
//         REQUIRE(!received_a.has_value());
//         REQUIRE(received_b.has_value());
//         REQUIRE(received_b.value() == Data{0xFF});
// }
//
// TEST_CASE("WrappedData wrap and unwrap round trips") {
//         Data original = {0x01, 0x02, 0x03};
//         const auto wrap_result = WrappedData::wrap_data(0x0042,
//         Data(original)); REQUIRE(!wrap_result.failed());
//
//         const auto unwrap_result =
//             WrappedData::unwrap_data(Data(wrap_result.value()));
//         REQUIRE(!unwrap_result.failed());
//         REQUIRE(unwrap_result.value().command_id == 0x0042);
//         REQUIRE(unwrap_result.value().data == original);
// }
//
// TEST_CASE("WrappedData unwrap returns error on data too small") {
//         const auto result = WrappedData::unwrap_data({0x01});
//         REQUIRE(result.failed());
// }
//
// TEST_CASE("WrappedData unwrap returns error on empty data") {
//         const auto result = WrappedData::unwrap_data({});
//         REQUIRE(result.failed());
// }
