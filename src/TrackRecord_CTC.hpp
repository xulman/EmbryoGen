#ifndef TRACKRECORD_CTC_H
#define TRACKRECORD_CTC_H

#include <map>
#include <fstream>

/** A datatype for keeping records of tracks according
    to the Cell Tracking Challenge (CTC) format */
struct TrackRecord_CTC
{
	int ID;
	int fromTimeStamp;
	int toTimeStamp;
	int parentID;

	TrackRecord_CTC(int id, int from, int to, int pid):
		ID(id), fromTimeStamp(from), toTimeStamp(to), parentID(pid) {};

	//default values are such to indicate that this track was not initialized
	TrackRecord_CTC():
		ID(-1), fromTimeStamp(-1), toTimeStamp(-1), parentID(0) {};
};


/** A datatype to hold all CTC tracks with a few convenience functions such
    as startNewTrack() or exportAllToFile() */
class TrackRecords_CTC: public std::map<int,TrackRecord_CTC>
{
public:
	/** initiates a new track ID at time point frameNo */
	void startNewTrack(const int ID, const int frameNo)
	{
		(*this)[ID] = TrackRecord_CTC(ID,frameNo,-1,0);
	}

	/** MoID mother got divided into DoAID and DoBID daughters
	    who came to being at frameNo, mother was last seen at frameNo-1.
	    It is also assumed that mother's track record is already existing. */
	void closeMotherStartDaughtersTracks(const int MoID,
	                   const int DoAID, const int DoBID,
	                   const int frameNo)
	{
		//close the mother tracks
		(*this)[MoID].toTimeStamp = frameNo-1;

		//start up two new daughter tracks
		(*this)[DoAID] = TrackRecord_CTC(DoAID,frameNo,-1,MoID);
		(*this)[DoBID] = TrackRecord_CTC(DoBID,frameNo,-1,MoID);
	}

	/** additionally (re)defines the parental link of the track ID */
	void updateParentalLink(const int ID, const int parentID)
	{
		(*this)[ID].parentID = parentID;
	}

	/** closes the track ID at time point frameNo */
	void closeTrack(const int ID, const int frameNo)
	{
		(*this)[ID].toTimeStamp = frameNo;
	}

	/** returns true if the track ID has been closed already */
	bool isTrackClosed(const int ID)
	{
		return ( (*this)[ID].toTimeStamp > -1 );
	}

	/** returns true if the track ID is registered/listed/tracked/present */
	bool isTrackFollowed(const int ID)
	{
		return ( this->find(ID) != this->end() );
	}

	void exportAllToFile(const char* filename)
	{
		// write out the track record:
		std::map<int,TrackRecord_CTC>::const_iterator itTr;
		std::ofstream of(filename);
		if (! of.is_open())
			throw new std::runtime_error(EREPORT("Cannot write to ").append(filename));

		for (itTr = (*this).begin(); itTr != (*this).end(); itTr++)
		{
			//export on harddrive (only if the cell has really been displayed at least once)
			if (itTr->second.toTimeStamp >= itTr->second.fromTimeStamp)
				of << itTr->second.ID << " "
					<< itTr->second.fromTimeStamp << " "
					<< itTr->second.toTimeStamp << " "
					<< itTr->second.parentID << std::endl;
		}
		of.close();
	}
};
#endif
