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
