#ifndef TRACKRECORD_H
#define TRACKRECORD_H

#include <map>
#include <set>
#include "util/Vector3d.h"
#include "util/FlowField.h"
#include "DisplayUnits/DisplayUnit.h"

/**
 * Reads, represents and supports with trajectories to, e.g., provide
 * with sample trajectories that a simulation system would like to follow.
 *
 * It holds the entire set of trajectories. That is, for every timepoint and
 * for every track ID, a coordinate is available, e.g., trajectories[time][trackID].x
 *
 * Author: Vladimir Ulman, 2018
 */
class TrackRecords: public std::map< float,std::map< int,Coord3d<float> > >
{
public:
	/** A set of IDs of tracks that are recorded in this object.
	    The synchronization between *this and this variable is, however,
	    not enforced. */
	std::set<int> knownTracks;

	/** Adds to the current trajectories whatever it finds in the 'filename'.
	    The file should be a text file with white-space separated items,
	    in this order, TIME X Y Z TRACK_ID. The X Y Z coordinates are element-wise
	    multiplied with the 'scale' (micron coords remain in microns).

	    The function also updates this->knownTracks . */
	void readFromFile(const char* filename, const Vector3d<float>& spaceScale,
	                  const float timeScale = 1.0f, const float timeShift = 0.0f);


	/** Writes all trajectories back into a text 'filename' as white-space
	    separated times, in this order, TIME X Y Z TRACK_ID. */
	void writeToFile(const char* filename) const;


	/** Reports displacement vector for a given 'track' between the two time points,
	    or use 'defaultVec' if no such information is available in the records. */
	Vector3d<float> getDisplacement(const float timeFrom, const float timeTo,
	                                const int track,
	                                const Vector3d<float>& defaultVec) const;


	/** Interpolates (if necessary) coordinate along the 'track' at 'time'.
	    Returns true (and valid 'retCoord') if that was possible, otherwise
		 (if 'time' is outside the track's time span, for instance) returns false. */
	bool getPositionAlongTrack(const float time, const int track,
	                           Coord3d<float>& retCoord) const;


	/** Scans over all tracks and inserts (overwrites) displacement vectors
	    between the given time span into the flow field 'FF'. The FF must be
	    initialized beforehand. Every vector of the FF is "dilated" before
	    the insertion by a box structuring element whose half-sizes ("radii")
	    are given in the 'propagationRadius' (in microns). */
	void insertToFF(const float timeFrom, const float timeTo,
	                FlowField<float>& FF,
	                const Vector3d<float>& propagationRadius,
	                const bool smoothExpansion=true) const
	{
		injectToFF(timeFrom,timeTo, FF, propagationRadius,smoothExpansion, false);
	}

	/** Scans over all tracks and inserts displacement vectors between the
	    given time span into the flow field 'FF'. The FF must be initialized
	    beforehand; it is, however, zeroed in this function prior its work.
	    If more displacement vectors coincide on the same voxel, their average
	    value/vector is stored in the end. Every vector of the FF is "dilated"
	    before the insertion by a box structuring element whose half-sizes
	    ("radii") are given in the 'propagationRadius' (in microns). */
	void resetToFF(const float timeFrom, const float timeTo,
	               FlowField<float>& FF,
	               const Vector3d<float>& propagationRadius,
	               const bool smoothExpansion=true) const
	{
		injectToFF(timeFrom,timeTo, FF, propagationRadius,smoothExpansion, true);
	}

private:
	void injectToFF(const float timeFrom, const float timeTo,
	                FlowField<float>& FF,
	                const Vector3d<float>& propagationRadius,
	                const bool smoothExpansion,
	                const bool doReset) const;


public:
	/** Scans over all tracks and displays displacement vectors between
	    the given time span. The vectors are drawn using the display
	    unit, with the given color and with increasing IDs starting from
	    the one given. The number of drawn/created vectors is returned. */
	int drawFF(const float timeFrom, const float timeTo,
	           DisplayUnit& du, const int ID, const int color) const;
};
#endif
