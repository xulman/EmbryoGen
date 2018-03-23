#include <list>
#include <iostream>
#include <fstream>
#include <cmath>

#include "params.h"		//for data structures
#include "agents.h"

//if defined, OpenMP is used to utilize (possibly avaiable) multi-threading
//#define MULTITHREADING

using namespace std;

//link here the global params
extern ParamsClass params;

//link here the list of all agents
extern std::list<Cell*> agents;


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
	const int filesNo=5;
	char fileNames[filesNo][30]={
			"cells/cell1.txt",
			"cells/cell2.txt",
			"cells/cell3.txt",
			"cells/cell4.txt",
			"cells/cell5.txt"
		};

	if (mode == 3) //wound healing
	{
        //how many grid lines and columns first:
        const int rows=3;
        const int cols=ceilf((float)params.numberOfAgents / rows);

        //now the distance between the lines:
        const float xStepping=(float)params.sceneSize.x / (float)(cols+1);
        const float yStepping=20.f;

		  params.numberOfAgents *= 2;

        std::cout << "sceneSize=(" << params.sceneSize.x << "," << params.sceneSize.y << "," << params.sceneSize.z << ")\n";
        std::cout << "sceneOffset=(" << params.sceneOffset.x << "," << params.sceneOffset.y << "," << params.sceneOffset.z << ")\n";

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
		std::cout << "going to introduce 2 nearby agents\n";

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
        const int rows=ceilf(sqrtf((float)params.numberOfAgents *
                                params.sceneSize.y / params.sceneSize.x));
        const int cols=ceilf((float)params.numberOfAgents / rows);

        //now the distance between the lines:
        const float xStepping=(float)params.sceneSize.x / (float)(cols+1);
        const float yStepping=(float)params.sceneSize.y / (float)(rows+1);

        std::cout << "going to introduce " << params.numberOfAgents << " grid-aligned agents\n";
        std::cout << "into a grid of " << rows << " rows and " << cols << " columns\n";

        std::cout << "sceneSize=(" << params.sceneSize.x << "," << params.sceneSize.y << "," << params.sceneSize.z << ")\n";
        std::cout << "sceneOffset=(" << params.sceneOffset.x << "," << params.sceneOffset.y << "," << params.sceneOffset.z << ")\n";

        int counter=0;
        float y=params.sceneOffset.y+yStepping;
        float x=params.sceneOffset.x+xStepping;
        for (int yL=0; yL < rows; ++yL)
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
    }
}


void moveAgents(const float timeDelta)
{
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
	 c=agents.begin();
	 while (c != agents.end())
	 {
		  if ((*c)->shouldDie)
		  {
		  		c=agents.erase(c);
				params.numberOfAgents--;
		  }
		  else c++;
	 }
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

	std::cout << "all agents were removed...\n";
}
