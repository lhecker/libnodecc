#include "libnodecc/buffer.h"

#include "libnodecc/util/math.h"


namespace node {

string::string(std::size_t size) : buffer() {
	this->_reset_unsafe(size);
}

string::string(const node::literal_string& other) : buffer(other) {
}

string::string(const node::buffer_view& other) : buffer() {
	const auto size = other.size();

	if (size) {
		this->_reset_unsafe(size);

		if (this->_size) {
			memcpy(this->data(), other.data(), size);

			this->_size = size;
			this->data<char>()[size] = '\0';
		}
	}
}

void string::reset(std::size_t size) {
	this->_reset_unsafe(size);
}

string string::copy(std::size_t size) const {
	string str;
	this->_copy(str, size);
	return str;
}

std::unique_ptr<const char, string::c_str_deleter> string::create_c_str() const noexcept {
	return std::unique_ptr<const char, c_str_deleter>(this->c_str());
}

const char* string::c_str() const noexcept {
	return this->data<const char>();
}

void string::_copy(string& target, std::size_t size) const {
	size = size == 0 ? this->size() + 1 : size + 1;

	buffer::_copy(target, size == 0 ? this->size() + 1 : size + 1);

	if (target) {
		target._size = size;
		target.data<char>()[size] = '\0';
	}
}

void string::_reset_unsafe(std::size_t size) {
	if (size > 0) {
		buffer::_reset_unsafe(size + 1);

		std::to_string(0);

		if (this->_size) {
			this->_size = size;
			this->data<char>()[size] = '\0';
		}
	}
}


template<typename Value, bool = std::is_unsigned<Value>::value, bool = std::is_signed<Value>::value, bool = std::is_floating_point<Value>::value>
struct to_string_size;

template<typename Value>
struct to_string_size<Value, true, false, false> {
	static constexpr size_t max = std::numeric_limits<Value>::digits10;
};

template<typename Value>
struct to_string_size<Value, false, true, false> {
	static constexpr size_t max = std::numeric_limits<Value>::digits10 + 1;
};

template<typename Value>
struct to_string_size<Value, false, true, true> {
	static constexpr size_t max = std::numeric_limits<Value>::max_exponent10 + 6 + 2;
};


template<typename Value, std::size_t Size = to_string_size<Value>::max>
inline string to_string_impl(const char format[], Value v) {
	string str(Size);

	if (str) {
		const int r = snprintf(str.template data<char>(), str.size() + 1, format, v);

		if (r < 0) {
			str.reset();
		} else if (std::size_t(r) > str.size()) {
			/*
			 * Did we mess up the sizes in to_string_size()?
			 * Please file a bug report about this if this happens.
			 */
			assert(false);
		} else {
			str._size = r;
		}
	}

	return str;
}


string to_string(int                    val) { return to_string_impl("%i",   val); }
string to_string(long int               val) { return to_string_impl("%li",  val); }
string to_string(long long int          val) { return to_string_impl("%lli", val); }

string to_string(unsigned int           val) { return to_string_impl("%u",   val); }
string to_string(unsigned long int      val) { return to_string_impl("%lu",  val); }
string to_string(unsigned long long int val) { return to_string_impl("%llu", val); }

string to_string(float                  val) { return to_string_impl("%f",   val); }
string to_string(double                 val) { return to_string_impl("%f",   val); }
string to_string(long double            val) { return to_string_impl("%Lf",  val); }

} // namespace node
