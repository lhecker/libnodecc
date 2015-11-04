#include <catch.hpp>
#include <cstdint>

#include "libnodecc/buffer.h"


#if UINTPTR_MAX == UINT64_MAX
# define ARCH_64
#else
# define ARCH_32
#endif

#define TEST_STRING "0123456789"

#ifdef ARCH_32
# define TEST_STRING_HASH 4185952242UL
#else
# define TEST_STRING_HASH 5818838724502565042ULL
#endif


constexpr size_t str_length(const char* str) {
	return *str ? 1 + str_length(str + 1) : 0;
}

static constexpr auto str_beg = TEST_STRING;
static constexpr auto str_size = str_length(str_beg);
static constexpr auto str_end = str_beg + str_size;

static_assert(str_size == 10, "buffer tests rely on a length of exactly 10");


TEST_CASE("buffer_view", "[buffer]") {
	REQUIRE(sizeof(node::buffer_view) == 2 * sizeof(void*));

	SECTION("buffer_view()") {
		const auto test = [](const node::buffer_view& view) {
			REQUIRE_FALSE(view);
			REQUIRE(view.empty());
			REQUIRE(view.size() == 0);
			REQUIRE_FALSE(view.data());
			REQUIRE_FALSE(view.begin());
		};

		const node::buffer_view view;
		test(view);

		SECTION("view(1, 7)") {
			test(view.view(1, 7));
		}

		SECTION("slice(1, 7)") {
			test(view.slice(1, 7));
		}
	}

	SECTION("buffer_view(\"" TEST_STRING "\")") {
		const node::buffer_view view(str_beg);

		REQUIRE(view);
		REQUIRE_FALSE(view.empty());
		REQUIRE(view.size() == str_size);
		REQUIRE(view.data<const char>() == str_beg);
		REQUIRE(view.begin<const char>() == str_beg);
		REQUIRE(view.end<const char>() == str_end);

		const auto test_slice_7 = [](const node::buffer_view& view) {
			REQUIRE(view);
			REQUIRE_FALSE(view.empty());
			REQUIRE(view.size() == 3);
			REQUIRE(view.data<const char>() == str_beg + 7);
			REQUIRE(view.begin<const char>() == str_beg + 7);
			REQUIRE(view.end<const char>() == str_end);
		};

		const auto test_slice_15 = [](const node::buffer_view& view) {
			REQUIRE_FALSE(view);
			REQUIRE(view.empty());
			REQUIRE(view.size() == 0);
			REQUIRE_FALSE(view.data());
			REQUIRE_FALSE(view.begin());
		};

		const auto test_slice_3_9 = [](const node::buffer_view& view) {
			REQUIRE(view);
			REQUIRE_FALSE(view.empty());
			REQUIRE(view.size() == 6);
			REQUIRE(view.data<const char>() == str_beg + 3);
			REQUIRE(view.begin<const char>() == str_beg + 3);
			REQUIRE(view.end<const char>() == str_beg + 9);
		};

		SECTION("view(7)") {
			test_slice_7(view.view(7));
		}

		SECTION("slice(7)") {
			test_slice_7(view.slice(7));
		}

		SECTION("view(7, 15)") {
			test_slice_7(view.view(7, 15));
		}

		SECTION("slice(7, 15)") {
			test_slice_7(view.slice(7, 15));
		}

		SECTION("view(15)") {
			test_slice_15(view.view(15));
		}

		SECTION("slice(15)") {
			test_slice_15(view.slice(15));
		}

		SECTION("view(3, 9)") {
			test_slice_3_9(view.view(3, 9));
		}

		SECTION("slice(3, 9)") {
			test_slice_3_9(view.slice(3, 9));
		}

		SECTION("index_of('a')") {
			const auto idx = view.index_of('a');
			REQUIRE(idx == node::buffer_view::npos);
		}

		SECTION("index_of('5')") {
			const auto idx = view.index_of('5');
			REQUIRE(idx == 5);
		}

		SECTION("index_of(\"foo\")") {
			const auto idx = view.index_of("foo");
			REQUIRE(idx == node::buffer_view::npos);
		}

		SECTION("index_of(\"123\")") {
			const auto idx = view.index_of("123");
			REQUIRE(idx == 1);
		}
	}
}

TEST_CASE("buffer", "[buffer]") {
	REQUIRE(sizeof(node::buffer) == 3 * sizeof(void*));

	SECTION("buffer()") {
		REQUIRE_THROWS_AS(node::buffer(size_t(-1)), std::bad_alloc);
	}

	SECTION("buffer(\"" TEST_STRING "\", weak)") {
		node::buffer buf;

		REQUIRE_NOTHROW(buf = node::buffer(str_beg, node::buffer_flags::weak));
		REQUIRE(buf);
		REQUIRE(buf.size() == str_size);
		REQUIRE(buf.data() == (uint8_t*)str_beg);
		REQUIRE(buf.is_weak());
		REQUIRE(buf.use_count() == 0);
	}

	SECTION("buffer(\"" TEST_STRING "\")") {
		node::buffer buf;

		REQUIRE_NOTHROW(buf = node::buffer(str_beg));
		REQUIRE(buf);
		REQUIRE(buf.size() == str_size);
		REQUIRE(buf.data());
		REQUIRE(buf.data() != (uint8_t*)str_beg);
		REQUIRE(buf.is_strong());
		REQUIRE(buf.use_count() == 1);
	}

	SECTION("buffer(1024)") {
		node::buffer buf;

		REQUIRE_NOTHROW(buf = node::buffer(1024));
		REQUIRE(buf);
		REQUIRE(buf.size() == 1024);
		REQUIRE(buf.data());
		REQUIRE(buf.is_strong());
		REQUIRE(buf.use_count() == 1);

		const auto test_slice_25 = [&buf](const node::buffer& b) {
			REQUIRE(b);
			REQUIRE(b.size() == 999);
			REQUIRE(b.begin() == &buf[25]);
			REQUIRE(b.end() == buf.end());

			REQUIRE(b.use_count() == 2);
			REQUIRE(buf.use_count() == 2);
		};

		SECTION("slice(25)") {
			test_slice_25(buf.slice(25));
		}

		SECTION("slice(25, 1025)") {
			test_slice_25(buf.slice(25, 1025));
		}

		SECTION("slice(1025)") {
			const auto b = buf.slice(1025);

			REQUIRE_FALSE(b);
			REQUIRE(b.size() == 0);
			REQUIRE_FALSE(b.begin());
			REQUIRE_FALSE(b.end());

			REQUIRE(b.use_count() == 0);
			REQUIRE(buf.use_count() == 1);
		}

		SECTION("slice(3, 9)") {
			const auto b = buf.slice(3, 9);

			REQUIRE(b);
			REQUIRE(b.size() == 6);
			REQUIRE(b.begin() == &buf[3]);
			REQUIRE(b.end() == &buf[9]);

			REQUIRE(b.use_count() == 2);
			REQUIRE(buf.use_count() == 2);
		}

		buf.reset();

		REQUIRE_FALSE(buf);
		REQUIRE(buf.size() == 0);
		REQUIRE_FALSE(buf.data());
		REQUIRE_FALSE(buf.is_strong());
		REQUIRE(buf.use_count() == 0);
	}

	SECTION("buffer(1024, deleter)") {
		const auto ptr = malloc(1024);
		REQUIRE(ptr);

		node::buffer buf;
		bool called = false;

		REQUIRE_NOTHROW(buf = node::buffer(ptr, 1024, [&called](void* base) { called = true; free(base); }));
		REQUIRE_FALSE(called);
		REQUIRE(buf);
		REQUIRE(buf.size() == 1024);
		REQUIRE(buf.data());
		REQUIRE(buf.is_strong());
		REQUIRE(buf.use_count() == 1);

		buf.reset();

		REQUIRE(called);
		REQUIRE_FALSE(buf);
		REQUIRE(buf.size() == 0);
		REQUIRE_FALSE(buf.data());
		REQUIRE_FALSE(buf.is_strong());
		REQUIRE(buf.use_count() == 0);
	}
}

TEST_CASE("mutable_buffer", "[buffer]") {
	REQUIRE(sizeof(node::mutable_buffer) == 4 * sizeof(void*));

	node::mutable_buffer buf;

	REQUIRE_FALSE(buf);
	REQUIRE(buf.size() == 0);
	REQUIRE(buf.capacity() == 0);

	buf.push_back('x');
	REQUIRE(buf.size() == 1);
	REQUIRE(buf.capacity() == 16);

	buf.append(str_beg);
	REQUIRE(buf.size() == 11);
	REQUIRE(buf.capacity() == 16);

	buf.append_number(1234, 10);
	REQUIRE(buf.size() == 15);
	REQUIRE(buf.capacity() == 16);

	buf.append_number(5, 16);
	REQUIRE(buf.size() == 16);
	REQUIRE(buf.capacity() == 16);

	buf.append_number(16, 16);
	REQUIRE(buf.size() == 18);
	REQUIRE(buf.capacity() == 24);

	buf.append(node::buffer(100));
	REQUIRE(buf.size() == 118);
	REQUIRE(buf.capacity() == 118);


	const auto b = buf.slice();
	REQUIRE(b);
	REQUIRE(b.size() == 118);
	REQUIRE(b.capacity() == 118);
	REQUIRE(b.use_count() == 2);


	buf.set_size(24);
	REQUIRE(buf.size() == 24);
	REQUIRE(buf.capacity() == 24);

	buf.set_size(17);
	REQUIRE(buf.size() == 17);
	REQUIRE(buf.capacity() == 24);

	buf.set_size(16);
	REQUIRE(buf.size() == 16);
	REQUIRE(buf.capacity() == 16);

	buf.set_capacity(4);
	REQUIRE(buf.size() == 4);
	REQUIRE(buf.capacity() == 16);

	buf.clear();
	REQUIRE(buf);
	REQUIRE(buf.size() == 0);
	REQUIRE(buf.capacity() == 16);

	buf.reset();
	REQUIRE_FALSE(buf);
	REQUIRE(buf.size() == 0);
	REQUIRE(buf.capacity() == 0);


	REQUIRE(b);
	REQUIRE(b.size() == 118);
	REQUIRE(b.capacity() == 118);
	REQUIRE(b.use_count() == 1);
}

template<typename T>
static size_t extract_hash(const T& buf) {
	static_assert(sizeof(size_t) == sizeof(void*), "size_t is currently assumed to be of pointer size");
	return reinterpret_cast<const size_t*>(&buf)[(sizeof(T) / sizeof(size_t)) - 1];
}

TEST_CASE("hashed_buffer_view", "[buffer]") {
	REQUIRE(sizeof(node::hashed_buffer_view) == 3 * sizeof(void*));

	const node::hashed_buffer_view view(str_beg);

	REQUIRE(extract_hash(view) == 0);
	REQUIRE(view.hash() == TEST_STRING_HASH);
	REQUIRE(extract_hash(view) == TEST_STRING_HASH);
}

TEST_CASE("hashed_buffer", "[buffer]") {
	REQUIRE(sizeof(node::hashed_buffer) == 4 * sizeof(void*));
}

TEST_CASE("string", "[buffer]") {
	REQUIRE(sizeof(node::string) == 3 * sizeof(void*));

	const node::string str(str_beg);
	REQUIRE(str);
	REQUIRE(str.size() == str_size);
	REQUIRE(str.data());
	REQUIRE(str.data() != (uint8_t*)str_beg);
	REQUIRE(str[str_size] == '\0');
	REQUIRE(str.is_strong());
	REQUIRE(str.use_count() == 1);
}

TEST_CASE("hashed_string", "[buffer]") {
	REQUIRE(sizeof(node::hashed_string) == 4 * sizeof(void*));

	const node::hashed_string view;
}

TEST_CASE("literal_string", "[buffer]") {
	REQUIRE(sizeof(node::literal_string) == 3 * sizeof(void*));

#define make_str_impl(str) str##_view
#define make_str(str) make_str_impl(str)

	using namespace node::literals;
	const auto str = make_str(TEST_STRING);

#undef make_view
#undef make_view_impl

	REQUIRE(extract_hash(str) == TEST_STRING_HASH);
	REQUIRE(str.hash() == TEST_STRING_HASH);
	REQUIRE(extract_hash(str) == TEST_STRING_HASH);

	REQUIRE(str);
	REQUIRE(str.size() == str_size);
	REQUIRE(str.data() == (uint8_t*)str_beg);
}
