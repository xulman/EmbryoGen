#pragma once

#include "../__details.hpp"
#include <memory>
#include <cstring>

namespace tools {
namespace structures {

template <typename T, std::size_t N>
class StaticVector {
	char __raw_data[N * sizeof(T)];
	T* _elems = reinterpret_cast<T*>(&__raw_data[0]);
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

	T& back() { return (*this)[size() - 1]; }
	const T& back() const { return (*this)[size() - 1]; }

	void pop_back() {
		details::boundary_check(0, _size, "StaticVector: pop_back() failed");
		
		back().~T();
		--_size;
	}

	void push_back(T elem) {
		details::boundary_check(_size, N, "StaticVector: push_back() failed");
		
		std::uninitialized_move_n(&elem, 1, _elems + _size);
		++_size;
	}

	void erase_at(std::size_t idx)
	{
		details::boundary_check(idx, _size, "StaticVector: erase_at() on invalid index");

		_elems[idx].~T();
		std::memmove(
			__raw_data + idx * sizeof(T), 		// dest
			__raw_data + (idx + 1) * sizeof(T), // src
			sizeof(T) // count in bytes
		);
		--_size;
	}

	template <typename... Args>
	T& emplace_back(Args&&... args) {
		push_back(T(std::forward<Args>(args)...));
		return (*this)[size() - 1];
	}

	std::size_t size() const noexcept { return _size; }

	T& operator[](std::size_t i) {
		details::boundary_check(i, _size, "StaticVector: operator[] on invalid index");
		return _elems[i];
	}

	const T& operator[](std::size_t i) const {
		details::boundary_check(i, _size, "StaticVector: const operator[] on invalid index");
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
