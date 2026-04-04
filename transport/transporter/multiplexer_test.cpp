// #include <catch2/catch_test_macros.hpp>
// #include <optional>
// #include <vector>
//
// #include "multiplexer.hpp"
// using namespace transport;
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
// TEST_CASE("Multiplexer send_with_id prepends id to data") {
//         MockTransporter mock;
//         std::unordered_map<TransporterId,
//                            std::reference_wrapper<MockTransporter>>
//             transporters;
//         transporters.emplace(0x01, std::ref(mock));
//         Multiplexer<MockTransporter> mux(mock, std::move(transporters));
//
//         Data data = {0xAA, 0xBB};
//         const auto result = mux.send_with_id(0x01, std::move(data));
//
//         REQUIRE(!result.failed());
//         REQUIRE(mock.sent.size() == 1);
//         REQUIRE(mock.sent[0][0] == 0x01);
//         REQUIRE(mock.sent[0][1] == 0xAA);
//         REQUIRE(mock.sent[0][2] == 0xBB);
// }
//
// TEST_CASE("Multiplexer send_with_id returns error for unregistered id") {
//         MockTransporter mock;
//         std::unordered_map<TransporterId,
//                            std::reference_wrapper<MockTransporter>>
//             transporters;
//         Multiplexer<MockTransporter> mux(mock, std::move(transporters));
//
//         Data data = {0x01};
//         const auto result = mux.send_with_id(0x99, std::move(data));
//
//         REQUIRE(result.failed());
// }
//
// TEST_CASE("Multiplexer get_channel returns channel for registered id") {
//         MockTransporter mock;
//         std::unordered_map<TransporterId,
//                            std::reference_wrapper<MockTransporter>>
//             transporters;
//         transporters.emplace(0x01, std::ref(mock));
//         Multiplexer<MockTransporter> mux(mock, std::move(transporters));
//
//         const auto result = mux.get_channel(0x01);
//
//         REQUIRE(!result.failed());
// }
//
// TEST_CASE("Multiplexer get_channel returns error for unregistered id") {
//         MockTransporter mock;
//         std::unordered_map<TransporterId,
//                            std::reference_wrapper<MockTransporter>>
//             transporters;
//         Multiplexer<MockTransporter> mux(mock, std::move(transporters));
//
//         const auto result = mux.get_channel(0x42);
//
//         REQUIRE(result.failed());
// }
//
// TEST_CASE("Multiplexer Channel send prepends id") {
//         MockTransporter mock;
//         std::unordered_map<TransporterId,
//                            std::reference_wrapper<MockTransporter>>
//             transporters;
//         transporters.emplace(0x02, std::ref(mock));
//         Multiplexer<MockTransporter> mux(mock, std::move(transporters));
//
//         auto channel_result = mux.get_channel(0x02);
//         REQUIRE(!channel_result.failed());
//
//         auto channel = std::move(channel_result).value();
//         Data data = {0xDE, 0xAD};
//         const auto result = channel.send(std::move(data));
//
//         REQUIRE(!result.failed());
//         REQUIRE(mock.sent[0][0] == 0x02);
//         REQUIRE(mock.sent[0][1] == 0xDE);
//         REQUIRE(mock.sent[0][2] == 0xAD);
// }
//
// TEST_CASE("Multiplexer Channel get_mtu subtracts one from transporter mtu") {
//         MockTransporter mock;
//         mock.mtu = 64;
//         std::unordered_map<TransporterId,
//                            std::reference_wrapper<MockTransporter>>
//             transporters;
//         transporters.emplace(0x01, std::ref(mock));
//         Multiplexer<MockTransporter> mux(mock, std::move(transporters));
//
//         auto channel_result = mux.get_channel(0x01);
//         REQUIRE(!channel_result.failed());
//
//         const auto channel = std::move(channel_result).value();
//         REQUIRE(channel.get_mtu() == 63);
// }
//
// TEST_CASE("Multiplexer receive dispatches to correct transporter") {
//         MockTransporter mock;
//         MockTransporter sub_mock;
//         std::unordered_map<TransporterId,
//                            std::reference_wrapper<MockTransporter>>
//             transporters;
//         transporters.emplace(0x01, std::ref(sub_mock));
//         Multiplexer<MockTransporter> mux(mock, std::move(transporters));
//
//         mock.deliver({0x01, 0xCA, 0xFE});
//
//         REQUIRE(sub_mock.sent.size() == 1);
//         REQUIRE(sub_mock.sent[0] == Data{0xCA, 0xFE});
// }
//
// TEST_CASE("Multiplexer receive ignores unknown transporter id") {
//         MockTransporter mock;
//         std::unordered_map<TransporterId,
//                            std::reference_wrapper<MockTransporter>>
//             transporters;
//         Multiplexer<MockTransporter> mux(mock, std::move(transporters));
//
//         REQUIRE_NOTHROW(mock.deliver({0xFF, 0x01, 0x02}));
// }
//
// TEST_CASE("Multiplexer receive ignores data smaller than header") {
//         MockTransporter mock;
//         std::unordered_map<TransporterId,
//                            std::reference_wrapper<MockTransporter>>
//             transporters;
//         Multiplexer<MockTransporter> mux(mock, std::move(transporters));
//
//         REQUIRE_NOTHROW(mock.deliver({}));
// }
