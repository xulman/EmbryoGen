#pragma once

#include "../__details.hpp"
#include <vector>
#include <cstring>

namespace tools {

namespace structures {

template <typename T, std::size_t N>
class SmallVector {
  private:
	char __raw_data[N * sizeof(T)];
	T* _static_data = reinterpret_cast<T*>(&__raw_data[0]);
	std::vector<T> _dynamic_data;
	std::size_t _size = 0;
	bool _in_dynamic = false;

	void arr_to_vec() {
		_dynamic_data.reserve(N * 2);
		for (std::size_t i = 0; i < N; ++i)
			_dynamic_data.push_back(std::move(_static_data[i]));
		_in_dynamic = true;
	}

	void destroy_static_data() {
		for (std::size_t i = 0; i < std::min(N, _size); ++i) {
			_static_data[i].~T();
		}
	}

	T* valid_data() {
		if (!_in_dynamic)
			return _static_data;
		return &_dynamic_data[0];
	}

	const T* valid_data() const {
		if (!_in_dynamic)
			return _static_data;
		return &_dynamic_data[0];
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
			std::uninitialized_move_n(&elem, 1, _static_data + _size);
		else {
			if (_size == N)
				arr_to_vec();

			_dynamic_data.push_back(std::forward<T>(elem));
		}
		++_size;
	}

	void erase_at(std::size_t idx)
	{
		details::boundary_check(idx, _size);

		if (!_in_dynamic)
		{
			_static_data[idx].~T();
			std::memmove(
				__raw_data + idx * sizeof(T), 		// dest
				__raw_data + (idx + 1) * sizeof(T), // src
				sizeof(T) 							// count in bytes
			);
		}
		else
		{
			_dynamic_data.erase(_dynamic_data.begin() + idx);
		}
		--_size;
	}

	T& back() { return (*this)[size() - 1]; }
	const T& back() const { return (*this)[size() - 1]; }

	template <typename... Args>
	T& emplace_back(Args&&... args) {
		push_back(T(std::forward<Args>(args)...));
		return back();
	}

	void pop_back() {
		details::boundary_check(0, _size);

		if (_in_dynamic)
			_dynamic_data.pop_back();
		else
			back().~T();

		--size;
	}

	void clear() {
		destroy_static_data();
		_dynamic_data.clear();
		_size = 0;
	}

	auto begin() noexcept { return details::ptr_iterator(valid_data()); }
	auto end() noexcept { return details::ptr_iterator(valid_data() + _size); }
	auto cbegin() const noexcept { return details::ptr_iterator(valid_data()); }
	auto cend() const noexcept {
		return details::ptr_iterator(valid_data() + _size);
	}
	auto begin() const noexcept { return cbegin(); }
	auto end() const noexcept { return cend(); }

	T& operator[](std::size_t i) {
		details::boundary_check(i, _size);
		return valid_data()[i];
	}

	const T& operator[](std::size_t i) const {
		details::boundary_check(i, _size);
		return valid_data()[i];
	}

	~SmallVector() { destroy_static_data(); }
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
