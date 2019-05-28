#ifndef GEOMETRY_UTIL_SPHERESFUNCTIONS_H
#define GEOMETRY_UTIL_SPHERESFUNCTIONS_H

#include "../Spheres.h"

class SpheresFunctions
{
public:
	/** grow 4 sphere geometry 's' in radius by 'dR', and in diameter by 'dD' */
	template <typename FT>
	static
	void grow4SpheresBy(Spheres& s, const FT dR, const FT dD)
	{
		if (s.noOfSpheres != 4)
			throw ERROR_REPORT("Cannot grow non-four sphere geometry.");

		//make the nuclei fatter by 'dD' um in diameter
		for (int i=0; i < 4; ++i) s.radii[i] += dR;

		//offset the centres as well
		Vector3d<FT> dispL1,dispL2;

		dispL1  = s.centres[2];
		dispL1 -= s.centres[1];
		dispL2  = s.centres[3];
		dispL2 -= s.centres[2];
		dispL1 *= dR / dispL1.len();
		dispL2 *= dD / dispL2.len();

		s.centres[2] += dispL1;
		s.centres[3] += dispL1;
		s.centres[3] += dispL2;

		dispL1 *= -1.0f;
		dispL2  = s.centres[0];
		dispL2 -= s.centres[1];
		dispL2 *= dD / dispL2.len();

		s.centres[1] += dispL1;
		s.centres[0] += dispL1;
		s.centres[0] += dispL2;
	}

	/** An utility class to update coordinates to maintain relative position
	    within a Spheres geometry. The class was originally designed for updating
	    positions of texture particles within a (travelling) sphere geometry.
	    The class tracks the previous geometry of the sphere to see the sphere's
	    travelling and, hence, to be able to update given coordinates.

	    One is expected to initialize an object of this class with the current state
	    of the sphere via the constructor. Whenever the sphere updates its geometry,
	    one can communicate the new state via prepareUpdating() to make this
	    object prepare a (internal) "recipe", and then update any coordinate
	    with the updateCoord() to make the coordinate follow the sphere's
	    travelling. */
	template <typename FT>
	class CoordsUpdater
	{
	public:
		/** memory of the previous centre of a sphere */
		Vector3d<FT> prevCentre;

		/** memory of the previous radius of a sphere */
		FT           prevRadius;

		/** memory of the previous orientation of a sphere, this is
		    a non-intrinsic parameter of a sphere and caller must figure
		    out for himself how to define it... */
		Vector3d<FT> prevOrientation;

		/** records the current state of a sphere */
		CoordsUpdater(const Vector3d<FT>& centre, const FT radius,
		              const Vector3d<FT>& orientation)
			: prevCentre(centre),
			  prevRadius(radius),
			  prevOrientation(orientation)
		{}

		/** records the current state of a sphere at 'index' from
		    the given Spheres geometry */
		CoordsUpdater(const Spheres& s, const int index,
		              const Vector3d<FT>& orientation)
		{
#ifdef DEBUG
			if (index < 0 || index >= s.noOfSpheres)
				throw ERROR_REPORT("Incorrect index of a sphere provided.");
#endif
			prevCentre = s.centres[index];
			prevRadius = s.radii[index];
			prevOrientation = orientation;
		}

		/** internal recipe: sphere's old centre to follow prevCentre -> newCentre */
		Vector3d<FT> coordBase;

		/** internal recipe: radial stretch follow prevRadius -> newRadius */
		FT radiusStretch;

		/** internal recipe: rotation matrix to follow prevOrientation -> newOrientation */
		FT rotMatrix[9];

		/** provide the new state of the sphere to make this object calculate
		    (essentially a sort of caching) appropriate rotMatrix and radiusStretch,
		    to allow the updateCoord() to do a proper job */
		void prepareUpdating(const Vector3d<FT>& newCentre, const FT newRadius,
		                     const Vector3d<FT>& newOrientation)
		{
			coordBase = prevCentre;
			radiusStretch = newRadius / prevRadius;

			//setup the quaternion for the rotation
			//from this URL:
			// http://answers.google.com/answers/threadview/id/361441.html

			//cosine of the rotation angle
			FT rotAng  = dotProduct(prevOrientation,newOrientation);
			rotAng    /= prevOrientation.len() * newOrientation.len();

			//setup identity (no rotation) matrix if there's (nearly)
			//no difference between the prev and new orientation
			if (std::abs(rotAng) > 0.9999f)
			{
				rotMatrix[0] = 1; rotMatrix[1] = 0; rotMatrix[2] = 0;
				rotMatrix[3] = 0; rotMatrix[4] = 1; rotMatrix[5] = 0;
				rotMatrix[6] = 0; rotMatrix[7] = 0; rotMatrix[8] = 1;
			}
			else
			{
				//get rotation angle and axis
				rotAng = std::acos(rotAng);
				Vector3d<FT> rotAxis = crossProduct(prevOrientation,newOrientation).changeToUnitOrZero();
				//NB: should not become zero as the input orientations do differ

				//quaternion params
				rotAng /= 2.0;
				const FT q0 = std::cos(rotAng);
				const FT q1 = std::sin(rotAng) * rotAxis.x;
				const FT q2 = std::sin(rotAng) * rotAxis.y;
				const FT q3 = std::sin(rotAng) * rotAxis.z;

				//rotation matrix from the quaternion
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

			//remember the new state
			prevCentre = newCentre;
			prevRadius = newRadius;
			prevOrientation = newOrientation;
		}

		/** provide the new state of the sphere to make this object calculate
		    (essentially a sort of caching) appropriate rotMatrix and radiusStretch,
		    to allow the updateCoord() to do a proper job */
		void prepareUpdating(const Spheres& s, const int index,
		                     const Vector3d<FT>& newOrientation)
		{
#ifdef DEBUG
			if (index < 0 || index >= s.noOfSpheres)
				throw ERROR_REPORT("Incorrect index of a sphere provided.");
#endif
			prepareUpdating(s.centres[index],s.radii[index],newOrientation);
		}

		/** updates the user's (texture) coordinate to follow the sphere */
		void updateCoord(Vector3d<FT>& pos) const
		{
			//shift to coord centre
			pos -= coordBase;

			//rotate...
			FT  x = rotMatrix[0]*pos.x + rotMatrix[1]*pos.y + rotMatrix[2]*pos.z;
			FT  y = rotMatrix[3]*pos.x + rotMatrix[4]*pos.y + rotMatrix[5]*pos.z;
			pos.z = rotMatrix[6]*pos.x + rotMatrix[7]*pos.y + rotMatrix[8]*pos.z;
			pos.x = x;
			pos.y = y;

			//stretch
			pos *= radiusStretch;

			//shift back to sphere's centre (which is memorised as 'prevCentre')
			pos += prevCentre;
		}

		void showPrevState(void) const
		{
			REPORT("object's addr  @ " << (long)this);
			REPORT("prevCentre     : " << prevCentre);
			REPORT("prevRadius     : " << prevRadius);
			REPORT("prevOrientation: " << prevOrientation);
		}

		void showRecipe(void) const
		{
			REPORT("object's addr @ " << (long)this);
			REPORT("centre shift  : " << coordBase << " -> " << prevCentre);
			REPORT("radius stretch: " << radiusStretch);
			REPORT("rotMatrix row1: " << rotMatrix[0] << "\t" << rotMatrix[1] << "\t" << rotMatrix[2]);
			REPORT("rotMatrix row2: " << rotMatrix[3] << "\t" << rotMatrix[4] << "\t" << rotMatrix[5]);
			REPORT("rotMatrix row3: " << rotMatrix[6] << "\t" << rotMatrix[7] << "\t" << rotMatrix[8]);
		}
	};
};
#endif
