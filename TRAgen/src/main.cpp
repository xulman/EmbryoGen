#include <list>
#include <string.h>
#include <stdlib.h>		//for atoi()

#include <i3d/image3d.h>

#include "params.h"		//for data structures
#include "agents.h"

#include "simulation.h" //for initializeAgents()

#include "DisplayUnits/BroadcasterDisplayUnit.h"
#include "DisplayUnits/ConsoleDisplayUnit.h"
#include "DisplayUnits/SceneryDisplayUnit.h"

//create single object (an instance of the global parameters class)
ParamsClass params;

//the list of all agents
std::list<Cell*> agents;

//number of allready processed frames
int frameCnt;


int main(void)
{
	//CTC drosophila:
	//x,y,z res = 2.46306,2.46306,0.492369 px/um
	//embryo along x-axis
	//bbox size around the embryo: 480,220,220 um
	//bbox size around the embryo: 1180,540,110 px

	//set up the environment
	params.sceneOffset=Vector3d<float>(0.f);
	params.sceneSize=Vector3d<float>(480.f,220.f,220.f);
	params.sceneOuterBorder=Vector3d<float>(0.f);
	params.sceneBorderColour.r=0.5f;
	params.sceneBorderColour.g=0.5f;
	params.sceneBorderColour.b=0.5f;

	params.inputCellsFilename="cells/cell%d.txt";
	params.numberOfAgents=0;

	params.friendshipDuration=10.f;
	params.maxCellSpeed=0.20f;

	//adapted for cell cycle length of 24 hours
	//params.cellCycleLength=14.5*60; //[min]
	params.cellCycleLength=30; //[min]
	params.cellPhaseDuration[G1Phase]=0.5f * params.cellCycleLength;
	params.cellPhaseDuration[SPhase]=0.3f * params.cellCycleLength;
	params.cellPhaseDuration[G2Phase]=0.15f * params.cellCycleLength;
	params.cellPhaseDuration[Prophase]=0.0125f * params.cellCycleLength;
	params.cellPhaseDuration[Metaphase]=0.0285f * params.cellCycleLength;
	params.cellPhaseDuration[Anaphase]=0.0025f * params.cellCycleLength;
	params.cellPhaseDuration[Telophase]=0.00325f * params.cellCycleLength;
	params.cellPhaseDuration[Cytokinesis]=0.00325f * params.cellCycleLength;

	params.currTime=0.f; //all three are in units of minutes
	params.incrTime=0.1f;
	params.stopTime=5000.f;

	//original res
	//params.imgResX=2.46f; //pixels per micrometer
	//params.imgResY=2.46f;
	//params.imgResZ=0.49f;
	//
	//my res
	params.imgResX=2.0f; //pixels per micrometer
	params.imgResY=2.0f;
	params.imgResZ=2.0f;
	params.imgSizeX=(size_t)ceil(params.sceneSize.x * params.imgResX); //pixels
	params.imgSizeY=(size_t)ceil(params.sceneSize.y * params.imgResY);
	params.imgSizeZ=(size_t)ceil(params.sceneSize.z * params.imgResZ);

	std::cout << "scene size: "
	  << params.sceneSize.x << " x " << params.sceneSize.y << " x " << params.sceneSize.z
	  << "  =  "
	  << params.imgSizeX << " x " << params.imgSizeY << " x " << params.imgSizeZ << "\n";

	params.imgOutlineFilename="Outline%05d.tif";
	params.imgPhantomFilename="Phantom%05d.tif";
	params.imgMaskFilename="Mask%05d.tif";
	params.imgFluoFilename="FluoImg%05d.tif";
	params.imgPhCFilename="PhCImg%05d.tif";
	params.tracksFilename="tracks.txt";


	//init display units
	ConsoleDisplayUnit cDU;
	SceneryDisplayUnit sDU("localhost:8765");

	BroadcasterDisplayUnit bDU;
	bDU.RegisterUnit(cDU);
	bDU.RegisterUnit(sDU);
	//bDU.UnregisterUnit(sDU);

	//init agents accordingly
	initializeAgents(&sDU);
	//initializeAgents(NULL);

	std:: cout << "nuclei in the system: " << params.numberOfAgents << "\n";

	//output image that will be iteratively re-rendered
	i3d::Image3d<i3d::GRAY8> img;
	img.MakeRoom(params.imgSizeX, params.imgSizeY, params.imgSizeZ);
	img.SetResolution(*(new i3d::Resolution(params.imgResX, params.imgResY, params.imgResZ)));

	//filename buffer...
	char fn[1024];

	//wait for key...
	std::cin >> fn[0];

	//run the simulation
	for (frameCnt=0; frameCnt < 500; ++frameCnt)
	{
		std::cout << "--------------- " << frameCnt << " (" << agents.size() << " agents) ---------------\n";
		if (frameCnt % 5 == 0)
		{
			//render cells into the image
			img.GetVoxelData() = 0;
			std::list<Cell*>::const_iterator c=agents.begin();
			for (; c != agents.end(); c++)
			{
				(*c)->RasterInto(img);
				(*c)->DrawIntoDisplayUnit();
			}

			//sprintf(fn,"mask%03d.tif",frameCnt);
			//img.SaveImage(fn);

			//wait for key...
			std::cin >> fn[0];
		}

		//advance the simulation into the next frame
		moveAgents(params.incrTime);
		params.currTime += params.incrTime;
	}

	//close
	closeAgents();
	return(0);
}
