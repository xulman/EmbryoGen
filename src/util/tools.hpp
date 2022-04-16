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

// TODO fix non-default constructible data
template <typename T, std::size_t N>
class SmallVector {
  private:
	std::array<T, N> _static_data;
	std::vector<T> _dynamic_data;
	std::size_t _size = 0;

	void arr_to_vec() {
		_dynamic_data.insert(_dynamic_data.begin(), _static_data.begin(),
		                     _static_data.end());
	}

  public:
	using value_type = T;
	constexpr static std::size_t static_size = N;

	std::size_t size() const { return _size; }
	void push_back(T elem) {
		if (_size < N)
			_static_data[_size] = std::move(elem);
		else {
			if (_size == N)
				arr_to_vec();

			_dynamic_data.push_back(std::forward<T>(elem));
		}
		++_size;
	}

	void clear() {
		_dynamic_data.clear();
		_size = 0;
	}

	// TODO create full iterator
	T* begin() {
		if (_size < N)
			return &_static_data[0];
		return &_dynamic_data[0];
	}

	T* end() {
		if (_size < N)
			return &_static_data[_size];
		return &_dynamic_data[_size];
	}

	const T* begin() const {
		if (_size < N)
			return &_static_data[0];
		return &_dynamic_data[0];
	}

	const T* end() const {
		if (_size < N)
			return &_static_data[_size];
		return &_dynamic_data[_size];
	}

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
};

// TODO fix non-default constructible data
template <typename T, std::size_t N>
class StaticVector {
	std::array<T, N> _data;
	std::size_t _size = 0;

  public:
	using value_type = T;
	constexpr static std::size_t static_size = N;

	void clear() { _size = 0; }
	void push_back(T elem) {
		details::boundary_check(_size, N);
		_data[_size] = std::move(elem);
		++_size;
	}

	std::size_t size() const { return _size; }
	auto begin() { return _data.begin(); }

	auto end() { return _data.begin() + _size; }

	auto begin() const { return _data.begin(); }

	auto end() const { return _data.begin() + _size; }

	T& operator[](std::size_t i) {
		details::boundary_check(i, _size);
		return _data[i];
	}

	const T& operator[](std::size_t i) const {
		details::boundary_check(i, _size);
		return _data[i];
	}
};

} // namespace structures

} // namespace tools

