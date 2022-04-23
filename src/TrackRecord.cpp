#include "TrackRecord.hpp"
#include "DisplayUnits/DisplayUnit.hpp"
#include "util/FlowField.hpp"
#include "util/Vector3d.hpp"
#include "util/report.hpp"
#include <fstream>

void TrackRecords::readFromFile(const char* filename,
                                const Vector3d<float>& spaceScale,
                                const float timeScale,
                                const float timeShift) {
	std::ifstream f(filename);
	if (!f.is_open())
		throw report::rtError(fmt::format("Cannot read from {}", filename));

	float time, x, y, z;
	int id, parent, ignore;
	char c;

	while (f.good()) {
		// read the first character
		f >> c;

		// if it starts with '#' then we ignore the complete line
		if (c == '#') {
			f.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
			continue;
		}

		// else, we return back the first character, and start reading the
		// record
		f.putback(c);
		f >> time >> x >> y >> z >> id >> parent >> ignore;
		if (f.good()) {
			// store the record
			auto& rr = (*this)[time * timeScale + timeShift];
			rr[id] = Coord3d<float>(x, y, z);
			rr[id].elemMult(spaceScale);

			knownTracks.insert(id);
		}
	}

	f.close();
}

void TrackRecords::writeToFile(const char* filename) const {
	std::ofstream f(filename);
	if (!f.is_open())
		throw report::rtError(fmt::format("Cannot write to {}", filename));

	auto t = this->begin();
	for (; t != this->end(); ++t) {
		for (const auto& id : t->second) {
			f << t->first << "\t" << id.second.x << "\t" << id.second.y << "\t"
			  << id.second.z << "\t" << id.first << " 0 0\n";
		}
	}

	f.close();
}

Vector3d<float>
TrackRecords::getDisplacement(const float timeFrom,
                              const float timeTo,
                              const int track,
                              const Vector3d<float>& defaultVec) const {
	try {
		const Coord3d<float>& start = this->at(timeFrom).at(track);
		const Coord3d<float>& end = this->at(timeTo).at(track);
		return (end - start);
	} catch (std::out_of_range*) {
		return defaultVec;
	}
}

/** Interpolates (if necessary) coordinate along the 'track' at 'time'.
    Returns true (and valid 'retCoord') if that was possible, otherwise
     (if 'time' is outside the track's time span, for instance) returns false.
 */
bool TrackRecords::getPositionAlongTrack(const float time,
                                         const int track,
                                         Coord3d<float>& retCoord) const {
	// iterate over all time points and determine two closest enclosing time
	// points
	auto earlierT = this->begin();
	while (earlierT != this->end() && earlierT->first < time)
		earlierT++;

	// all tracks are stored at earlier time (= asked time is behind any track
	// we have)
	if (earlierT == this->end())
		return false;

	// here: earlierT => time
	if (earlierT->first == time &&
	    earlierT->second.find(track) != earlierT->second.end()) {
		// track exists at exactly the queried time, return its coord
		retCoord = earlierT->second.at(track);
		return true;
	}

	// backup the current iterator
	auto laterT = earlierT;

	// here: either earlierT > time or track@time does not exists
	// we go back looking for first occurrence of the track
	if (earlierT != this->begin())
		earlierT--;
	else
		return false;
	// all tracks are stored at later time (= asked time is before any track we
	// have)

	// here: earlierT < time
	while (earlierT != this->begin() &&
	       earlierT->second.find(track) == earlierT->second.end())
		earlierT--;
	if (earlierT == this->begin() &&
	    earlierT->second.find(track) == earlierT->second.end())
		return false;
	// here: earlierT points at closest track@lower_than_time

	// DEBUG_REPORT("earlier is at " << earlierT->first);

	// we go forward looking for first occurrence of the track
	while (laterT != this->end() &&
	       laterT->second.find(track) == laterT->second.end())
		laterT++;
	if (laterT == this->end())
		return false;

	// DEBUG_REPORT("later is at " << laterT->first);

	// finally, compute weighted sum of the closest coords
	const float w =
	    (time - earlierT->first) / (laterT->first - earlierT->first);
	retCoord = earlierT->second.at(track);
	retCoord *= 1.f - w;
	retCoord += w * laterT->second.at(track);

	return true;
}

int TrackRecords::drawFF(const float timeFrom,
                         const float timeTo,
                         DisplayUnit& du,
                         const int ID,
                         const int color) const {
	// return value
	int elemCnt = 0;

	// positions at a track at the given two times
	Coord3d<float> A, B;

	// iterates over all track IDs
	for (const auto id : knownTracks) {
		if (getPositionAlongTrack(timeFrom, id, A) &&
		    getPositionAlongTrack(timeTo, id, B)) {
			// managed to obtain both track positions
			du.DrawVector(ID + elemCnt++, A, B - A, color);
		}
	}

	return elemCnt;
}
