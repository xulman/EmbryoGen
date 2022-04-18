#pragma once

#include "../__details.hpp"
#include <vector>

namespace tools {

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

template <typename T>
using SmallVector3 = SmallVector<T, 3>;

template <typename T>
using SmallVector4 = SmallVector<T, 4>;

template <typename T>
using SmallVector5 = SmallVector<T, 5>;

template <typename T>
using SmallVector10 = SmallVector<T, 10>;

} // namespace structures

} // namespace tools
