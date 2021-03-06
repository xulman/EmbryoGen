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
		Spheres createAppropriateTargetGeom() const
		{ return Spheres(optimalTargetSpheresNo); }
		//NB, in action: copy elision or move constructor from Spheres

		/** considering the current expansion plan and the associated source geometry,
		    it reports how many spheres must the target geometry be consisting of */
		int getOptimalTargetSpheresNo() const
		{ return optimalTargetSpheresNo; }

		/** rebuilds the associated target geometry (Spheres) from the source geometry
		    according to the expansion plan using linear interpolation */
		void expandSrcIntoThis(Spheres& targetGeom) const
		{
			expandSrcIntoThis(targetGeom, [](Vector3d<FT>&,FT){}, [](FT r,FT){ return r; });
		}

		typedef const std::function< void(Vector3d<FT>&,FT) >   posShakerType;
		typedef const std::function< FT(FT,FT) >                radiusShakerType;
		typedef posShakerType*      posShakerPtr;
		typedef radiusShakerType*   radiusShakerPtr;

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
		                       const std::function< FT(FT,FT) >& radiusShaker) const
		{
#ifdef DEBUG
			//test appropriate size of the target geom
			if (targetGeom.noOfSpheres != optimalTargetSpheresNo)
				throw ERROR_REPORT("Target geom is made of " << targetGeom.noOfSpheres
				  << " spheres but " << optimalTargetSpheresNo << " is expected.");
#endif
			//create a single purpose receipt of constant content
			std::list<posShakerPtr> positionShakers;
			std::list<radiusShakerPtr> radiusShakers;
			for (size_t i = 0; i < expansionPlan.size(); ++i)
			{
				positionShakers.push_back(&positionShaker);
				radiusShakers.push_back(&radiusShaker);
			}

			expandSrcIntoThis(targetGeom, positionShakers,radiusShakers);
		}

		void expandSrcIntoThis(Spheres& targetGeom,
		                       const std::list<posShakerPtr>& positionShakers,
		                       const std::list<radiusShakerPtr>& radiusShakers) const
		{
#ifdef DEBUG
			if (positionShakers.size() != radiusShakers.size())
				throw ERROR_REPORT("position shakers length (" << positionShakers.size()
				        << " differs from radii shakers length (" << radiusShakers.size() << ")");
			if (positionShakers.size() != expansionPlan.size())
				throw ERROR_REPORT("shakers length (" << positionShakers.size()
				        << " differs from the expected length (" << expansionPlan.size() << ")");

			//test appropriate size of the target geom
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
			typename std::list<posShakerPtr>::const_iterator positionShakers_iter = positionShakers.begin();
			typename std::list<radiusShakerPtr>::const_iterator radiusShakers_iter = radiusShakers.begin();
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
					(**positionShakers_iter)(newCentre, fraction);

					newRadius = deltaRadius;
					newRadius *= fraction;
					newRadius += sourceGeom.radii[plan.fromSrcIdx];
					newRadius = (**radiusShakers_iter)(newRadius, fraction);

					targetGeom.centres[nextTargetIdx] = newCentre;
					targetGeom.radii[  nextTargetIdx] = newRadius;
					++nextTargetIdx;
				}

				++positionShakers_iter;
				++radiusShakers_iter;
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

		void printPlan() const
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
	class LinkedSpheres: protected Interpolator<FT>
	{
	public:
		// ------------------- task settings: spatial layout -------------------
		LinkedSpheres(Spheres& referenceGeom, const Vector3d<FT>& basalSideDir)
		  : Interpolator<FT>(referenceGeom), basalSideDir(1,0,0)
		{
			if (referenceGeom.noOfSpheres < 2)
				throw ERROR_REPORT("reference geometry must include at least two spheres");

			resetFromAssociatedGeom();
			setBasalSideDir(basalSideDir);
		}

		/** reinspects the associated Sphere geometry and re-reads the main axis from it */
		void resetFromAssociatedGeom()
		{
			this->mainDir  = this->sourceGeom.getCentres()[1];
			this->mainDir -= this->sourceGeom.getCentres()[0];
			updateLocalDirs();
		}

		/** (re)set the direction towards agent's basal side */
		void setBasalSideDir(const Vector3d<FT>& basalSideDir)
		{
			this->basalSideDir = basalSideDir;
			updateLocalDirs();
		}

		const Vector3d<FT>& getMainAxis()          const { return mainDir; }
		const Vector3d<FT>& getBasalSideAxis()     const { return basalSideDir; }
		const Vector3d<FT>& getAux3rdAxis()        const { return aux3rdDir; }
		const Vector3d<FT>& getRectifiedBasalDir() const { return rectifiedBasalDir; }

	protected:
		Vector3d<FT> mainDir, basalSideDir;            //inputs, given
		Vector3d<FT> rectifiedBasalDir, aux3rdDir;     //aux, computed

		void updateLocalDirs()
		{
			//3rd axis, perpendicular to the main and to the basal axes
			aux3rdDir = crossProduct(basalSideDir,mainDir);
			aux3rdDir.changeToUnitOrZero();

			//axis in the plane given by the main axis and the basalSideDir
			//and that is perpendicular to the main axis
			rectifiedBasalDir = crossProduct(mainDir,aux3rdDir);
			rectifiedBasalDir.changeToUnitOrZero();
		}

	public:
		void setupUnitAzimuthDir(const FT azimuth, Vector3d<FT>& extrusionDir) const
		{
			extrusionDir  = std::cos(azimuth) * rectifiedBasalDir;
			extrusionDir += std::sin(azimuth) * aux3rdDir;
		}
		Vector3d<FT> setupUnitAzimuthDir(const FT azimuth) const
		{
			Vector3d<FT> tmp;
			setupUnitAzimuthDir(azimuth,tmp);
			return tmp;
		}

		// ------------------- task settings: lines layout -------------------
		using typename Interpolator<FT>::posShakerType;
		using typename Interpolator<FT>::radiusShakerType;
		using typename Interpolator<FT>::posShakerPtr;
		using typename Interpolator<FT>::radiusShakerPtr;

		static void defaultPosNoAdjustment(Vector3d<FT>&,FT) {}
		static FT defaultRadiusNoChg(FT r,FT) { return r; }
		int defaultNoOfSpheresOnConnectionLines = 1;

		class AzimuthDrivenPositionExtruder {
		public:
			AzimuthDrivenPositionExtruder(const FT azimuth, const LinkedSpheres<FT>& context,
			                              const std::function< FT(FT) >& extrusionProfile_)
			  : extender(context.setupUnitAzimuthDir(azimuth)), extrusionProfile(extrusionProfile_) {}

			AzimuthDrivenPositionExtruder(const Vector3d<FT>& azimuthDir_,
			                              const std::function< FT(FT) >& extrusionProfile_)
			  : extender(azimuthDir_), extrusionProfile(extrusionProfile_) {}

			const Vector3d<FT> extender;
			const std::function< FT(FT) > extrusionProfile;

			void operator()(Vector3d<FT>& position,FT frac) const
			{
				//DEBUG_REPORT("extruder for vector " << extender << " at frac=" << frac);
				position += extrusionProfile(frac) * extender;
			}
			//NB: the operator's type must be compatible with posShakerType
		};

	protected:
		const posShakerType defaultPosNoAdjustmentRef = defaultPosNoAdjustment;
		const radiusShakerType defaultRadiusNoChgRef = defaultRadiusNoChg;

		//list of lines/azimuths, NULL ptr amounts to default*Ptr //TODO
		std::map<float,posShakerType>    azimuthToPosShaker;
		std::map<float,radiusShakerType> azimuthToRadiusShaker;
		std::map<float,int>              azimuthToNoOfSpheres;

	public:
		/** resets the line ups: starting from 'minAzimuth' in steps of 'stepAzimuth' up to (incluside) 'maxAzimuth',
		    series of spheres are created and placed (lined up) at each azimuth and along an extrusion profile. The
		    latter is a curve that dictates how far from the main dir (connection line between 1st and 2nd sphere
		    in the associated geometry) given sphere should be placed. In this method, a sin() curve is used. */
		void resetAllAzimuthsToExtrusions(const float minAzimuth, const float stepAzimuth, const float maxAzimuth)
		{
			const FT mag = static_cast<FT>(0.5) * mainDir.len();
			resetAllAzimuthsToExtrusions(minAzimuth,stepAzimuth,maxAzimuth, [mag](FT f){ return mag*std::sin(f*M_PI); });
		}

		/** resets the same as the other resetAllAzimuthsToExtrusions() but caller needs to provide
		    his/her own function for domain [0,1] */
		void resetAllAzimuthsToExtrusions(const float minAzimuth, const float stepAzimuth, const float maxAzimuth,
		                                  const std::function< FT(FT) >& extrusionProfile)
		{
			azimuthToPosShaker.clear();
			azimuthToRadiusShaker.clear();
			azimuthToNoOfSpheres.clear();

			Vector3d<FT> azimuthDir;
			for (float a = minAzimuth; a <= maxAzimuth; a += stepAzimuth)
			{
				setupUnitAzimuthDir(a,azimuthDir);

				azimuthToPosShaker.insert(std::pair<float,posShakerType>(
					a,
					AzimuthDrivenPositionExtruder(azimuthDir,extrusionProfile) ));
				//
				azimuthToRadiusShaker.insert(std::pair<float,radiusShakerType>(a,defaultRadiusNoChg));
				azimuthToNoOfSpheres[a] = defaultNoOfSpheresOnConnectionLines;
			}
		}

		void resetNoOfSpheresInAllAzimuths(const int noOfSpheres)
		{
			for (auto& m : azimuthToNoOfSpheres) m.second = noOfSpheres;
		}

		void addOrChangeAzimuthToExtrusion(const float azimuth)
		{
			//insert cannot overwrite, we have to clean beforehand
			removeAzimuth(azimuth);

			Vector3d<FT> azimuthDir;
			setupUnitAzimuthDir(azimuth,azimuthDir);

			const FT mag = static_cast<FT>(0.5) * mainDir.len();
			azimuthToPosShaker.insert(std::pair<float,posShakerType>(
				azimuth,
				AzimuthDrivenPositionExtruder(azimuthDir,[mag](FT f){ return mag*std::sin(f*M_PI); }) ));
			//
			azimuthToRadiusShaker.insert(std::pair<float,radiusShakerType>(azimuth,defaultRadiusNoChg));
			azimuthToNoOfSpheres[azimuth] = defaultNoOfSpheresOnConnectionLines;
		}

		void addOrChangeAzimuth(const float azimuth,
		               posShakerType& posShaker,
		               radiusShakerType& radiusShaker,
		               int noOfSpheres)
		{
			//insert cannot overwrite, we have to clean beforehand
			removeAzimuth(azimuth);

			azimuthToPosShaker.insert(std::pair<float,posShakerType>(azimuth,posShaker));
			azimuthToRadiusShaker.insert(std::pair<float,radiusShakerType>(azimuth,radiusShaker));
			azimuthToNoOfSpheres[azimuth] = noOfSpheres;
		}

		void removeAzimuth(const float azimuth)
		{
			azimuthToPosShaker.erase(azimuth);
			azimuthToRadiusShaker.erase(azimuth);
			azimuthToNoOfSpheres.erase(azimuth);
		}

		// ------------------- task implementation: create the layout -------------------
		int getNoOfNecessarySpheres() const
		{
			int cnt = this->sourceGeom.getNoOfSpheres();
			for (auto& m : azimuthToNoOfSpheres) cnt += m.second;
			return cnt;
		}

		/** accessor of the inherited (but protected) method */
		void printPlan() const { Interpolator<FT>::printPlan(); }

		int printSkeleton(DisplayUnit& du, const int startWithThisID, const int color,
		                  const Spheres& generatedGeom, const int skipSpheres = 2) const
		{
			int ID = startWithThisID-1;
			int sNo = skipSpheres-1;
			const Vector3d<G_FLOAT>* centres = generatedGeom.getCentres();

			for (const auto& m : azimuthToNoOfSpheres)
			if (m.second > 0)
			{
				du.DrawLine(++ID, centres[0], centres[++sNo], color);
				for (int i = 1; i < m.second; ++i, ++sNo)
					du.DrawLine(++ID, centres[sNo], centres[sNo+1], color);
				du.DrawLine(++ID, centres[sNo], centres[1], color);
			}

			return ID;
		}


		/** builds ino the given geometry according to the pre-defined line ups (azimuths) */
		void buildInto(Spheres& newGeom)
		{
#ifdef DEBUG
			if (newGeom.noOfSpheres != getNoOfNecessarySpheres())
				throw ERROR_REPORT("Given geometry cannot host the one defined here.");
#endif
			//iterate over all azimuths, set up and apply the up-stream Interpolator
			this->expansionPlan.clear();
			this->optimalTargetSpheresNo = this->sourceGeom.getNoOfSpheres();
			positionShakers.clear();
			radiusShakers.clear();

			//prepare direction vectors
			for (const auto& map : azimuthToNoOfSpheres)
			{
				this->addToPlan(0,1, map.second);

				posShakerPtr pSP = &azimuthToPosShaker[map.first];
				DEBUG_REPORT(map.first << ": posShaker @ " << pSP);
				if (pSP != NULL) positionShakers.push_back(pSP);
				else             positionShakers.push_back(&defaultPosNoAdjustmentRef);

				radiusShakerPtr rSP = &azimuthToRadiusShaker[map.first];
				DEBUG_REPORT(map.first << ": radiusShaker @ " << rSP);
				if (rSP != NULL) radiusShakers.push_back(rSP);
				else             radiusShakers.push_back(&defaultRadiusNoChgRef);
			}
			this->expandSrcIntoThis(newGeom, positionShakers,radiusShakers);
		}

		void rebuildInto(Spheres& newGeom)
		{
#ifdef DEBUG
			if (newGeom.noOfSpheres != getNoOfNecessarySpheres())
				throw ERROR_REPORT("Given geometry cannot host the one defined here.");
#endif
			this->expandSrcIntoThis(newGeom, positionShakers,radiusShakers);
		}

	protected:
		std::list<posShakerPtr> positionShakers;
		std::list<radiusShakerPtr> radiusShakers;


		// ------------------- task implementation: maintain the layout -------------------
	public:
		/** given the current state in 'geom' and some new state from the associated
		    reference geom (the one provided at this object's construction), changes
		    in radii of the first two spheres and in their mutual distance are detected,
		    and distributed along the line ups */
		void refreshThis(Spheres& geom)
		{
#ifdef DEBUG
			if (geom.noOfSpheres != getNoOfNecessarySpheres())
				throw ERROR_REPORT("Given geometry is not compatible with the one defined here.");
#endif
			const float deltaRadius0 = this->sourceGeom.getRadii()[0] - geom.getRadii()[0];
			const float deltaRadius1 = this->sourceGeom.getRadii()[1] - geom.getRadii()[1];

			Vector3d<FT> posDeltaDir(geom.getCentres()[1]);
			posDeltaDir -= geom.getCentres()[0];

			float deltaMainAxisDist
				= (this->sourceGeom.getCentres()[1] - this->sourceGeom.getCentres()[0]).len()
				- posDeltaDir.len();
			if (std::abs(deltaMainAxisDist) < 0.001f) deltaMainAxisDist = 0; //stabilizes the mainDir axis
			posDeltaDir.changeToUnitOrZero();

			/*
			DEBUG_REPORT("deltaRad0=" << deltaRadius0 << ", deltaRad1=" << deltaRadius1
				<< ", deltaDist=" << deltaMainAxisDist);
			*/

			geom.updateRadius(0, this->sourceGeom.getRadii()[0]);
			geom.updateRadius(1, this->sourceGeom.getRadii()[1]);
			geom.updateCentre(0, geom.getCentres()[0] - 0.5f*deltaMainAxisDist*posDeltaDir);
			geom.updateCentre(1, geom.getCentres()[1] + 0.5f*deltaMainAxisDist*posDeltaDir);

			//iterate the lineups and update, based on detected changes, the radii
			//and positions along the main axis
			int sNo = this->sourceGeom.getNoOfSpheres();
			for (const auto& m : azimuthToNoOfSpheres)
			if (m.second > 0)
			{
				const float intersections = static_cast<float>(m.second+1);
				for (int i = 1; i <= m.second; ++i, ++sNo)
				{
					float frac = (float)i/intersections;
					geom.updateRadius(sNo, geom.getRadii()[sNo]
					                       + (1-frac)*deltaRadius0
					                       + frac*deltaRadius1);
					geom.updateCentre(sNo, geom.getCentres()[sNo]
					                       + deltaMainAxisDist*(frac-0.5f)*posDeltaDir);
				}
			}
		}
	};
};
#endif
