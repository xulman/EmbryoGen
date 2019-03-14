#ifndef VECTOR3D_H
#define VECTOR3D_H

#include <iostream>
#include <cmath>
#include <i3d/vector3d.h>

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

	/** element-wise minimum is stored in this vector */
	void elemMin(const Vector3d<T>& v)
	{
		x = x < v.x ? x : v.x;
		y = y < v.y ? y : v.y;
		z = z < v.z ? z : v.z;
	}

	/** element-wise maximum is stored in this vector */
	void elemMax(const Vector3d<T>& v)
	{
		x = x > v.x ? x : v.x;
		y = y > v.y ? y : v.y;
		z = z > v.z ? z : v.z;
	}

	/** element-wise multiplication is stored in this vector */
	void elemMult(const Vector3d<T>& v)
	{
		x *= v.x;
		y *= v.y;
		z *= v.z;
	}

	/** element-wise division is stored in this vector */
	void elemDivBy(const Vector3d<T>& v)
	{
		x /= v.x;
		y /= v.y;
		z /= v.z;
	}

	void changeToUnitOrZero(void)
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

	void fromI3dVector3d(const i3d::Vector3d<T>& iv3d)
	{
		x = iv3d.x;
		y = iv3d.y;
		z = iv3d.z;
	}
	void toI3dVector3d(i3d::Vector3d<T>& iv3d) const
	{
		iv3d.x = x;
		iv3d.y = y;
		iv3d.z = z;
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
