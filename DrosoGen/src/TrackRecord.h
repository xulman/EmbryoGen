#ifndef TRACKRECORD_H
#define TRACKRECORD_H

#include <map>

/// a datatype for keeping records of tracks
struct TrackRecord
{
	int ID;
	int fromTimeStamp;
	int toTimeStamp;
	int parentID;

	TrackRecord(int id, int from, int to, int pid):
		ID(id), fromTimeStamp(from), toTimeStamp(to), parentID(pid) {};

	TrackRecord():
		ID(0), fromTimeStamp(0), toTimeStamp(0), parentID(0) {};

	/** initiates a new track ID at time point frameNo */
	static
	void StartNewTrack(std::map<int,TrackRecord>& tracks,
		const int ID, const int frameNo)
	{
		tracks[ID] = TrackRecord(ID,frameNo,-1,0);
	}

	/** MoID mother got divided into DoAID and DoBID daughters
	    who came to being at frameNo, mother was last seen at frameNo-1.
	    It is also assumed that mother's track record is already existing. */
	static
	void ReportNewBornDaughters(std::map<int,TrackRecord>& tracks,
		const int MoID,
		const int DoAID, const int DoBID,
		const int frameNo)
	{
		//close the mother tracks
		tracks[MoID].toTimeStamp=frameNo-1;

		//start up two new daughter tracks
		tracks[DoAID].fromTimeStamp=frameNo;
		tracks[DoAID].toTimeStamp=-1;
		tracks[DoAID].parentID=MoID;

		tracks[DoBID].fromTimeStamp=frameNo;
		tracks[DoBID].toTimeStamp=-1;
		tracks[DoBID].parentID=MoID;
	}

	/** closes the track ID at time point frameNo */
	static
	void CloseTrack(std::map<int,TrackRecord>& tracks,
		const int ID, const int frameNo)
	{
		tracks[ID].toTimeStamp = frameNo;
	}
};
#endif
