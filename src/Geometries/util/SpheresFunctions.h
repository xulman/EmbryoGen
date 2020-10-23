#ifndef GEOMETRY_UTIL_SPHERESFUNCTIONS_H
#define GEOMETRY_UTIL_SPHERESFUNCTIONS_H

#include <cmath>
#include <functional>
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
#ifdef DEBUG
			//some sanity checks (and warnings)
			if (prevRadius <= 0)
				REPORT("WARNING: updating coords from negative previous radius (" << prevRadius << ")");
			if (newRadius <= 0)
				REPORT("WARNING: updating coords into negative (current) radius (" << newRadius << ")");
			if (prevOrientation.len2() == 0)
				REPORT("WARNING: updating coords and there is no previous orientation, the vec is (0,0,0)");
			if (newOrientation.len2() == 0)
				REPORT("WARNING: updating coords and there is no current orientation, the vec is (0,0,0)");
#endif
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
			if (std::abs(rotAng) > 0.999999999)
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
				rotAng /= (FT)2.0;
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

	template <typename FT>
	struct SquareMatrix {
		explicit
		SquareMatrix(int noOfSpheres) : side(noOfSpheres), data(new FT[side*side]) {}
		~SquareMatrix() { delete[] data; }

		const int side;
		FT* const data;

		FT* operator()(int row,int col) const { return data + (row*side +col); }
		FT set(int row,int col, FT val) const { data[row*side +col] = val; return val; }
		FT get(int row,int col) const { return data[row*side +col]; }

		void print()
		{
			for (int row=0; row < side; ++row)
			{
				for (int col=0; col < side; ++col)
					REPORT_NOHEADER_NOENDL("\t" << get(row,col));
				REPORT_NOHEADER_JUSTENDL();
			}
		}
	};


	template <typename FT>
	class Interpolator
	{
	public:
		/** bind this Interpolator to the input (source) fixed geometry of Spheres, this source one will
		    be expanded into some given target one according to the expansion plan; the source geom
		    cannot be changed later to prevent form potential inconsistencies with the expansion plan */
		Interpolator(const Spheres& _sourceGeom)
		  : optimalTargetSpheresNo(_sourceGeom.getNoOfSpheres()),
		    sourceGeom(_sourceGeom)
		{}

		/** considering the current expansion plan and the associated source geometry,
		    it creates and shares an appropriately sized geometry */
		Spheres createAppropriateTargetGeom()
		{ return Spheres(optimalTargetSpheresNo); }
		//NB, in action: copy elision or move constructor from Spheres

		/** considering the current expansion plan and the associated source geometry,
		    it reports how many spheres must the target geometry be consisting of */
		int getOptimalTargetSpheresNo()
		{ return optimalTargetSpheresNo; }

		/** rebuilds the associated target geometry (Spheres) from the source geometry
		    according to the expansion plan using linear interpolation */
		void expandSrcIntoThis(Spheres& targetGeom)
		{
			expandSrcIntoThis(targetGeom, [](Vector3d<FT>&,FT){}, [](FT r,FT){ return r; });
		}

		/** Rebuilds the associated target geometry (Spheres) from the source geometry
		    according to the expansion plan using the provided shakers. The purpose of
		    a shaker is to bias (or randomize) its first argument. The argument is always
		    computed with linear interpolation, and the shaker is expected only to figure
		    out and apply some "delta" to it. The bias (or randomization) may be a function
		    itself of the second shaker's argument, which is between 0 and 1 and is suggesting
		    how far is the originally linearly interpolated first argument in the job.
		    The position shaker adjusts (or don't) the sphere's centre. The radius shaker
		    reads in sphere's radius and returns an adjusted (or the same) value. */
		void expandSrcIntoThis(Spheres& targetGeom,
		                       const std::function< void(Vector3d<FT>&,FT) >& positionShaker,
		                       const std::function< FT(FT,FT) >& radiusShaker)
		{
			//test appropriate size of the target geom
#ifdef DEBUG
			if (targetGeom.noOfSpheres != optimalTargetSpheresNo)
				throw ERROR_REPORT("Target geom is made of " << targetGeom.noOfSpheres
				  << " spheres but " << optimalTargetSpheresNo << " is expected.");
#endif

			//copy the source as is
			for (int i = 0; i < sourceGeom.noOfSpheres; ++i)
			{
				targetGeom.centres[i] = sourceGeom.centres[i];
				targetGeom.radii[i] = sourceGeom.radii[i];
			}
			int nextTargetIdx = sourceGeom.noOfSpheres;

			//add interpolated spheres according to the plan
			Vector3d<FT> distVec, newCentre;
			FT deltaRadius,newRadius;
			for (const auto& plan : expansionPlan)
			{
				distVec  = sourceGeom.centres[plan.toSrcIdx];
				distVec -= sourceGeom.centres[plan.fromSrcIdx];
				deltaRadius = sourceGeom.radii[plan.toSrcIdx] - sourceGeom.radii[plan.fromSrcIdx];

				for (int i = 1; i <= plan.noOfSpheresInBetween; ++i)
				{
					const FT fraction = static_cast<FT>(i) / static_cast<FT>(plan.noOfSpheresInBetween + 1);
					newCentre = distVec;
					newCentre *= fraction;
					newCentre += sourceGeom.centres[plan.fromSrcIdx];
					positionShaker(newCentre, fraction);

					newRadius = deltaRadius;
					newRadius *= fraction;
					newRadius += sourceGeom.radii[plan.fromSrcIdx];
					newRadius = radiusShaker(newRadius, fraction);

					targetGeom.centres[nextTargetIdx] = newCentre;
					targetGeom.radii[  nextTargetIdx] = newRadius;
					++nextTargetIdx;
				}
			}
		}

		void addToPlan(const int fromSrcIdx, const int toSrcIdx, const float relativeRadiusOverlap = 0.8f)
		{
			//computes noOfSpheresInBetween and calls the one above
			//TODO
		}

		/** adds another plan item to the expansion plan, even if similar plan item already exists (no checks are done) */
		void addToPlan(const int fromSrcIdx, const int toSrcIdx, const int noOfSpheresInBetween)
		{
#ifdef DEBUG
			if (fromSrcIdx < 0 || fromSrcIdx >= sourceGeom.noOfSpheres)
				throw ERROR_REPORT("src index is invalid, should be within [0," << sourceGeom.noOfSpheres-1 << "]");
			if (toSrcIdx < 0 || toSrcIdx >= sourceGeom.noOfSpheres)
				throw ERROR_REPORT("to index is invalid, should be within [0," << sourceGeom.noOfSpheres-1 << "]");
			if (noOfSpheresInBetween <= 0)
				throw ERROR_REPORT("illegal number of requested spheres: " << noOfSpheresInBetween);
#endif
			expansionPlan.emplace_back(fromSrcIdx, toSrcIdx, noOfSpheresInBetween);
			optimalTargetSpheresNo += noOfSpheresInBetween;
		}

		/** removes this item from the overall plan,  */
		void removeFromPlan(const int fromSrcIdx, const int toSrcIdx, const bool removeAllMatching = false)
		{
			bool keepFinding = true;

			typename std::list<planItem_t>::iterator i = expansionPlan.begin();
			while (i != expansionPlan.end() && keepFinding)
			{
				if (i->fromSrcIdx == fromSrcIdx && i->toSrcIdx == toSrcIdx)
				{
					//found a match:
					optimalTargetSpheresNo -= i->noOfSpheresInBetween;
					i = expansionPlan.erase(i); //NB: sets 'i' to elem right after the erased one
					keepFinding = removeAllMatching;
				}
				else ++i;
			}
		}

		void printPlan()
		{
			REPORT("Exact copy of the src geometry (" << sourceGeom.noOfSpheres << ")");
			for (const auto& plan : expansionPlan)
				REPORT("Between " << plan.fromSrcIdx << " and " << plan.toSrcIdx
				  << " (" << plan.noOfSpheresInBetween << ")");
			REPORT("Total no. of spheres: " << optimalTargetSpheresNo);
		}

	protected:
		struct planItem_t
		{
			planItem_t(int f,int t,int n): fromSrcIdx(f), toSrcIdx(t), noOfSpheresInBetween(n) {};
			const int fromSrcIdx, toSrcIdx; //primary key
			const int noOfSpheresInBetween; //value
		};

		/** essentially a multimap (permitting multiple values for the same key)
		    that preserves the order in which items were added */
		std::list<planItem_t> expansionPlan;

		int optimalTargetSpheresNo;

		const Spheres& sourceGeom;
	};


	/**
	 * Builds, maintains and updates a spheres-based geometry of an agent that recognizes
	 * its own polarity and direction towards its basal side. The polarity is given by two
	 * (main) spheres, which define the gross basis of the overall shape of the agent. The
	 * net shape is established by adding "links of spheres" between the two spheres. Each
	 * link is defined by its "azimuth": consider a plane whose normal coincides with the
	 * polarity axis and in which lies the vector towards the basal side of the agent -- this
	 * vector represents the azimuth at 0 deg. Net shape is obtained by utilizing multiple
	 * such azimuths, each defines a direction of extrusion that is applied on given number
	 * of new spheres that would otherwise be placed on a straight line between the two polarity-
	 * defining spheres. Finally, the net shape "follows" (is updated with) the changes in
	 * position and radius of the two spheres. */
	template <typename FT>
	class LinkedSpheres
	{
	public:
		//dam tomu: dve pozice a polomery (aka main axis), basal axis - to je nutnost

		//optional, pres settery:
		//minAzimuth, maxAzimuth, stepAzimuth - skonci v citelne mape azimutu
		//pocty na retizku
		//muzu nastavit default shaker
		//muzu nastavit kazdemu azimutu jeho shaker, jinak se pouzije ten default

		//gettery:
		//celkovy pocet spheres
		//rebuild zadanou geometrii from scratch
		//update zadanou geometrii na zaklade detekovanych zmen v referencni geometrii

		// ------------------- task settings: spatial layout -------------------
		LinkedSpheres(Spheres& referenceGeom, const Vector3d<FT>& basalSideDir)
		{
			if (referenceGeom.noOfSpheres < 2)
				throw ERROR_REPORT("reference geometry must be at least of two spheres");

			setCentres(referenceGeom.centres[0],referenceGeom.centres[1]);
			setRadii(referenceGeom.radii[0],referenceGeom.radii[1]);
			setBasalSideDir(basalSideDir);
		}

		void setCentres(const Vector3d<FT>& fromCentre, const Vector3d<FT>& toCentre)
		{
			this->fromCentre = fromCentre;
			this->toCentre   = toCentre;
		}

		void setRadii(const FT fromRadius, const FT toRadius)
		{
			this->fromRadius = fromRadius;
			this->toRadius   = toRadius;
		}

		void setBasalSideDir(const Vector3d<FT>& basalSideDir)
		{
			this->basalSideDir = basalSideDir;
		}

		Vector3d<FT> fromCentre, toCentre;
		FT fromRadius, toRadius;
		Vector3d<FT> basalSideDir;

		// ------------------- task settings: lines layout -------------------
		typedef const std::function< void(Vector3d<FT>&,FT) >* posShakerPtr;
		typedef const std::function< FT(FT,FT) >* radiusShakerPtr;

		static const posShakerPtr defaultPosNoAdjustment = [](Vector3d<FT>&,FT){};
		static const radiusShakerPtr defaultRadiusNoChg  = [](FT r,FT){ return r; };
		int defaultNoOfSpheresOnConnectionLines = 1;

	protected:
		//list of lines, NULL ptr amounts to default*Ptr
		std::map<float, posShakerPtr    > azimuthToPosShaker;
		std::map<float, radiusShakerPtr > azimuthToRadiusShaker;
		std::map<float, int >             azimuthToNoOfSpheres;

	public:
		void resetAllAzimuthsWithDefaults(const float minAzimuth, const float stepAzimuth, const float maxAzimuth)
		{
			azimuthToPosShaker.clear();
			azimuthToRadiusShaker.clear();
			azimuthToNoOfSpheres.clear();

			for (float a = minAzimuth; a <= maxAzimuth; a += stepAzimuth)
			{
				azimuthToPosShaker[a] = defaultPosNoAdjustment;
				azimuthToRadiusShaker[a] = defaultRadiusNoChg;
				azimuthToNoOfSpheres[a] = defaultNoOfSpheresOnConnectionLines;
			}
		}

		void resetNoOfSpheresInAllAzimuths(const int noOfSpheres)
		{
			for (auto& m : azimuthToNoOfSpheres) m.second = noOfSpheres;
		}

		void addOrChangeAzimuthToDefaults(const float azimuth)
		{
			azimuthToPosShaker[azimuth] = defaultPosNoAdjustment;
			azimuthToRadiusShaker[azimuth] = defaultRadiusNoChg;
			azimuthToNoOfSpheres[azimuth] = defaultNoOfSpheresOnConnectionLines;
		}

		void addOrChangeAzimuth(const float azimuth,
		               posShakerPtr posShaker,
		               radiusShakerPtr radiusShaker,
		               int noOfSpheres)
		{
			azimuthToPosShaker[azimuth] = posShaker;
			azimuthToRadiusShaker[azimuth] = radiusShaker;
			azimuthToNoOfSpheres[azimuth] = noOfSpheres;
		}

		void removeAzimuth(const float azimuth)
		{
			azimuthToPosShaker.erase(azimuth);
			azimuthToRadiusShaker.erase(azimuth);
			azimuthToNoOfSpheres.erase(azimuth);
		}

		int getNoOfNecessarySpheres()
		{
			int cnt = 0;
			for (auto& m : azimuthToNoOfSpheres) cnt += m.second;
			return cnt;
		}











		//----------------------------


		// ------------------- task settings/inputs -------------------
		Vector3d<FT> fromPos, tillPos;
		Vector3d<FT> extrusionDir;

		// ------------------- temporaries/outputs -------------------
		Vector3d<FT> extrusionDirRectified;

		// ------------------- main routine -------------------
		void populate(std::vector< Vector3d<FT>* >& newLineUp)
		{
			populate(newLineUp, [](FT frac){ return std::sin(frac *(FT)3.14159) * (FT)6; } );
		}

		void populate(std::vector< Vector3d<FT>* >& newLineUp,
		              const std::function<FT(FT)>& extrusionDist)
		{
			Vector3d<FT> distVec, newCentre;

			//main axis to sample along plus extrusionDir
			distVec  = tillPos;
			distVec -= fromPos;

			//aux axis to extrude along that is quasi-parallel with the extrusionDir
			//
			//that is in the plane given by the main axis and the extrusionDir,
			//and that is perpendicular to the main axis
			extrusionDirRectified = crossProduct(extrusionDir,distVec);
			extrusionDirRectified = crossProduct(distVec,extrusionDirRectified);
			extrusionDirRectified.changeToUnitOrZero();

			for (size_t i = 0; (size_t)i < newLineUp.size(); ++i)
			{
				const FT frac = (FT)(i+1) / (FT)(newLineUp.size()+1);

				//position along the direct line
				newCentre = distVec;
				newCentre *= frac;
				newCentre += fromPos;

				//extrusion offset
				newCentre += extrusionDist(frac) * extrusionDirRectified;

				*(newLineUp[i]) = newCentre;
			}
		}
	};
};
#endif
