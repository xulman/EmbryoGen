#pragma once

#include <array>
#include <concepts>
#include <stdexcept>
#include <type_traits>
#include <vector>

namespace details {
inline void boundary_check(std::size_t i, std::size_t size) {
#ifndef NDEBUG
	using namespace std::string_literals;
	if (i >= size)
		throw std::out_of_range("Index "s + std::to_string(i) +
		                        " is >= size (" + std::to_string(size) + ")\n");
#endif
}

template <typename T>
class ptr_iterator {
	T* _ptr = nullptr;

  public:
	ptr_iterator() = default;
	ptr_iterator(T* ptr) : _ptr(ptr) {}

	ptr_iterator& operator++() {
		++_ptr;
		return *this;
	}
	ptr_iterator operator++(int) {
		ptr_iterator cpy = *this;
		++(*this);
		return cpy;
	}
	T& operator*() { return *_ptr; }
	T* operator->() { return _ptr; }
	friend auto operator<=>(const ptr_iterator& lhs,
	                        const ptr_iterator& rhs) = default;
};

template <typename T>
class const_ptr_iterator {
	const T* _ptr = nullptr;

  public:
	const_ptr_iterator() = default;
	const_ptr_iterator(const T* ptr) : _ptr(ptr) {}

	const_ptr_iterator& operator++() {
		++_ptr;
		return *this;
	}
	const_ptr_iterator operator++(int) {
		ptr_iterator cpy = *this;
		++(*this);
		return cpy;
	}
	const T& operator*() const { return *_ptr; }
	const T* operator->() const { return _ptr; }

	friend auto operator<=>(const const_ptr_iterator& lhs,
	                        const const_ptr_iterator& rhs) = default;
};

} // namespace details

namespace tools {
namespace concepts {
template <typename T>
concept basic_container = requires(T a, T::value_type b) {
	{ a.size() } -> std::convertible_to<std::size_t>;
	{a.push_back(b)};
	{a.begin()};
	{a.end()};
	{a.clear()};
};
} // namespace concepts

namespace structures {

template <typename T, std::size_t N>
class SmallVector {
  private:
	char _static_data[sizeof(T) * N];
	T* _elems = (T*)(&_static_data[0]);
	std::vector<T> _dynamic_data;
	std::size_t _size = 0;

	void arr_to_vec() {
		_dynamic_data.reserve(N * 2);
		for (auto& val : *this) {
			_dynamic_data.push_back(std::move(val));
		}
	}

	void destroy_elems() {
		for (std::size_t i = 0; i < std::min(N, _size); ++i) {
			_elems[i].~T();
		}
	}

  public:
	using value_type = T;
	constexpr static std::size_t static_size = N;

	SmallVector() = default;
	SmallVector(const SmallVector&) = default;
	SmallVector(SmallVector&&) = default;
	SmallVector<T, N>& operator=(const SmallVector&) = default;
	SmallVector<T, N>& operator=(SmallVector&&) = default;

	std::size_t size() const { return _size; }
	void push_back(T elem) {
		if (_size < N)
			std::uninitialized_move_n(&elem, 1, _elems + _size);
		else {
			if (_size == N)
				arr_to_vec();

			_dynamic_data.push_back(std::forward<T>(elem));
		}
		++_size;
	}

	void clear() {
		destroy_elems();
		_dynamic_data.clear();
		_size = 0;
	}

	auto begin() noexcept {
		if (_size < N)
			return details::ptr_iterator(&_elems[0]);
		return details::ptr_iterator(&_dynamic_data[0]);
	}
	auto end() noexcept {
		if (_size < N)
			return details::ptr_iterator(&_elems[_size]);
		return details::ptr_iterator(&_dynamic_data[_size]);
	}
	auto cbegin() const noexcept {
		if (_size < N)
			return details::const_ptr_iterator(&_elems[0]);
		return details::const_ptr_iterator(&_dynamic_data[0]);
	}
	auto cend() const noexcept {
		if (_size < N)
			return details::const_ptr_iterator(&_elems[_size]);
		return details::const_ptr_iterator(&_dynamic_data[_size]);
	}
	auto begin() const noexcept { return cbegin(); }
	auto end() const noexcept { return cend(); }

	T& operator[](std::size_t i) {
		details::boundary_check(i, _size);

		if (i < N) {
			return _static_data[i];
		}
		return _dynamic_data[i];
	}

	const T& operator[](std::size_t i) const {
		details::boundary_check(i, _size);

		if (i < N) {
			return _static_data[i];
		}
		return _dynamic_data[i];
	}

	~SmallVector() { destroy_elems(); }
};

template <typename T, std::size_t N>
class StaticVector {
	char _data[sizeof(T) * N];
	T* _elems = (T*)(&_data[0]);
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
