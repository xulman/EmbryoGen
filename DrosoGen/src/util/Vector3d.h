#ifndef VECTOR3D_H
#define VECTOR3D_H

#include <iostream>
#include <cmath>
#include <i3d/vector3d.h>
#include <functional>

/** simply a 3D vector... */
template <typename T>
class Vector3d
{
public:
	/** the vector data */
	T x,y,z;

	/** default constructor... */
	Vector3d(void) :
		x(0), y(0), z(0) {}

	/** init constructor... */
	Vector3d(const T xx,const T yy,const T zz) :
		x(xx), y(yy), z(zz) {}

	/** init constructor... */
	Vector3d(const T xyz) :
		x(xyz), y(xyz), z(xyz) {}

	/** copy constructor from a vector of the same type */
	Vector3d(const Vector3d<T>& vec)
	{
		this->x=vec.x;
		this->y=vec.y;
		this->z=vec.z;
	}

	/** copy constructor from i3d::Vector3d */
	Vector3d(const i3d::Vector3d<T>& iv3d)
	{
		fromI3dVector3d(iv3d);
	}

	Vector3d<T>& operator=(const Vector3d<T>& vec)
	{
		this->x=vec.x;
		this->y=vec.y;
		this->z=vec.z;
		return( *this );
	}

	Vector3d<T>& operator=(const T scalar)
	{
		this->x=scalar;
		this->y=scalar;
		this->z=scalar;
		return( *this );
	}

	Vector3d<T>& operator+=(const Vector3d<T>& vec)
	{
		this->x+=vec.x;
		this->y+=vec.y;
		this->z+=vec.z;
		return( *this );
	}

	Vector3d<T>& operator-=(const Vector3d<T>& vec)
	{
		this->x-=vec.x;
		this->y-=vec.y;
		this->z-=vec.z;
		return( *this );
	}

	Vector3d<T>& operator*=(const T& scal)
	{
		this->x*=scal;
		this->y*=scal;
		this->z*=scal;
		return( *this );
	}

	Vector3d<T>& operator/=(const T& scal)
	{
		this->x/=scal;
		this->y/=scal;
		this->z/=scal;
		return( *this );
	}

	T inline len(void) const
	{
		return static_cast<T>( std::sqrt(len2()) );
	}

	T inline len2(void) const
	{
		return (x*x + y*y + z*z);
	}

	Vector3d<T>& changeToUnitOrZero(void)
	{
		T l = this->len2();
		if (l > 0)
		{
			l = std::sqrt(l);
			x /= l;
			y /= l;
			z /= l;
		}
		//or l == 0 which means x == y == z == 0
		return *this;
	}

	/** element-wise minimum is stored in this vector */
	Vector3d<T>& elemMin(const Vector3d<T>& v)
	{
		x = x < v.x ? x : v.x;
		y = y < v.y ? y : v.y;
		z = z < v.z ? z : v.z;
		return *this;
	}

	/** element-wise maximum is stored in this vector */
	Vector3d<T>& elemMax(const Vector3d<T>& v)
	{
		x = x > v.x ? x : v.x;
		y = y > v.y ? y : v.y;
		z = z > v.z ? z : v.z;
		return *this;
	}

	/** element-wise multiplication is stored in this vector */
	Vector3d<T>& elemMult(const Vector3d<T>& v)
	{
		x *= v.x;
		y *= v.y;
		z *= v.z;
		return *this;
	}

	/** element-wise division is stored in this vector */
	Vector3d<T>& elemDivBy(const Vector3d<T>& v)
	{
		x /= v.x;
		y /= v.y;
		z /= v.z;
		return *this;
	}

	/** change this vector with element-wise absolute values */
	Vector3d<T>& elemAbs(void)
	{
		x = std::abs(x);
		y = std::abs(y);
		z = std::abs(z);
		return *this;
	}

	/** change this vector with element-wise squared values */
	Vector3d<T>& elemSquare(void)
	{
		x *= x;
		y *= y;
		z *= z;
		return *this;
	}

	bool elemIsLessThan(const Vector3d<T>& v) const
	{
		if (x >= v.x) return false;
		if (y >= v.y) return false;
		if (z >= v.z) return false;
		return true;
	}

	bool elemIsLessOrEqualThan(const Vector3d<T>& v) const
	{
		if (x > v.x) return false;
		if (y > v.y) return false;
		if (z > v.z) return false;
		return true;
	}

	bool elemIsGreaterThan(const Vector3d<T>& v) const
	{
		if (x <= v.x) return false;
		if (y <= v.y) return false;
		if (z <= v.z) return false;
		return true;
	}

	bool elemIsGreaterOrEqualThan(const Vector3d<T>& v) const
	{
		if (x < v.x) return false;
		if (y < v.y) return false;
		if (z < v.z) return false;
		return true;
	}

	bool elemIsPredicateTrue(const std::function<bool(const T)>& unaryPredicate)
	const
	{
		if (!unaryPredicate(x)) return false;
		if (!unaryPredicate(y)) return false;
		if (!unaryPredicate(z)) return false;
		return true;
	}

	bool elemIsPredicateTrue(const Vector3d<T>& v,
	                         const std::function<bool(const T,const T)>& binaryPredicate)
	const
	{
		if (!binaryPredicate(x,v.x)) return false;
		if (!binaryPredicate(y,v.y)) return false;
		if (!binaryPredicate(z,v.z)) return false;
		return true;
	}

	/** converts this pixel coordinate into this micron coordinate,
	    returns *this to allow for concatenating of commands... */
	Vector3d<T>& toMicrons(const Vector3d<T>& res, const Vector3d<T>& off)
	{
		x = x/res.x +off.x;
		y = y/res.y +off.y;
		z = z/res.z +off.z;
		return *this;
	}

	/** converts this micron coordinate into this pixel coordinate,
	    returns *this to allow for concatenating of commands... */
	Vector3d<T>& toPixels(const Vector3d<T>& res, const Vector3d<T>& off)
	{
		x = (x-off.x) *res.x;
		y = (y-off.y) *res.y;
		z = (z-off.z) *res.z;
		return *this;
	}

	/** converts from _the centre_ of the given pxIn coordinate into this micron coordinate,
	    returns *this to allow for concatenating of commands... */
	Vector3d<T>& toMicronsFrom(const Vector3d<size_t>& pxIn, const Vector3d<T>& res, const Vector3d<T>& off)
	{
		x = ( (T)pxIn.x +(T)0.5 )/res.x +off.x;
		y = ( (T)pxIn.y +(T)0.5 )/res.y +off.y;
		z = ( (T)pxIn.z +(T)0.5 )/res.z +off.z;
		return *this;
	}

	/** converts this micron coordinate into _nearest_ pxOut coordinate,
	    returns pxOut to allow for concatenating of commands... */
	Vector3d<size_t>& toPixels(Vector3d<size_t>& pxOut, const Vector3d<T>& res, const Vector3d<T>& off) const
	{
		//NB: +0.5 is to achieve proper-rounding when casting to size_t does down-rounding
		pxOut.x = (size_t)( ((x-off.x) *res.x) +(T)0.5 );
		pxOut.y = (size_t)( ((y-off.y) *res.y) +(T)0.5 );
		pxOut.z = (size_t)( ((z-off.z) *res.z) +(T)0.5 );
		return pxOut;
	}

	/** converts this coordinate into an image offset/index given the size of the image */
	size_t toImgIndex(const Vector3d<size_t>& imgSize) const
	{
		return ( (size_t)x + imgSize.x*( (size_t)y + imgSize.y*(size_t)z ) );
	}

	/** converts this coordinate into an image offset/index given the size of the image */
	size_t toImgIndex(const i3d::Vector3d<size_t>& imgSize) const
	{
		return ( (size_t)x + imgSize.x*( (size_t)y + imgSize.y*(size_t)z ) );
	}

	/** converts image offset/index into this image coordinate given the image size */
	Vector3d<size_t>& fromImgIndex(size_t idx, const Vector3d<size_t>& imgSize)
	{
		z    = idx / (imgSize.x*imgSize.y);
		idx -=  z  * (imgSize.x*imgSize.y);
		y    = idx /  imgSize.x;
		z    = idx -  imgSize.x * y;

		//compiler should complain when using this function when *this is non-size_t Vector3d
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
	Vector3d<T>& fromI3dVector3d(const i3d::Vector3d<T>& iv3d)
	{
		x = iv3d.x;
		y = iv3d.y;
		z = iv3d.z;
		return *this;
	}

	void toI3dVector3d(i3d::Vector3d<T>& iv3d) const
	{
		iv3d.x = x;
		iv3d.y = y;
		iv3d.z = z;
	}

	i3d::Vector3d<T> toI3dVector3d(void) const
	{
		return i3d::Vector3d<T>(x,y,z);
	}

	/** converts this vector into the "same" vector whose elements
	    are of another type (e.g., size_t -> float conversion),
	    creates and returns the converted vector */
	template <typename FT> //FT = foreign type
	Vector3d<FT> to(void) const
	{
		return Vector3d<FT>( (FT)x, (FT)y, (FT)z );
	}

	/** converts a given vector whose elements are of another type
	    into this vector (e.g., size_t -> float conversion), returns
	    *this to allow for concatenating of commands... */
	template <typename FT> //FT = foreign type
	Vector3d<T>& from(const Vector3d<FT>& v)
	{
		x = (T)v.x;
		y = (T)v.y;
		z = (T)v.z;
		return *this;
	}
};

/** reports vector (with parentheses) */
template <typename T>
std::ostream& operator<<(std::ostream& s,const Vector3d<T>& v)
{
	s << "(" << v.x << "," << v.y << "," << v.z << ")";
	return s;
}

/** function for dot product of two vectors */
template <typename T>
T dotProduct(const Vector3d<T>& u, const Vector3d<T>& v)
{
	return (u.x*v.x + u.y*v.y + u.z*v.z);
}

/** calculates addition of two vectors: vecA + vecB */
template <typename T>
Vector3d<T> operator+(const Vector3d<T>& vecA, const Vector3d<T>& vecB)
{
	Vector3d<T> res(vecA);
	res+=vecB;
	return (res);
}

/** calculates difference of two vectors: vecA - vecB */
template <typename T>
Vector3d<T> operator-(const Vector3d<T>& vecA, const Vector3d<T>& vecB)
{
	Vector3d<T> res(vecA);
	res-=vecB;
	return (res);
}

/** calculates scalar multiplication with vector: scal * vec */
template <typename T>
Vector3d<T> operator*(const T scal, const Vector3d<T>& vec)
{
	Vector3d<T> res(vec);
	res *= scal;
	return (res);
}
// ----------------------------------------------------------------------------

/** a dedicated type just to differentiate formally vector from coordinate,
    and is used mainly in conjunction with operator<<

    Alternatively, one could design this class to hold a reference on Vector3d<>
    to avoid duplication (copying) of the vector but then one would loose the
    ability to use the (math) operations already defined for Vector3d<>. */
template <typename T>
class Coord3d : public Vector3d<T>
{
public:
	/** a default empty constructor */
	Coord3d(void)
		: Vector3d<T>(0) {}

	/** a copy constructor to "convert" pure vector into a coordinate */
	Coord3d(const Vector3d<T>& vec)
		: Vector3d<T>(vec) {}

	/** an init constructor to "convert" x,y,z into a coordinate */
	Coord3d(const T x, const T y, const T z)
		: Vector3d<T>(x,y,z) {}
};

/** reports position coordinate (with square brackets) */
template <typename T>
std::ostream& operator<<(std::ostream& s,const Coord3d<T>& v)
{
	s << "[" << v.x << "," << v.y << "," << v.z << "]";
	return s;
}
// ----------------------------------------------------------------------------


/** current (lightweight) representation of the force name */
typedef const char* ForceName;

/** a placeholder for ForceName type of "unknown force",
    to prevent of allocating it over and over again */
static ForceName unknownForceType = "unknown force";

/** essentially a "named" and "postioned" 3D vector */
template <typename T>
class ForceVector3d : public Vector3d<T>
{
public:
	/** position where this force is acting
	    (where it is anchored; where is the base of the force vector) */
	Vector3d<T> base;

	/** aux optional information about the force, often used to index
	    the anchor (ForceVector3d::base) of this force */
	long hint;

	/** type of the force (only to find out how to report it) */
	ForceName type;

	/** default constructor... */
	ForceVector3d(void) : Vector3d<T>(), base(), type(unknownForceType) {}

	/** init constructor... */
	ForceVector3d(const T xx,const T yy,const T zz,
	              const Vector3d<T>& _base, const ForceName _type)
		: Vector3d<T>(xx,yy,zz), base(_base), hint(0), type(_type) {}

	/** init constructor... */
	ForceVector3d(const T xyz,
	              const Vector3d<T>& _base, const ForceName _type)
		: Vector3d<T>(xyz), base(_base), hint(0), type(_type) {}

	/** init constructor... */
	ForceVector3d(const Vector3d<T>& v,
	              const Vector3d<T>& _base, const ForceName _type)
		: Vector3d<T>(v), base(_base), hint(0), type(_type) {}

	/** init constructor with explicit hint... */
	ForceVector3d(const Vector3d<T>& v,
	              const Vector3d<T>& _base, const long _hint, const ForceName _type)
		: Vector3d<T>(v), base(_base), hint(_hint), type(_type) {}

	/** copy constructor... */
	ForceVector3d(const ForceVector3d<T>& vec)
		: Vector3d<T>(vec)
	{
		this->base = vec.base;
		this->hint = vec.hint;
		this->type = vec.type;
	}

	ForceVector3d<T>& operator=(const ForceVector3d<T>& vec)
	{
		this->x    = vec.x;
		this->y    = vec.y;
		this->z    = vec.z;
		this->base = vec.base;
		this->hint = vec.hint;
		this->type = vec.type;
		return( *this );
	}

	ForceVector3d<T>& operator=(const Vector3d<T>& vec)
	{
		this->x    = vec.x;
		this->y    = vec.y;
		this->z    = vec.z;
		this->base = 0;
		this->hint = 0;
		this->type = unknownForceType;
		return( *this );
	}

	ForceVector3d<T>& operator=(const T scalar)
	{
		this->x = this->y = this->z = scalar;
		this->base = 0;
		this->hint = 0;
		this->type = unknownForceType;
		return( *this );
	}
};

/** reports force as a force type, force vector and position coordinate */
template <typename T>
std::ostream& operator<<(std::ostream& s,const ForceVector3d<T>& f)
{
	s << f.type << ": (" << f.x << "," << f.y << "," << f.z
	  << ") @ [" << f.base.x << "," << f.base.y << "," << f.base.z << "]";
	return s;
}
#endif
