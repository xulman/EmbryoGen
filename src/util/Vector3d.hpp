#pragma once

#include <algorithm>
#include <cmath>
#include <concepts>
#include <fmt/core.h>
#include <functional>
#include <i3d/vector3d.h>
#include <iostream>
#include <type_traits>

/** simply a 3D vector... */
template <typename T>
requires std::is_scalar_v<T>
class Vector3d {
  private:
	Vector3d<T>& _applyToElems(auto unaryOp) {
		x = unaryOp(x);
		y = unaryOp(y);
		z = unaryOp(z);
		return *this;
	}

	template <typename U>
	Vector3d<T>& _applyToElems(auto binaryOp, const Vector3d<U>& vec) {
		x = binaryOp(x, vec.x);
		y = binaryOp(y, vec.y);
		z = binaryOp(z, vec.z);

		return *this;
	}

  public:
	using value_type = T;

	/** the vector data */
	T x = 0, y = 0, z = 0;

	/** default constructor... */
	Vector3d(void) = default;

	/** init constructor... */
	Vector3d(const T xx, const T yy, const T zz) : x(xx), y(yy), z(zz) {}

	/** init constructor... */
	Vector3d(const T xyz) : x(xyz), y(xyz), z(xyz) {}

	/** enable implicit conversion */
	template <typename U>
	requires std::is_convertible_v<U, T> Vector3d(const Vector3d<U> vec)
	    : x(T(vec.x)), y(T(vec.y)), z(T(vec.z)) {}

	/** copy constructor from i3d::Vector3d */
	Vector3d(const i3d::Vector3d<T>& iv3d) { fromI3dVector3d(iv3d); }

	T& operator[](std::size_t i) {
		switch (i) {
		case 0:
			return x;
		case 1:
			return y;
		case 2:
			return z;
		}
		throw std::out_of_range(
		    fmt::format("Index: {} is out of range\n", i).c_str());
	}

	T operator[](std::size_t i) const {
		switch (i) {
		case 0:
			return x;
		case 1:
			return y;
		case 2:
			return z;
		}
		throw std::out_of_range(
		    fmt::format("Index: {} is out of range\n", i).c_str());
	}

	template <typename U>
	requires std::is_convertible_v<U, T> Vector3d<T>
	&operator=(const U scal) {
		return _applyToElems([=](T) { return T(scal); });
	}

	Vector3d<T>& operator+=(const Vector3d<T>& vec) {
		return _applyToElems(std::plus{}, vec);
	}

	Vector3d<T>& operator-=(const Vector3d<T>& vec) {
		return _applyToElems(std::minus{}, vec);
	}

	template <typename U>
	requires std::is_convertible_v<U, T> Vector3d<T>
	&operator*=(const U scal) {
		return _applyToElems([=](T a) { return a * T(scal); });
	}

	template <typename U>
	requires std::is_convertible_v<U, T> Vector3d<T>
	&operator/=(const U scal) {
		return _applyToElems([=](T a) { return a / T(scal); });
	}

	T inline len(void) const { return static_cast<T>(std::sqrt(len2())); }

	T inline len2(void) const { return (x * x + y * y + z * z); }

	Vector3d<T>& changeToUnitOrZero(void) {
		T l = len2();
		if (l > 0) {
			l = std::sqrt(l);
			*this /= l;
		}
		// or l == 0 which means x == y == z == 0
		return *this;
	}

	/** element-wise minimum is stored in this vector */
	Vector3d<T>& elemMin(const Vector3d<T>& vec) {
		return _applyToElems([](T a, T b) { return std::min(a, b); }, vec);
	}

	/** element-wise maximum is stored in this vector */
	Vector3d<T>& elemMax(const Vector3d<T>& vec) {
		return _applyToElems([](T a, T b) { return std::max(a, b); }, vec);
	}

	/** element-wise multiplication is stored in this vector */
	Vector3d<T>& elemMult(const Vector3d<T>& vec) {
		return _applyToElems(std::multiplies{}, vec);
	}

	/** element-wise division is stored in this vector */
	Vector3d<T>& elemDivBy(const Vector3d<T>& vec) {
		return _applyToElems(std::divides{}, vec);
	}

	/** change this vector with element-wise absolute values */
	Vector3d<T>& elemAbs(void) {
		return _applyToElems([](T a) { return std::abs(a); });
	}

	/** change this vector with element-wise squared values */
	Vector3d<T>& elemSquare(void) { return elemMult(*this); }

	Vector3d<T>& elemFloor(void) {
		return _applyToElems([](T a) { return std::floor(a); });
	}

	Vector3d<T>& elemRound(void) {
		return _applyToElems([](T a) { return std::round(a); });
	}

	Vector3d<T>& elemCeil(void) {
		return _applyToElems([](T a) { return std::ceil(a); });
	}

	template <typename unaryOp_t>
	requires std::is_convertible_v<std::result_of_t<unaryOp_t(T)>, T>
	    Vector3d<T>
	&elemMathOp(unaryOp_t unaryOp) { return _applyToElems(unaryOp); }

	bool elemIsLessThan(const Vector3d<T>& v) const {
		if (x >= v.x)
			return false;
		if (y >= v.y)
			return false;
		if (z >= v.z)
			return false;
		return true;
	}

	bool elemIsLessOrEqualThan(const Vector3d<T>& v) const {
		if (x > v.x)
			return false;
		if (y > v.y)
			return false;
		if (z > v.z)
			return false;
		return true;
	}

	bool elemIsGreaterThan(const Vector3d<T>& v) const {
		if (x <= v.x)
			return false;
		if (y <= v.y)
			return false;
		if (z <= v.z)
			return false;
		return true;
	}

	bool elemIsGreaterOrEqualThan(const Vector3d<T>& v) const {
		if (x < v.x)
			return false;
		if (y < v.y)
			return false;
		if (z < v.z)
			return false;
		return true;
	}

	bool elemIsPredicateTrue(
	    const std::function<bool(const T)>& unaryPredicate) const {
		if (!unaryPredicate(x))
			return false;
		if (!unaryPredicate(y))
			return false;
		if (!unaryPredicate(z))
			return false;
		return true;
	}

	bool elemIsPredicateTrue(
	    const Vector3d<T>& v,
	    const std::function<bool(const T, const T)>& binaryPredicate) const {
		if (!binaryPredicate(x, v.x))
			return false;
		if (!binaryPredicate(y, v.y))
			return false;
		if (!binaryPredicate(z, v.z))
			return false;
		return true;
	}

	/** converts this pixel coordinate into this micron coordinate,
	    returns *this to allow for concatenating of commands... */
	Vector3d<T>& toMicrons(const Vector3d<T>& res, const Vector3d<T>& off) {
		x = x / res.x + off.x;
		y = y / res.y + off.y;
		z = z / res.z + off.z;
		return *this;
	}

	/** converts this pixel coordinate into this micron coordinate,
	    returns *this to allow for concatenating of commands... */
	Vector3d<T>& toMicrons(const i3d::Vector3d<T>& res,
	                       const i3d::Vector3d<T>& off) {
		x = x / res.x + off.x;
		y = y / res.y + off.y;
		z = z / res.z + off.z;
		return *this;
	}

	/** converts this micron coordinate into this pixel coordinate,
	    returns *this to allow for concatenating of commands... */
	Vector3d<T>& toPixels(const Vector3d<T>& res, const Vector3d<T>& off) {
		x = (x - off.x) * res.x;
		y = (y - off.y) * res.y;
		z = (z - off.z) * res.z;
		return *this;
	}

	/** converts this micron coordinate into this pixel coordinate,
	    returns *this to allow for concatenating of commands... */
	Vector3d<T>& toPixels(const i3d::Vector3d<T>& res,
	                      const i3d::Vector3d<T>& off) {
		x = (x - off.x) * res.x;
		y = (y - off.y) * res.y;
		z = (z - off.z) * res.z;
		return *this;
	}

	/** converts from floating-point pixel coordinate 'floatPxPos' into this
	   integer pixel coordinate (which is still stored with floating-point
	   precision/type), the down-rounding implements the same
	   real-px-coord-to-int-px-coord policy as in the fromMicronsTo(), returns
	   *this to allow for concatenating of commands... */
	template <typename FT> // FT = foreign type
	Vector3d<T>& toPixels(const Vector3d<FT>& floatPxPos) {
		_applyToElems([](T, FT a) { return T(a); }, floatPxPos);
		if constexpr (std::is_floating_point_v<T>)
			_applyToElems([](T a) { return std::floor(a); });

		return *this;

		// or just:
		// return from(floatPxPos).elemFloor();
		// NB: this would, however, complain when template parameter T is some
		// integer type
		//     because std::floor(), used inside elemFloor(), does not exist for
		//     integer types and so std::floor<double>() is used returning
		//     double implicitly converted into the integer type (and producing
		//     a warning about the possibly-lossy conversion)
	}

	/** converts this floating-point pixel coordinate into this integer pixel
	   coordinate (which is still stored with floating-point precision/type),
	   the down-rounding implements the same real-px-coord-to-int-px-coord
	   policy as in the fromMicronsTo(), returns *this to allow for
	   concatenating of commands... */
	Vector3d<T>& toPixels(void) {
		if constexpr (std::is_floating_point_v<T>)
			_applyToElems([](T a) { return std::floor(a); });

		return *this;
	}

	/** converts from _the centre_ of the given pxIn coordinate into this micron
	   coordinate, returns *this to allow for concatenating of commands... */
	Vector3d<T>& toMicronsFrom(const Vector3d<std::size_t>& pxIn,
	                           const Vector3d<T>& res,
	                           const Vector3d<T>& off) {
		x = (T(pxIn.x) + T(0.5)) / res.x + off.x;
		y = (T(pxIn.y) + T(0.5)) / res.y + off.y;
		z = (T(pxIn.z) + T(0.5)) / res.z + off.z;
		return *this;
	}

	/** converts this micron coordinate into a containing voxel, whose
	   coordinate is stored into pxOut; returns pxOut to allow for concatenating
	   of commands... */
	Vector3d<std::size_t>& fromMicronsTo(Vector3d<std::size_t>& pxOut,
	                                     const Vector3d<T>& res,
	                                     const Vector3d<T>& off) const {
		// the real-px-coord-to-int-px-coord policy
		pxOut.x = std::size_t((x - off.x) * res.x);
		pxOut.y = std::size_t((y - off.y) * res.y);
		pxOut.z = std::size_t((z - off.z) * res.z);
		return pxOut;
	}

	/** converts this coordinate into an image offset/index given the size of
	 * the image */
	size_t toImgIndex(const Vector3d<std::size_t>& imgSize) const {
		return std::size_t(x) +
		       imgSize.x * (std::size_t(y) + imgSize.y * std::size_t(z));
	}

	/** converts this coordinate into an image offset/index given the size of
	 * the image */
	size_t toImgIndex(const i3d::Vector3d<std::size_t>& imgSize) const {
		return std::size_t(x) +
		       imgSize.x * (std::size_t(y) + imgSize.y * std::size_t(z));
	}

	/** converts image offset/index into this image coordinate given the image
	 * size */
	Vector3d<std::size_t>& fromImgIndex(std::size_t idx,
	                                    const Vector3d<size_t>& imgSize) {
		z = idx / (imgSize.x * imgSize.y);
		idx -= z * (imgSize.x * imgSize.y);
		y = idx / imgSize.x;
		z = idx - imgSize.x * y;

		// compiler should complain when using this function when *this is
		// non-size_t Vector3d
		return *this;
	}

	/** converts image offset/index into the output image coordinate 'imgPos'
	    given the image size stored in this vector
	Vector3d<size_t>& fromImgIndex(size_t idx, const Vector3d<size_t>& imgPos)
	{
	    imgPos.z  =    idx     / (x*y);
	    idx      -= imgPos.z  * (x*y);
	    imgPos.y  =    idx     /  x;
	    imgPos.z  =    idx     -  x * imgPos.y;

	    return *this;
	}
	*/

	/** type converts from i3d::Vector3d<> into this vector, also returns
	    this vector to allow for operations chaining */
	Vector3d<T>& fromI3dVector3d(const i3d::Vector3d<T>& iv3d) {
		x = iv3d.x;
		y = iv3d.y;
		z = iv3d.z;
		return *this;
	}

	void toI3dVector3d(i3d::Vector3d<T>& iv3d) const {
		iv3d.x = x;
		iv3d.y = y;
		iv3d.z = z;
	}

	i3d::Vector3d<T> toI3dVector3d(void) const {
		return i3d::Vector3d<T>(x, y, z);
	}

	template <typename FT>
	requires std::is_convertible_v<T, FT>
	void toScalars(FT& x, FT& y, FT& z) const {
		x = this->x;
		y = this->y;
		z = this->z;
	}

	std::tuple<T, T, T> toScalars(void) const { return {x, y, z}; }

	/** converts this vector into the "same" vector whose elements
	    are of another type (e.g., size_t -> float conversion),
	    creates and returns the converted vector */
	template <typename FT>
	requires std::is_convertible_v<T, FT> // FT = foreign type
	    Vector3d<FT> to(void)
	const { return Vector3d<FT>(FT(x), FT(y), FT(z)); }

	/** converts a given vector whose elements are of another type
	    into this vector (e.g., size_t -> float conversion), returns
	    *this to allow for concatenating of commands... */
	template <typename FT>
	requires std::is_convertible_v<FT, T> // FT = foreign type
	    Vector3d<T>
	&from(const Vector3d<FT>& vec) {
		return _applyToElems([](T, FT a) { return T(a); }, vec);
	}

	/** resets this vector with the elements (that may be of another
	    type), returns *this to allow for concatenating of commands... */
	template <typename FT>
	requires std::is_convertible_v<FT, T> // FT = foreign type
	    Vector3d<T>
	&from(const FT xx, const FT yy, const FT zz) {
		x = T(xx);
		y = T(yy);
		z = T(zz);
		return *this;
	}

	/** resets this vector's elems with the same element (that may be of another
	    type), returns *this to allow for concatenating of commands... */

	template <typename FT>
	requires std::is_convertible_v<FT, T> // FT = foreign type
	    Vector3d<T>
	&from(const FT xyz) {
		return _applyToElems([=](T) { return T(xyz); });
	}
};

/** reports vector (with parentheses) */
template <typename T>
std::ostream& operator<<(std::ostream& s, const Vector3d<T>& v) {
	s << fmt::format("({}, {}, {})", v.x, v.y, v.z);
	return s;
}

/** function for dot (scalar) product of two vectors */
template <typename T>
T dotProduct(const Vector3d<T>& u, const Vector3d<T>& v) {
	return (u.x * v.x + u.y * v.y + u.z * v.z);
}

/** function for cross (vector) product of two vectors */
template <typename T>
Vector3d<T> crossProduct(const Vector3d<T>& u, const Vector3d<T>& v) {
	return Vector3d<T>(u.y * v.z - u.z * v.y, u.z * v.x - u.x * v.z,
	                   u.x * v.y - u.y * v.x);
}

/** calculates addition of two vectors: vecA + vecB */
template <typename T, typename U>
requires std::is_convertible_v<U, T> Vector3d<T>
operator+(const Vector3d<T>& vecA, const Vector3d<U>& vecB) {
	Vector3d<T> res(vecA);
	res += vecB;
	return res;
}

/** calculates difference of two vectors: vecA - vecB */
template <typename T, typename U>
requires std::is_convertible_v<U, T> Vector3d<T>
operator-(const Vector3d<T>& vecA, const Vector3d<U>& vecB) {
	Vector3d<T> res(vecA);
	res -= vecB;
	return res;
}

/** calculates scalar multiplication with vector: scal * vec */
template <typename T, typename U>
requires std::is_convertible_v<U, T> Vector3d<T>
operator*(const U scal, const Vector3d<T>& vec) {
	Vector3d<T> res(vec);
	res *= scal;
	return res;
}
// ----------------------------------------------------------------------------

/** a dedicated type just to differentiate formally vector from coordinate,
    and is used mainly in conjunction with operator<<

    Alternatively, one could design this class to hold a reference on Vector3d<>
    to avoid duplication (copying) of the vector but then one would loose the
    ability to use the (math) operations already defined for Vector3d<>. */
template <typename T>
class Coord3d : public Vector3d<T> {
  public:
	/** a default empty constructor */
	Coord3d(void) = default;

	/** a copy constructor to "convert" pure vector into a coordinate */
	Coord3d(const Vector3d<T>& vec) : Vector3d<T>(vec) {}

	/** an init constructor to "convert" x,y,z into a coordinate */
	Coord3d(const T x, const T y, const T z) : Vector3d<T>(x, y, z) {}
};

/** reports position coordinate (with square brackets) */
template <typename T>
std::ostream& operator<<(std::ostream& s, const Coord3d<T>& v) {
	s << "[" << v.x << "," << v.y << "," << v.z << "]";
	return s;
}
// ----------------------------------------------------------------------------

/** current (lightweight) representation of the force name */
using ForceName_t = std::string;

/** a placeholder for ForceName type of "unknown force",
    to prevent of allocating it over and over again */
static ForceName_t unknownForceType = "unknown force";

/** essentially a "named" and "postioned" 3D vector */
template <typename T>
class ForceVector3d : public Vector3d<T> {
  public:
	/** position where this force is acting
	    (where it is anchored; where is the base of the force vector) */
	Vector3d<T> base;

	/** aux optional information about the force, often used to index
	    the anchor (ForceVector3d::base) of this force */
	long hint = 0;

	/** type of the force (only to find out how to report it) */
	ForceName_t type;

	/** default constructor... */
	ForceVector3d(void) : Vector3d<T>(), base(), type(unknownForceType) {}

	/** init constructor... */
	ForceVector3d(const T xx,
	              const T yy,
	              const T zz,
	              Vector3d<T> _base,
	              ForceName_t _type)
	    : Vector3d<T>(xx, yy, zz), base(std::move(_base)),
	      type(std::move(_type)) {}

	/** init constructor... */
	ForceVector3d(const T xyz, Vector3d<T> _base, ForceName_t _type)
	    : Vector3d<T>(xyz), base(std::move(_base)), type(std::move(_type)) {}

	/** init constructor... */
	ForceVector3d(Vector3d<T> v, Vector3d<T> _base, ForceName_t _type)
	    : Vector3d<T>(std::move(v)), base(std::move(_base)),
	      type(std::move(_type)) {}

	/** init constructor with explicit hint... */
	ForceVector3d(Vector3d<T> v,
	              Vector3d<T> _base,
	              const long _hint,
	              ForceName_t _type)
	    : Vector3d<T>(std::move(v)), base(std::move(_base)), hint(_hint),
	      type(std::move(_type)) {}

	/** copy constructor... */
	ForceVector3d(const ForceVector3d<T>& vec) = default;

	ForceVector3d<T>& operator=(const ForceVector3d<T>& vec) = default;

	ForceVector3d<T>& operator=(const Vector3d<T>& vec) {
		this->x = vec.x;
		this->y = vec.y;
		this->z = vec.z;
		base = 0;
		hint = 0;
		type = unknownForceType;
		return *this;
	}

	ForceVector3d<T>& operator=(const T scalar) {
		this->x = this->y = this->z = scalar;
		base = 0;
		hint = 0;
		type = unknownForceType;
		return *this;
	}
};

/** reports force as a force type, force vector and position coordinate */
template <typename T>
std::ostream& operator<<(std::ostream& s, const ForceVector3d<T>& f) {
	s << f.type << ": (" << f.x << "," << f.y << "," << f.z << ") @ ["
	  << f.base.x << "," << f.base.y << "," << f.base.z << "]";
	return s;
}