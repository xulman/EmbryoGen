#ifndef TRACKRECORD_H
#define TRACKRECORD_H

#include <map>
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
	/** adds to the current trajectories whatever it finds in the 'filename',
	    the file should be a text file with white-space separated items,
	    in this order, TIME X Y Z TRACK_ID */
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
			}
		}

		f.close();
	}


	/** writes all trajectories back into a text 'filename' as white-space
	    separated times, in this order, TIME X Y Z TRACK_ID */
	void writeToFile(const char* filename)
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
};
#endif
