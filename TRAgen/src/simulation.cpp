#include <list>
#include <iostream>
#include <fstream>
#include <cmath>

#include <string.h>		//for memcpy()
#include <stdio.h>		//for sprintf()
#include <stdlib.h>		//for exit()

#include "params.h"		//for data structures
#include "rnd_generators.h" //for random chosing of textures
#include "agents.h"

//if defined, OpenMP is used to utilize (possibly avaiable) multi-threading
//#define MULTITHREADING - not used now

using namespace std;

//link here the global params
extern ParamsClass params;

//link here the list of all agents
extern std::list<Cell*> agents;

 //link here the counter to get continous file name indices
 extern int frameCnt;

 //the structure to hold all track data
 #include <map>
 std::map<int, TrackRecord> tracks;


void initializeAgents(DisplayUnit* const ds)
{
	//stepping in all directions -> influences the final number of nuclei
	const float dx = 14.0f;

	//longer axis x
	//symmetric/short axes y,z

	const float Xside  = (0.9f*params.sceneSize.x)/2.0f;
	const float YZside = (0.9f*params.sceneSize.y)/2.0f;

	for (float z=-Xside; z <= +Xside; z += dx)
	{
		//radius at this z position
		const float radius = YZside * sin(acos(fabsf(z)/Xside));

		const int howManyToPlace = (int)ceil(6.28f*radius / dx);
		for (int i=0; i < howManyToPlace; ++i)
		{
			const float ang = float(i)/float(howManyToPlace);

			Cell* ag=new Cell();
			//the wished position relative to [0,0,0] centre
			ag->pos.x = z;
			ag->pos.y = radius * cosf(ang*6.28f);
			ag->pos.z = radius * sinf(ang*6.28f);

			//position is shifted to the scene centre
			ag->pos.x += params.sceneSize.x/2.0f;
			ag->pos.y += params.sceneSize.y/2.0f;
			ag->pos.z += params.sceneSize.z/2.0f;

			//drawing info...
			ag->displayUnit = ds;
			ag->DrawIntoDisplayUnit();

			agents.push_back(ag);

			//std::cout << "adding at " << agents.back()->pos << "\n";
		}
	}
	params.numberOfAgents = (int)agents.size();

	//ask any cell (e.g. the first one) to forward the flush request to the used display unit
	agents.front()->FlushDisplayUnit();

	//agents.front()->isSelected = true;

	//go over all cells and create their tracks
	 std::list<Cell*>::const_iterator c=agents.begin();
	 for (; c != agents.end(); c++)
	 	tracks.insert(std::pair<int,TrackRecord>((*c)->ID,TrackRecord(frameCnt,-1,0)));
}


void moveAgents(const float timeDelta)
{
	//(persistent) list of agents to be removed
	static std::list<Cell*> dead_agents;

	 //obtain new shapes... (can run in parallel)
	 std::list<Cell*>::iterator c=agents.begin();
	 for (; c != agents.end(); c++)
		  (*c)->adjustShape(timeDelta);

	 //obtain fresh forces... (can run in parallel)
	 for (c=agents.begin(); c != agents.end(); c++)
		  (*c)->calculateForces(timeDelta);

	 //obtain new positions... (can run in parallel)
	 for (c=agents.begin(); c != agents.end(); c++)
		  (*c)->applyForces(timeDelta);

	 //remove removable (can't run in parallel)
	 c=dead_agents.begin();
	 while (c != dead_agents.end())
	 {
	 	delete (*c);
		c=dead_agents.erase(c);
	 }

	 //TODO: remove after sometime
	 if (dead_agents.size() != 0)
		 REPORT("CONFUSION: dead_agents IS NOT EMPTY!");

/*
	 //create removable (can't run in parallel)
	 c=agents.begin();
	 while (c != agents.end())
	 {
		  if ((*c)->shouldDie)
		  {
				//schedule this one for removal
				dead_agents.push_back(*c);

		 		if (tracks[(*c)->ID].toTimeStamp > -1)
					REPORT("CONFUSION: dying cell HAS CLOSED TRACK!");
				tracks[(*c)->ID].toTimeStamp=frameCnt-1;

		  		c=agents.erase(c);
				params.numberOfAgents--;
				//this removed the agent from the list of active cells,
				//but does not remove it from the memory!
				//(so that other agents that still hold _pointer_
				//on this one in their Cell::listOfFriends lists
				//can _safely_ notice this one has Cell::shouldDie
				//flag on and take appropriate action)
				//
				//after the next round of Cell::applyForces() is over,
				//all agents have definitely removed this agent from
				//their Cell::listOfFriends, that is when we can delete
				//this agent from the memory
		  }
		  else c++;
	 }
*/

#ifdef TRAGEN_DEBUG
	 if (dead_agents.size() > 0)
		 REPORT(dead_agents.size() << " cells will be removed from memory");
#endif
}


void closeAgents(void)
{
	//delete all agents...
	std::list<Cell*>::iterator iter=agents.begin();
	while (iter != agents.end())
	{
		(*iter)->displayUnit = NULL;
		delete *iter; *iter = NULL;
		iter++;
	}

	DEBUG_REPORT("all agents were removed...");

	 // write out the track record:
	 std::map<int, TrackRecord >::iterator itTr;
	 std::ofstream of(params.tracksFilename.c_str());

	 for (itTr = tracks.begin(); itTr != tracks.end(); itTr++)
	 {
		  //finish unclosed tracks....
		  if (itTr->second.toTimeStamp < 0) itTr->second.toTimeStamp=frameCnt-1;

		  //export on harddrive (only if the cell has really been displayed at least once)
		  if (itTr->second.toTimeStamp >= itTr->second.fromTimeStamp)
		  		of << itTr->first << " " 
					 << itTr->second.fromTimeStamp << " "
					 << itTr->second.toTimeStamp << " "
					 << itTr->second.parentID << std::endl;
	 }
	of.close();
}

void ReportNewBornDaughters(const int MoID,
	const int DoAID, const int DoBID)
{
	//close the mother tracks
	tracks[MoID].toTimeStamp=frameCnt-1;

	//start up two new daughter tracks
	tracks[DoAID].fromTimeStamp=frameCnt;
	tracks[DoAID].toTimeStamp=-1;
	tracks[DoAID].parentID=MoID;

	tracks[DoBID].fromTimeStamp=frameCnt;
	tracks[DoBID].toTimeStamp=-1;
	tracks[DoBID].parentID=MoID;
}
