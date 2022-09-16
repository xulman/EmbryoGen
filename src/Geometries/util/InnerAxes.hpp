#pragma once

#include <cmath>
#include "../../util/Vector3d.hpp"

/**
 * This is a container to represent a base of a 3D vector space that
 * was intended to represented an embedded Cartesian coordinate system.
 * Alternative term (that actually made it to the class name) is
 * "inner axes" (within, e.g., a sphere).
 *
 * Indeed, it can be useful, e.g., for representing positions of texture
 * particles in coordinates that are intrinsic to the containing sphere
 * (thus independent to the actual sphere position and orientation).
 */
template <typename FT>
class InnerAxes
{
public:
	InnerAxes(void)
		: mA(1,0,0), sA(0,1,0), tA(0,0,1) {}

	Vector3d<FT> mA; //mainAxis
	Vector3d<FT> sA; //secondaryAxis
	Vector3d<FT> tA; //ternaryAxis

	/** establish the main, secondary and ternary axes from the given one axis:
		 the main axis becomes the skeleton, the secondary axis is derived from the skeleton,
		 and the ternary is a vector product of the two; the secondary is the main axis in
		 which min-valued element is replaced with the value of the max element, mid-valued
		 element is preserved, and max-valued element is calculated to satisfy perpendicularity */
	void setFromSkeletonAxis(const Vector3d<FT>& skeleton)
	{
		//skip if zero or essentially-zero skeleton vector is provided
		if (skeleton.len2() <= 0.0001) return;

		mA = skeleton;
		mA.elemAbs(); //short-cut to avoid calling abs() over and over

		//ii is the index with mIn element of the skeleton
		short ii = 0;
		FT vi = mA.x;
		if (mA.y < vi) { ii = 1; vi = mA.y; }
		if (mA.z < vi) { ii = 2; vi = mA.z; }

		//ia is the index with mAx element of the skeleton
		short ia = 0;
		FT va = mA.x;
		if (mA.y > va) { ia = 1; va = mA.y; }
		if (mA.z > va) { ia = 2; va = mA.z; }

		//ii and ia are not supposed to point on the same element
		if (ii == ia) ii = (ii+1)%3;

		//the main axis again
		FT tmp[3];
		tmp[0] = skeleton.x;
		tmp[1] = skeleton.y;
		tmp[2] = skeleton.z;
		va = tmp[ia];

		//shuffle for the secondary axis
		tmp[ii] = va;
		tmp[ia] = 0;

		//calculate the missing element
		if (va != 0)
			tmp[ia] = -(skeleton.x*tmp[0] + skeleton.y*tmp[1] + skeleton.z*tmp[2]) / va;

		sA.x = tmp[0];
		sA.y = tmp[1];
		sA.z = tmp[2];

		mA = skeleton; //reset, see above...

		tA = crossProduct(mA,sA);

		mA.changeToUnitOrZero();
		sA.changeToUnitOrZero();
		tA.changeToUnitOrZero();
	}

	bool isOrthogonalBase(void)
	{
		if (std::abs(dotProduct(mA,sA)) > 0.0001) return false;
		if (std::abs(dotProduct(mA,tA)) > 0.0001) return false;
		if (std::abs(dotProduct(sA,tA)) > 0.0001) return false;
		return true;
	}

	bool isOrthonormalBase(void)
	{
		if (!isOrthogonalBase()) return false;
		if (mA.len2() > 0.0000001) return false;
		if (sA.len2() > 0.0000001) return false;
		if (tA.len2() > 0.0000001) return false;
		return true;
	}

	void rotateAxes(const Vector3d<FT>& oldDirection,
	                const Vector3d<FT>& newDirection)
	{
		//cosine of the rotation angle
		FT rotAng  = dotProduct(oldDirection,newDirection);
		rotAng    /= oldDirection.len() * newDirection.len();

		//don't do anything if there's nearly no difference between the old and new directions
		if (std::abs(rotAng) > 0.9999f) return;

		//get rotation angle and axis
		rotAng = std::acos(rotAng);
		Vector3d<FT> rotAxis = crossProduct(oldDirection,newDirection).changeToUnitOrZero();

		//setup the quaternion for the rotation
		//from this URL:
		// http://answers.google.com/answers/threadview/id/361441.html

		rotAng /= 2.0;
		const FT q0 = std::cos(rotAng);
		const FT q1 = std::sin(rotAng) * rotAxis.x;
		const FT q2 = std::sin(rotAng) * rotAxis.y;
		const FT q3 = std::sin(rotAng) * rotAxis.z;

		//setup the rotation matrix
		setupRotMatrixFromQuaternion(q0,q1,q2,q3);

		//rotate axes one by one
		rotateVector(mA);
		rotateVector(sA);
		rotateVector(tA);
	}

	inline
	void rotateVector(Vector3d<FT>& v)
	{
		FT x = rotMatrix[0]*v.x + rotMatrix[1]*v.y + rotMatrix[2]*v.z;
		FT y = rotMatrix[3]*v.x + rotMatrix[4]*v.y + rotMatrix[5]*v.z;
		v.z  = rotMatrix[6]*v.x + rotMatrix[7]*v.y + rotMatrix[8]*v.z;
		v.x = x;
		v.y = y;
	}

	FT rotMatrix[9];
	//
	void setupRotMatrixFromQuaternion(const FT q0, const FT q1,
	                                  const FT q2, const FT q3)
	{
		//from this URL:
		// http://answers.google.com/answers/threadview/id/361441.html

		//        row col
		rotMatrix[0*3 +0] = q0*q0 + q1*q1 - q2*q2 - q3*q3;
		rotMatrix[0*3 +1] = 2 * (q1*q2 - q0*q3);
		rotMatrix[0*3 +2] = 2 * (q1*q3 + q0*q2);

		//this is the 2nd row of the matrix...
		rotMatrix[1*3 +0] = 2 * (q2*q1 + q0*q3);
		rotMatrix[1*3 +1] = q0*q0 - q1*q1 + q2*q2 - q3*q3;
		rotMatrix[1*3 +2] = 2 * (q2*q3 - q0*q1);

		rotMatrix[2*3 +0] = 2 * (q3*q1 - q0*q2);
		rotMatrix[2*3 +1] = 2 * (q3*q2 + q0*q1);
		rotMatrix[2*3 +2] = q0*q0 - q1*q1 - q2*q2 + q3*q3;
	}
};
