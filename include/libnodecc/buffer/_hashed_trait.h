#ifndef nodecc_buffer__hashed_trait_h
#define nodecc_buffer__hashed_trait_h


namespace node {

	class buffer_view;

namespace detail {

// inject the hash trait using CRTP
template<typename T>
class hashed_trait {
public:
	typedef hashed_trait<T> hashed_type;


	constexpr hashed_trait(std::size_t hash) : _hash(hash) {}

	constexpr std::size_t const_hash() const noexcept {
		return this->_hash;
	}

	std::size_t hash() const noexcept {
		std::size_t hash = this->_hash;

		if (hash == 0) {
			hash = node::util::fnv1a<std::size_t>::hash(*static_cast<const T*>(this));

			if (hash == 0) {
				hash = 1;
			}

			this->_hash = hash;
		}

		return hash;
	}

	template<typename S>
	bool equals(const hashed_trait<S>& other) const noexcept {
		const auto lhs = static_cast<const T*>(this);
		const auto rhs = static_cast<const S*>(&other);

		return lhs->size() == rhs->size()
		    && (
		           lhs->data() == rhs->data()
		        || (
		               lhs->data() != nullptr
		            && rhs->data() != nullptr
		            && lhs->hash() == rhs->hash()
		            && memcmp(lhs->data(), rhs->data(), lhs->size()) == 0
		        )
		    );
	}

protected:
	mutable std::size_t _hash;
};

} // namespace detail
} // namespace node

#endif // nodecc_buffer__hashed_trait_h
