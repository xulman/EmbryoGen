#pragma once

#include "../__details.hpp"
#include <memory>

namespace tools {
namespace structures {

template <typename T, std::size_t N>
class StaticVector {
	T _elems[N];
	std::size_t _size = 0;

	void destroy_elems() noexcept {
		for (T& elem : *this)
			elem.~T();
	}

  public:
	using value_type = T;
	constexpr static std::size_t static_size = N;

	StaticVector() = default;
	StaticVector(const StaticVector&) = default;
	StaticVector(StaticVector&&) = default;
	StaticVector<T, N>& operator=(const StaticVector&) = default;
	StaticVector<T, N>& operator=(StaticVector&&) = default;

	void clear() noexcept {
		destroy_elems();
		_size = 0;
	}
	void push_back(T elem) {
		details::boundary_check(_size, N);
		std::uninitialized_move_n(&elem, 1, _elems + _size);
		++_size;
	}

	template <typename... Args>
	T& emplace_back(Args&&... args) {
		push_back(T(std::forward<Args>(args)...));
		return (*this[size() - 1]);
	}

	std::size_t size() const { return _size; }

	T& operator[](std::size_t i) {
		details::boundary_check(i, _size);
		return _elems[i];
	}

	const T& operator[](std::size_t i) const {
		details::boundary_check(i, _size);
		return _elems[i];
	}

	auto begin() noexcept { return details::ptr_iterator(_elems); }
	auto end() noexcept { return details::ptr_iterator(_elems + _size); }
	auto cbegin() const noexcept { return details::const_ptr_iterator(_elems); }
	auto cend() const noexcept {
		return details::const_ptr_iterator(_elems + _size);
	}
	auto begin() const noexcept { return cbegin(); }
	auto end() const noexcept { return cend(); }

	~StaticVector() noexcept { destroy_elems(); }
};

template <typename T>
using StaticVector3 = StaticVector<T, 3>;

template <typename T>
using StaticVector4 = StaticVector<T, 4>;

template <typename T>
using StaticVector5 = StaticVector<T, 5>;

template <typename T>
using StaticVector10 = StaticVector<T, 10>;

} // namespace structures
} // namespace tools
