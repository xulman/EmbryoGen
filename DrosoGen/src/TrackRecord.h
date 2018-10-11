#ifndef TRACKRECORD_H
#define TRACKRECORD_H

#include <map>
#include <set>
#include <fstream>
#include "util/Vector3d.h"

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
	/** a set of IDs of tracks that are recorded in this object,
	    the synchronization between *this and this variable is, however,
	    not enforced */
	std::set<int> knownTracks;

	/** adds to the current trajectories whatever it finds in the 'filename',
	    the file should be a text file with white-space separated items,
	    in this order, TIME X Y Z TRACK_ID

	    the function also updates this->knownTracks */
	void readFromFile(const char* filename)
	{
		std::ifstream f(filename);
		if (! f.is_open())
			throw new std::runtime_error(std::string("TrackRecords::readFromFile(): Cannot open ").append(filename));

		float time,x,y,z;
		int id;
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
			f >> time >> x >> y >> z >> id;
			if (f.good())
			{
				//store the record
				auto& rr = (*this)[time];
				rr[id] = Coord3d<float>(x,y,z);

				knownTracks.insert(id);
			}
		}

		f.close();
	}


	/** writes all trajectories back into a text 'filename' as white-space
	    separated times, in this order, TIME X Y Z TRACK_ID */
	void writeToFile(const char* filename) const
	{
		std::ofstream f(filename);
		if (! f.is_open())
			throw new std::runtime_error(std::string("TrackRecords::writeToFile(): Cannot open ").append(filename));

		auto t = this->begin();
		for (; t != this->end(); ++t)
		{
			for (const auto& id : t->second)
			{
				f << t->first << "\t"
				  << id.second.x << "\t"
				  << id.second.y << "\t"
				  << id.second.z << "\t"
				  << id.first << "\n";
			}
		}

		f.close();
	}


	/** report displacement vector for a given 'track' between the two time points,
	    or use 'defaultVec' if no such information is available in the records */
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


	/** interpolate (if necessary) coordinate along the 'track' at 'time',
	    returns true (and valid 'retCoord') if that was possible, otherwise
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
};
#endif
