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

#ifdef RENDER_TO_IMAGES
 //link here the counter to get continous file name indices
 extern int frameCnt;

 //the structure to hold all track data
 #include <map>
 std::map<int, TrackRecord> tracks;
#endif


void initializeAgents(const int mode)
{
	//bank of sampled cells
	/* original shapes
	const int filesNo=3;
	char fileNames[filesNo][30]={
			"cells/cell_1.txt",
			"cells/cell_2.txt",
			"cells/cell_3.txt"
		};
	*/
	int filesNo=0;
	char fileNames[100][1024]; //can host 100 pieces of 1 kB long filenames

	//iteratively tries to open cell files under of given input filename format
	for (int fnum=1; fnum < 20; ++fnum)
	{
		char fn[1024];
		//"instantiate" the current cell filename
		sprintf(fn,params.inputCellsFilename.c_str(),fnum);

		REPORT_NOENDL("trying to read cell file: " << fn);

		//try to open/read it
		std::ifstream tfn(fn);
		if (tfn.is_open())
		{
			tfn.close();

			//remember this filename
			memcpy(fileNames[filesNo],fn,1024);
			++filesNo;

			std::cout << " -  OK\n";
		}
		else
			std::cout << " - NOK\n";
	}
	DEBUG_REPORT(filesNo << " cells are in the pool");

	if (filesNo == 0)
	{
		REPORT("");
		REPORT("I was not able to open any cell configuration file.\n");
		REPORT("Please, copy to the current folder the folder \"cells\" where");
		REPORT("sample configuration files can be found.\n");
		REPORT("Or adjust path to the files in the file src/main.cpp on the line");
		REPORT("which reads:   params.inputCellsFilename=\"cells/cell%02d.txt\"; ");
		REPORT("and recompile.");
		exit(EXIT_FAILURE);
	}

	if (mode == 3) //wound healing
	{
        //how many grid lines and columns first:
        const int rows=3;
        const int cols=(int)ceilf((float)params.numberOfAgents / (float)rows);

        //now the distance between the lines:
        const float xStepping=(float)params.sceneSize.x / (float)(cols+1);
        const float yStepping=20.f;

		  params.numberOfAgents *= 2;

        DEBUG_REPORT("sceneSize=(" << params.sceneSize.x << "," << params.sceneSize.y << "," << params.sceneSize.z << ")");
        DEBUG_REPORT("sceneOffset=(" << params.sceneOffset.x << "," << params.sceneOffset.y << "," << params.sceneOffset.z << ")");

        int counter=0;
        float y=params.sceneOffset.y+yStepping;
        float x=params.sceneOffset.x+xStepping;
		  int yL=0;
		  while (yL < 2*rows+5)
		  {
		  	if (yL == rows) yL+=5;

         for (int xL=0; xL < cols; ++xL)
            if (counter < params.numberOfAgents)
            {
                Cell* ag=new Cell(fileNames[counter % filesNo]);
					 ag->pos.x=x+(float)xL*xStepping;
					 ag->pos.y=y+(float)yL*yStepping;

                agents.push_back(ag);
                //std::cout << "adding at [" << agents.back()->pos.x << "," << agents.back()->pos.y << "]\n";
                ++counter;
            }
		   ++yL;
		  }
	}
	else if (mode == 2) //"as is" from TXT files
	{
		//read cells from the entire bank,
		//and place them as suggested in the files
		//const int readFiles=1;
        const int readFiles=filesNo;
        for (int i=0; i < readFiles; ++i)
			agents.push_back( new Cell(fileNames[i]) );

		//update the number of cells in the system
		params.numberOfAgents=readFiles;
	}
	else if (mode == 10) //read initial positions from an external file
	{
		ifstream pos("initialPositions_clean.dat");
		int counter=0;
		float x,y;

		while ((pos >> x >> y) && (counter < params.numberOfAgents))
		{
			Cell* ag=new Cell(fileNames[counter % filesNo]);
			ag->pos.x=x;
			ag->pos.y=y;
			
			agents.push_back(ag);
			//std::cout << "adding at [" << agents.back()->pos.x << "," << agents.back()->pos.y << "]\n";
			++counter;
		}
		pos.close();
		params.numberOfAgents=counter; //in case the file is shorter than demanded
	}
	else if (mode == 1) //two cells for testing
	{
		//just two agents close to each other
		DEBUG_REPORT("going to introduce 2 nearby agents");

		agents.push_back( new Cell(fileNames[1],0.,0.,1.) );
		agents.push_back( new Cell(fileNames[2],1.,0.,0.) );
		agents.front()->pos.x=15.;
		agents.front()->pos.y=30.;
		agents.back()->pos.x=45.;
		agents.back()->pos.y=30.;

		//possibly, force these two to be friends....
		//agents.front()->listOfFriends.push_back(Cell::myFriend(agents.back(),30.));
		//break symmetry...
		//agents.back()->listOfFriends.push_back(Cell::myFriend(agents.front(),30.));

		//update the number of cells in the system
		params.numberOfAgents=2;
	}
	else  //uniformly spread cells -- a grid
    {
        //how many grid lines and columns first:
        const int rows=(int) ceilf(sqrtf((float)params.numberOfAgents *
                                params.sceneSize.y / params.sceneSize.x));
        const int cols=(int) ceilf((float)params.numberOfAgents / (float)rows);

        //now the distance between the lines:
        const float xStepping=(float)params.sceneSize.x / (float)(cols+1);
        const float yStepping=(float)params.sceneSize.y / (float)(rows+1);

        DEBUG_REPORT("going to introduce " << params.numberOfAgents << " grid-aligned agents");
        DEBUG_REPORT("into a grid of " << rows << " rows and " << cols << " columns");

        DEBUG_REPORT("sceneSize=(" << params.sceneSize.x << "," << params.sceneSize.y << "," << params.sceneSize.z << ")");
        DEBUG_REPORT("sceneOffset=(" << params.sceneOffset.x << "," << params.sceneOffset.y << "," << params.sceneOffset.z << ")");

        int counter=0;
        float y=params.sceneOffset.y+yStepping;
        float x=params.sceneOffset.x+xStepping;
        for (int yL=0; yL < rows; ++yL)
         for (int xL=0; xL < cols; ++xL)
            if (counter < params.numberOfAgents)
            {
					 int cellShape_ID=int(GetRandomUniform(0.f,20.f*(float)filesNo)) % filesNo;
                Cell* ag=new Cell(fileNames[cellShape_ID]);
					 ag->pos.x=x+(float)xL*xStepping;
					 ag->pos.y=y+(float)yL*yStepping;

                agents.push_back(ag);
                //std::cout << "adding at [" << agents.back()->pos.x << "," << agents.back()->pos.y << "]\n";
                ++counter;
            }
    }

#ifdef RENDER_TO_IMAGES
	//go over all cells and create their tracks
	 std::list<Cell*>::const_iterator c=agents.begin();
	 for (; c != agents.end(); c++)
	 	tracks.insert(std::pair<int,TrackRecord>((*c)->ID,TrackRecord(frameCnt,-1,0)));
#endif
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

	 //create removable (can't run in parallel)
	 c=agents.begin();
	 while (c != agents.end())
	 {
		  if ((*c)->shouldDie)
		  {
				//schedule this one for removal
				dead_agents.push_back(*c);
#ifdef RENDER_TO_IMAGES
		 		if (tracks[(*c)->ID].toTimeStamp > -1)
					REPORT("CONFUSION: dying cell HAS CLOSED TRACK!");
				tracks[(*c)->ID].toTimeStamp=frameCnt-1;
#endif
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
		delete *iter; *iter = NULL;
		iter++;
	}

	DEBUG_REPORT("all agents were removed...");

#ifdef RENDER_TO_IMAGES
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
#endif
}

#ifdef RENDER_TO_IMAGES
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
#endif
