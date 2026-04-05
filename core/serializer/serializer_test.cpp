#include <catch2/catch_test_macros.hpp>

#include "serializer.hpp"

struct Point {
        uint32_t x;
        uint32_t y;
        serializer::Data serialize() const {
                serializer::Writer w;
                w.write(x);
                w.write(y);
                return std::move(w.buf);
        }
        static result::Result<Point> deserialize(serializer::DataView buf) {
                serializer::Reader r{buf};
                const auto x = r.read<uint32_t>();
                if (x.failed())
                        return result::err(x.error());
                const auto y = r.read<uint32_t>();
                if (y.failed())
                        return result::err(y.error());
                return result::ok(Point{
                    .x = std::move(x).value(),
                    .y = std::move(y).value(),
                });
        }
        bool operator==(const Point &) const = default;
};
static_assert(serializer::Serializable<Point>);

TEST_CASE("serialize produces correct byte count") {
        const Point p{.x = 1, .y = 2};
        const auto buf = p.serialize();
        REQUIRE(buf.size() == sizeof(uint32_t) * 2);
}

TEST_CASE("deserialize roundtrips correctly") {
        const Point p{.x = 42, .y = 99};
        const auto buf = p.serialize();
        const auto result = Point::deserialize(buf);
        REQUIRE_FALSE(result.failed());
        REQUIRE(result.value().x == p.x);
        REQUIRE(result.value().y == p.y);
}

TEST_CASE("deserialize fails on empty buffer") {
        const auto result = Point::deserialize({});
        REQUIRE(result.failed());
}

TEST_CASE("deserialize fails on truncated buffer") {
        const Point p{.x = 1, .y = 2};
        auto buf = p.serialize();
        buf.resize(3);
        const auto result = Point::deserialize(buf);
        REQUIRE(result.failed());
}

TEST_CASE("writer accumulates multiple writes") {
        serializer::Writer w;
        w.write<uint8_t>(0x01);
        w.write<uint16_t>(0x0203);
        REQUIRE(w.buf.size() == 3);
}

TEST_CASE("reader fails on out of bounds read") {
        const serializer::Data buf = {0x01};
        serializer::Reader r{buf};
        const auto result = r.read<uint32_t>();
        REQUIRE(result.failed());
}

// --- trivial types ---

TEST_CASE("uint8_t roundtrips") {
        serializer::Writer w;
        w.write<uint8_t>(0xFF);
        serializer::Reader r{w.buf};
        const auto result = r.read<uint8_t>();
        REQUIRE_FALSE(result.failed());
        REQUIRE(result.value() == 0xFF);
}

TEST_CASE("uint16_t roundtrips") {
        serializer::Writer w;
        w.write<uint16_t>(0xBEEF);
        serializer::Reader r{w.buf};
        const auto result = r.read<uint16_t>();
        REQUIRE_FALSE(result.failed());
        REQUIRE(result.value() == 0xBEEF);
}

TEST_CASE("uint64_t roundtrips") {
        serializer::Writer w;
        w.write<uint64_t>(0xDEADBEEFCAFEBABE);
        serializer::Reader r{w.buf};
        const auto result = r.read<uint64_t>();
        REQUIRE_FALSE(result.failed());
        REQUIRE(result.value() == 0xDEADBEEFCAFEBABE);
}

TEST_CASE("float roundtrips") {
        serializer::Writer w;
        w.write(3.14f);
        serializer::Reader r{w.buf};
        const auto result = r.read<float>();
        REQUIRE_FALSE(result.failed());
        REQUIRE(result.value() == 3.14f);
}

TEST_CASE("bool roundtrips") {
        serializer::Writer w;
        w.write(true);
        w.write(false);
        serializer::Reader r{w.buf};
        REQUIRE(r.read<bool>().value() == true);
        REQUIRE(r.read<bool>().value() == false);
}

TEST_CASE("reader advances position across sequential reads") {
        serializer::Writer w;
        w.write<uint8_t>(0x01);
        w.write<uint8_t>(0x02);
        w.write<uint8_t>(0x03);
        serializer::Reader r{w.buf};
        REQUIRE(r.read<uint8_t>().value() == 0x01);
        REQUIRE(r.read<uint8_t>().value() == 0x02);
        REQUIRE(r.read<uint8_t>().value() == 0x03);
}

TEST_CASE("reader fails on second read when buffer exhausted") {
        serializer::Writer w;
        w.write<uint8_t>(0x01);
        serializer::Reader r{w.buf};
        REQUIRE_FALSE(r.read<uint8_t>().failed());
        REQUIRE(r.read<uint8_t>().failed());
}

// --- std::string ---

TEST_CASE("string roundtrips") {
        serializer::Writer w;
        w.write(std::string("hello"));
        serializer::Reader r{w.buf};
        const auto result = r.read_string();
        REQUIRE_FALSE(result.failed());
        REQUIRE(result.value() == "hello");
}

TEST_CASE("empty string roundtrips") {
        serializer::Writer w;
        w.write(std::string(""));
        serializer::Reader r{w.buf};
        const auto result = r.read_string();
        REQUIRE_FALSE(result.failed());
        REQUIRE(result.value().empty());
}

TEST_CASE("string with embedded nulls roundtrips") {
        const std::string s = {'\0', 'a', '\0', 'b'};
        serializer::Writer w;
        w.write(s);
        serializer::Reader r{w.buf};
        const auto result = r.read_string();
        REQUIRE_FALSE(result.failed());
        REQUIRE(result.value() == s);
}

TEST_CASE("string size prefix is 4 bytes plus content") {
        serializer::Writer w;
        w.write(std::string("abc"));
        REQUIRE(w.buf.size() == sizeof(uint32_t) + 3);
}

TEST_CASE("string read fails on truncated length prefix") {
        const serializer::Data buf = {0x00, 0x00};
        serializer::Reader r{buf};
        REQUIRE(r.read_string().failed());
}

TEST_CASE("string read fails when content shorter than declared length") {
        serializer::Writer w;
        w.write(std::string("hello world"));
        auto buf = w.buf;
        buf.resize(sizeof(uint32_t) + 2);
        serializer::Reader r{buf};
        REQUIRE(r.read_string().failed());
}

TEST_CASE("multiple strings roundtrip sequentially") {
        serializer::Writer w;
        w.write(std::string("foo"));
        w.write(std::string("bar"));
        serializer::Reader r{w.buf};
        REQUIRE(r.read_string().value() == "foo");
        REQUIRE(r.read_string().value() == "bar");
}

// --- std::vector ---

TEST_CASE("vector<uint32_t> roundtrips") {
        serializer::Writer w;
        w.write(std::vector<uint32_t>{1, 2, 3});
        serializer::Reader r{w.buf};
        const auto result = r.read_vec<uint32_t>();
        REQUIRE_FALSE(result.failed());
        REQUIRE(result.value() == std::vector<uint32_t>{1, 2, 3});
}

TEST_CASE("empty vector roundtrips") {
        serializer::Writer w;
        w.write(std::vector<uint32_t>{});
        serializer::Reader r{w.buf};
        const auto result = r.read_vec<uint32_t>();
        REQUIRE_FALSE(result.failed());
        REQUIRE(result.value().empty());
}

TEST_CASE("vector<Unit> fast path roundtrips") {
        serializer::Writer w;
        w.write(std::vector<serializer::Unit>{0xAA, 0xBB, 0xCC});
        serializer::Reader r{w.buf};
        const auto result = r.read_vec<serializer::Unit>();
        REQUIRE_FALSE(result.failed());
        REQUIRE(result.value() ==
                std::vector<serializer::Unit>{0xAA, 0xBB, 0xCC});
}

TEST_CASE("vector size prefix is 4 bytes plus element data") {
        serializer::Writer w;
        w.write(std::vector<uint16_t>{1, 2, 3});
        REQUIRE(w.buf.size() == sizeof(uint32_t) + 3 * sizeof(uint16_t));
}

TEST_CASE("vector read fails on truncated content") {
        serializer::Writer w;
        w.write(std::vector<uint32_t>{1, 2, 3});
        auto buf = w.buf;
        buf.resize(sizeof(uint32_t) + sizeof(uint32_t));
        serializer::Reader r{buf};
        REQUIRE(r.read_vec<uint32_t>().failed());
}

// --- std::array ---

TEST_CASE("array<uint8_t, 4> roundtrips") {
        serializer::Writer w;
        w.write(std::array<uint8_t, 4>{0x01, 0x02, 0x03, 0x04});
        serializer::Reader r{w.buf};
        const auto result = r.read_array<uint8_t, 4>();
        REQUIRE_FALSE(result.failed());
        REQUIRE(result.value() ==
                std::array<uint8_t, 4>{0x01, 0x02, 0x03, 0x04});
}

TEST_CASE("array produces no length prefix") {
        serializer::Writer w;
        w.write(std::array<uint32_t, 3>{1, 2, 3});
        REQUIRE(w.buf.size() == 3 * sizeof(uint32_t));
}

TEST_CASE("array read fails on truncated buffer") {
        serializer::Writer w;
        w.write(std::array<uint32_t, 3>{1, 2, 3});
        auto buf = w.buf;
        buf.resize(sizeof(uint32_t));
        serializer::Reader r{buf};
        REQUIRE(r.read_array<uint32_t, 3>().failed());
}

// --- std::pair ---

TEST_CASE("pair<uint8_t, uint32_t> roundtrips") {
        serializer::Writer w;
        w.write(std::pair<uint8_t, uint32_t>{0x01, 0xDEADBEEF});
        serializer::Reader r{w.buf};
        const auto result = r.read_pair<uint8_t, uint32_t>();
        REQUIRE_FALSE(result.failed());
        REQUIRE(result.value() ==
                std::pair<uint8_t, uint32_t>{0x01, 0xDEADBEEF});
}

TEST_CASE("pair read fails when second element truncated") {
        serializer::Writer w;
        w.write(std::pair<uint8_t, uint32_t>{0x01, 0x02});
        auto buf = w.buf;
        buf.resize(sizeof(uint8_t));
        serializer::Reader r{buf};
        REQUIRE(r.read_pair<uint8_t, uint32_t>().failed());
}

// --- std::optional ---

TEST_CASE("optional with value roundtrips") {
        serializer::Writer w;
        w.write(std::optional<uint32_t>{42});
        serializer::Reader r{w.buf};
        const auto result = r.read_optional<uint32_t>();
        REQUIRE_FALSE(result.failed());
        REQUIRE(result.value().has_value());
        REQUIRE(result.value().value() == 42);
}

TEST_CASE("optional without value roundtrips") {
        serializer::Writer w;
        w.write(std::optional<uint32_t>{});
        serializer::Reader r{w.buf};
        const auto result = r.read_optional<uint32_t>();
        REQUIRE_FALSE(result.failed());
        REQUIRE_FALSE(result.value().has_value());
}

TEST_CASE("nullopt is 1 byte") {
        serializer::Writer w;
        w.write(std::optional<uint32_t>{});
        REQUIRE(w.buf.size() == sizeof(uint8_t));
}

TEST_CASE("optional with value is 1 byte flag plus payload") {
        serializer::Writer w;
        w.write(std::optional<uint32_t>{1});
        REQUIRE(w.buf.size() == sizeof(uint8_t) + sizeof(uint32_t));
}

TEST_CASE("optional read fails on truncated value") {
        serializer::Writer w;
        w.write(std::optional<uint32_t>{0xDEAD});
        auto buf = w.buf;
        buf.resize(sizeof(uint8_t));
        serializer::Reader r{buf};
        REQUIRE(r.read_optional<uint32_t>().failed());
}

// --- Serializable concept / composition ---

TEST_CASE("Serializable Point roundtrips via concept interface") {
        const Point p{.x = 7, .y = 13};
        const auto buf = p.serialize();
        const auto result = Point::deserialize(buf);
        REQUIRE_FALSE(result.failed());
        REQUIRE(result.value() == p);
}

TEST_CASE("vector of Serializable structs roundtrips manually") {
        const std::vector<Point> points = {{1, 2}, {3, 4}, {5, 6}};
        serializer::Writer w;
        w.write(static_cast<uint32_t>(points.size()));
        for (const auto &p : points) {
                const auto bytes = p.serialize();
                w.write(static_cast<uint32_t>(bytes.size()));
                w.buf.insert(w.buf.end(), bytes.begin(), bytes.end());
        }

        serializer::Reader r{w.buf};
        const auto count = r.read<uint32_t>();
        REQUIRE_FALSE(count.failed());
        std::vector<Point> out;
        for (uint32_t i = 0; i < count.value(); ++i) {
                const auto size = r.read<uint32_t>();
                REQUIRE_FALSE(size.failed());
                const serializer::DataView view{r.buf.data() + r.pos,
                                                size.value()};
                const auto p = Point::deserialize(view);
                REQUIRE_FALSE(p.failed());
                out.push_back(p.value());
                r.pos += size.value();
        }
        REQUIRE(out == points);
}

TEST_CASE("mixed types written and read back in order") {
        serializer::Writer w;
        w.write<uint8_t>(0xAB);
        w.write(std::string("test"));
        w.write<uint32_t>(9999);
        w.write(std::optional<uint16_t>{7});

        serializer::Reader r{w.buf};
        REQUIRE(r.read<uint8_t>().value() == 0xAB);
        REQUIRE(r.read_string().value() == "test");
        REQUIRE(r.read<uint32_t>().value() == 9999);
        const auto opt = r.read_optional<uint16_t>();
        REQUIRE_FALSE(opt.failed());
        REQUIRE(opt.value() == 7);
}
