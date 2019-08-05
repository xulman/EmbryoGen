#ifndef TRACKRECORD_H
#define TRACKRECORD_H

#include <map>
#include <set>
#include <fstream>
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
	                  const float timeScale = 1.0f, const float timeShift = 0.0f)
	{
		std::ifstream f(filename);
		if (! f.is_open())
			throw new std::runtime_error(EREPORT("Cannot read from ").append(filename));

		float time,x,y,z;
		int id,parent,ignore;
		char c;

		while (f.good())
		{
			//read the first character
			f >> c;

			//if it starts with '#' then we ignore the complete line
			if (c == '#')
			{
				f.ignore(std::numeric_limits<std::streamsize>::max(),'\n');
				continue;
			}

			//else, we return back the first character, and start reading the record
			f.putback(c);
			f >> time >> x >> y >> z >> id >> parent >> ignore;
			if (f.good())
			{
				//store the record
				auto& rr = (*this)[time*timeScale +timeShift];
				rr[id] = Coord3d<float>(x,y,z);
				rr[id].elemMult(spaceScale);

				knownTracks.insert(id);
			}
		}

		f.close();
	}


	/** Writes all trajectories back into a text 'filename' as white-space
	    separated times, in this order, TIME X Y Z TRACK_ID. */
	void writeToFile(const char* filename) const
	{
		std::ofstream f(filename);
		if (! f.is_open())
			throw new std::runtime_error(EREPORT("Cannot write to ").append(filename));

		auto t = this->begin();
		for (; t != this->end(); ++t)
		{
			for (const auto& id : t->second)
			{
				f << t->first << "\t"
				  << id.second.x << "\t"
				  << id.second.y << "\t"
				  << id.second.z << "\t"
				  << id.first << " 0 0\n";
			}
		}

		f.close();
	}


	/** Reports displacement vector for a given 'track' between the two time points,
	    or use 'defaultVec' if no such information is available in the records. */
	Vector3d<float> getDisplacement(const float timeFrom, const float timeTo,
	                                const int track,
	                                const Vector3d<float>& defaultVec) const
	{
		try
		{
			const Coord3d<float>& start = this->at(timeFrom).at(track);
			const Coord3d<float>&  end  = this->at( timeTo ).at(track);
			return (end-start);
		}
		catch (std::out_of_range)
		{
			return defaultVec;
		}
	}


	/** Interpolates (if necessary) coordinate along the 'track' at 'time'.
	    Returns true (and valid 'retCoord') if that was possible, otherwise
		 (if 'time' is outside the track's time span, for instance) returns false. */
	bool getPositionAlongTrack(const float time, const int track,
	                           Coord3d<float>& retCoord) const
	{
		//iterate over all time points and determine two closest enclosing time points
		auto earlierT = this->begin();
		while (earlierT != this->end() && earlierT->first < time) earlierT++;

		//all tracks are stored at earlier time (= asked time is behind any track we have)
		if (earlierT == this->end()) return false;

		//here: earlierT => time
		if (earlierT->first == time && earlierT->second.find(track) != earlierT->second.end())
		{
			//track exists at exactly the queried time, return its coord
			retCoord = earlierT->second.at(track);
			return true;
		}

		//backup the current iterator
		auto laterT = earlierT;

		//here: either earlierT > time or track@time does not exists
		//we go back looking for first occurrence of the track
		if (earlierT != this->begin()) earlierT--;
		else return false;
		     //all tracks are stored at later time (= asked time is before any track we have)

		//here: earlierT < time
		while (earlierT != this->begin() && earlierT->second.find(track) == earlierT->second.end()) earlierT--;
		if (earlierT == this->begin() && earlierT->second.find(track) == earlierT->second.end()) return false;
		//here: earlierT points at closest track@lower_than_time

		//DEBUG_REPORT("earlier is at " << earlierT->first);

		//we go forward looking for first occurrence of the track
		while (laterT != this->end() && laterT->second.find(track) == laterT->second.end()) laterT++;
		if (laterT == this->end()) return false;

		//DEBUG_REPORT("later is at " << laterT->first);

		//finally, compute weighted sum of the closest coords
		const float w = (time - earlierT->first) / (laterT->first - earlierT->first);
		retCoord = earlierT->second.at(track);
		retCoord *= 1.f-w;
		retCoord += w*laterT->second.at(track);

		return true;
	}


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
	                const bool doReset) const
	{
#ifdef DEBUG
		if (!FF.isConsistent())
			throw ERROR_REPORT("inconsistent FF given!");
#endif
		i3d::Image3d<int> FFcnts;
		if (doReset)
		{
			//clear the FF, and prepare the count image
			FF.x->GetVoxelData() = 0;
			FF.y->GetVoxelData() = 0;
			FF.z->GetVoxelData() = 0;

			FFcnts.CopyMetaData(*FF.x);
			FFcnts.GetVoxelData() = 0;
		}

		//offset and resolution of the flow field images/containers
		const Vector3d<float> off(FF.x->GetOffset());
		const Vector3d<float> res(FF.x->GetResolution().GetRes());

		//any position in pixels
		Vector3d<float> pos;
		Vector3d<int> posI;

		//pixels size of the 'propagationRadius' w.r.t. resolution
		pos = res;
		pos.elemMult(propagationRadius).elemCeil();
		const Vector3d<int> halfBoxSize((int)pos.x,(int)pos.y,(int)pos.z);
		int x,y,z;

		Vector3d<float> sigmaSq((float)halfBoxSize.x,(float)halfBoxSize.y,(float)halfBoxSize.z);
		sigmaSq.elemDivBy( Vector3d<float>(2.5f) ); //halfBoxSize _nearly_ as much as 3*sigma
		sigmaSq.elemMult( sigmaSq );
		//
		//assure sigmaSq is never zero and is mega-large (=100px) where it would have stayed zero
		if (halfBoxSize.x == 0) sigmaSq.x = 10000;
		if (halfBoxSize.y == 0) sigmaSq.y = 10000;
		if (halfBoxSize.z == 0) sigmaSq.z = 10000;
		//
		DEBUG_REPORT("halfBoxSize = " << halfBoxSize << ", sigmaSq = " << sigmaSq);

		//positions in microns (in real world coords) at a track at the given two times
		Coord3d<float> A,B;

		//short-cuts to the FF images
		float* const FFx = FF.x->GetFirstVoxelAddr();
		float* const FFy = FF.y->GetFirstVoxelAddr();
		float* const FFz = FF.z->GetFirstVoxelAddr();
		int*   const FFc = FFcnts.GetFirstVoxelAddr();

		//iterates over all track IDs
		for (const auto id : knownTracks)
		if (getPositionAlongTrack(timeFrom,id,A) &&
			 getPositionAlongTrack(timeTo,  id,B))
		{
			//managed to obtain both track positions,
			//the displacement vector (in microns)
			B -= A;

			//insert the displacement vector into a box of 'propagationRadius'
			//around the position A (projected into the FF's images)
			pos.from(A).toPixels(res,off); //in px within the FF's images

			//round to the nearest pixel (integer) coordinate
			posI.toPixels(pos);

			//sweep the box
			for (z = posI.z - halfBoxSize.z; z <= posI.z + halfBoxSize.z; ++z)
			for (y = posI.y - halfBoxSize.y; y <= posI.y + halfBoxSize.y; ++y)
			{
				float attenuation  = (pos.z-(float)z)*(pos.z-(float)z) /sigmaSq.z;
				      attenuation += (pos.y-(float)y)*(pos.y-(float)y) /sigmaSq.y;

				//voxel offset at [0, y,z] in the image
				long index = z*(signed)FF.x->GetSliceSize() + y*(signed)FF.x->GetWidth();
				//
				//now at [pos.x-halfBoxSize.x, y,z]
				index += posI.x - halfBoxSize.x;

				for (x = posI.x - halfBoxSize.x; x <= posI.x + halfBoxSize.x; ++x)
				{
					if (FF.x->Include(x,y,z))
					{
						attenuation += (pos.x-(float)x)*(pos.x-(float)x) /sigmaSq.x;
						attenuation  = smoothExpansion? std::exp(-0.5f*attenuation) : 1.0f;

						if (doReset)
						{
							//is inside the image, put a vector then
							*(FFx+index) += attenuation * B.x;
							*(FFy+index) += attenuation * B.y;
							*(FFz+index) += attenuation * B.z;
							*(FFc+index) += 1;
						}
						else
						{
							*(FFx+index) = attenuation * B.x;
							*(FFy+index) = attenuation * B.y;
							*(FFz+index) = attenuation * B.z;
						}
					}
					++index;
				}
			}
		}

		if (doReset)
		{
			//now, finish the averaging
			for (size_t index=0; index < FFcnts.GetImageSize(); ++index)
			if (*(FFc+index) > 0)
			{
				*(FFx+index) /= (float)*(FFc+index);
				*(FFy+index) /= (float)*(FFc+index);
				*(FFz+index) /= (float)*(FFc+index);
			}
		}
	}


public:
	/** Scans over all tracks and displays displacement vectors between
	    the given time span. The vectors are drawn using the display
	    unit, with the given color and with increasing IDs starting from
	    the one given. The number of drawn/created vectors is returned. */
	int drawFF(const float timeFrom, const float timeTo,
	           DisplayUnit& du, const int ID, const int color) const
	{
		//return value
		int elemCnt = 0;

		//positions at a track at the given two times
		Coord3d<float> A,B;

		//iterates over all track IDs
		for (const auto id : knownTracks)
		{
			if (getPositionAlongTrack(timeFrom,id,A) &&
			    getPositionAlongTrack(timeTo,  id,B))
			{
				//managed to obtain both track positions
				du.DrawVector(ID+ elemCnt++,A,B-A,color);
			}
		}

		return elemCnt;
	}
};
#endif
