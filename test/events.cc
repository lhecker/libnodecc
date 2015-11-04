#include <catch.hpp>

#include "libnodecc/events.h"


TEST_CASE("events::symbol", "[events]") {
	node::events::symbol<void()> s;

	REQUIRE(sizeof(s) == 1);
}

TEST_CASE("events::emitter", "[events]") {
#define MAKE_SYMBOL(_sym_)        node::events::symbol<void(size_t i)> s##_sym_;
#define MAKE_ON(_sym_, _mul_)     ee.on(s##_sym_, [&invocations](size_t i) { invocations += _mul_ * i; });
#define MAKE_EMIT(_sym_, _mul_)   ee.emit(s##_sym_, i);
#define MAKE_CODE(_sym_map_, _emit_map_)                              \
	_sym_map_(MAKE_SYMBOL)                                            \
	node::events::emitter ee;                                         \
	size_t invocations = 0;                                    \
	_emit_map_(MAKE_ON)                                               \
	for (size_t i = 0; i < 123; i++) { _emit_map_(MAKE_EMIT) } \

	SECTION("single -> single") {
#define SYM_MAP(XX) \
	XX(1) \

#define EMIT_MAP(XX) \
	XX(1, 1) \

		MAKE_CODE(SYM_MAP, EMIT_MAP)

		REQUIRE(invocations == 7503);

#undef SYM_MAP
#undef EMIT_MAP
	}

	SECTION("single -> many") {
#define SYM_MAP(XX) \
	XX(1) \

#define EMIT_MAP(XX) \
	XX(1, 1) \
	XX(1, 2) \
	XX(1, 3) \
	XX(1, 4) \
	XX(1, 5) \

		MAKE_CODE(SYM_MAP, EMIT_MAP)

		REQUIRE(invocations == 562725);

#undef SYM_MAP
#undef EMIT_MAP
	}

	SECTION("many -> single") {
#define SYM_MAP(XX) \
	XX(1) \
	XX(2) \
	XX(3) \
	XX(4) \
	XX(5) \

#define EMIT_MAP(XX) \
	XX(1, 1) \
	XX(2, 2) \
	XX(3, 3) \
	XX(4, 4) \
	XX(5, 5) \

		MAKE_CODE(SYM_MAP, EMIT_MAP)

		REQUIRE(invocations == 112545);

#undef SYM_MAP
#undef EMIT_MAP
	}

	SECTION("many -> many") {
#define SYM_MAP(XX) \
	XX(1) \
	XX(2) \
	XX(3) \
	XX(4) \
	XX(5) \

#define EMIT_MAP(XX) \
	XX(1, 1) \
	XX(2, 2) \
	XX(2, 2) \
	XX(3, 3) \
	XX(3, 3) \
	XX(3, 3) \
	XX(4, 4) \
	XX(4, 4) \
	XX(4, 4) \
	XX(4, 4) \
	XX(5, 5) \
	XX(5, 5) \
	XX(5, 5) \
	XX(5, 5) \
	XX(5, 5) \

		MAKE_CODE(SYM_MAP, EMIT_MAP)

		REQUIRE(invocations == 1688175);

#undef SYM_MAP
#undef EMIT_MAP
	}

#undef MAKE_SYMBOL
#undef MAKE_ON
#undef MAKE_EMIT
#undef MAKE_CODE

	SECTION("recursive") {
		node::events::symbol<void(size_t)> s1;
		node::events::symbol<void(size_t)> s2;
		node::events::emitter ee;
		size_t sum = 0;

		ee.on(s1, [&](size_t i) {
			if (i == 7) {
				sum = i;
			} else {
				ee.emit(s2, i + 1);
			}
		});

		ee.on(s2, [&](size_t i) {
			if (i == 7) {
				sum = i;
			} else {
				ee.emit(s1, i + 1);
			}
		});

		ee.emit(s1, 1);
		REQUIRE(sum == 7);
	}

	SECTION("deletion") {
		node::events::symbol<void()> s1;
		node::events::symbol<void()> s2;
		node::events::emitter ee;
		size_t invocations = 0;

		const auto it = ee.on(s1, [&]() { invocations++; });
		ee.on(s1, [&]() { invocations++; });
		ee.on(s2, [&]() { invocations++; });
		ee.on(s2, [&]() { invocations++; });

		ee.emit(s1);
		ee.emit(s2);
		REQUIRE(invocations == 4);

		ee.removeAllListeners(s2);

		ee.emit(s1);
		ee.emit(s2);
		REQUIRE(invocations == 6);

		ee.off(s1, it);

		ee.emit(s1);
		ee.emit(s2);
		REQUIRE(invocations == 7);

		ee.removeAllListeners();

		ee.emit(s1);
		ee.emit(s2);
		REQUIRE(invocations == 7);
	}

	SECTION("recursive deletion") {
		node::events::symbol<void()> s1;
		node::events::symbol<void()> s2;
		node::events::symbol<void()> s3;
		node::events::emitter ee;
		size_t depth = 0;
		size_t s2_extra = 0;

		ee.on(s1, [&]() {
			ee.emit(++depth > 3 ? s2 : s1);
		});

		ee.on(s2, [&]() {
			ee.emit(++depth > 5 ? s3 : s2);
		});

		const auto it = ee.on(s2, [&]() {
			REQUIRE(false);
		});

		ee.on(s2, [&]() {
			s2_extra++;
		});

		ee.on(s3, [&]() {
			if (++depth == 7) {
				ee.removeAllListeners(s1);
				ee.off(s2, it);
				ee.removeAllListeners(s3);
			}

			ee.emit(s3);
		});

		ee.emit(s1);
		REQUIRE(depth == 7);
		REQUIRE(s2_extra == 2);

		ee.emit(s1);
		ee.emit(s2);
		ee.emit(s3);
		REQUIRE(depth == 8);
		REQUIRE(s2_extra == 3);
	}
}
