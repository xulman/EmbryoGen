#include <list>
#include <string.h>
#include <stdlib.h>		//for atoi()

#include "params.h"		//for data structures
#include "agents.h"

#include "simulation.h" //for initializeAgents()
#include "graphics.h"	//for initializeGL()

//create single object (an instance of the global parameters class)
ParamsClass params;

//the list of all agents
std::list<Cell*> agents;


int main(int argc,char **argv)
{
	//set up the environment
    params.sceneOffset=Vector3d<float>(0.f);
    params.sceneSize=Vector3d<float>(200.f,200.f,1.f); //az se vyladi sily mezi nima
    //params.sceneSize=Vector3d<float>(60.f,60.f,1.f); //testing with mode=1
	params.sceneOuterBorder=Vector3d<float>(10.f);
	params.sceneBorderColour.r=0.5f;
	params.sceneBorderColour.g=0.5f;
	params.sceneBorderColour.b=0.5f;

	params.inputCellsFilename="cells/cell%d.txt";
	params.numberOfAgents=169;

	//some parameter supplied?
	if (argc > 1)
	{
		int userNoOfAgents=atoi(argv[1]);
		if ((userNoOfAgents > 0) && (userNoOfAgents < 1000))
			params.numberOfAgents=userNoOfAgents;
		else
			REPORT("Unreasonable number of cells supplied, staying with default value instead.");
	}

	params.friendshipDuration=10.f;
	params.maxCellSpeed=0.20f;

	//adapted for cell cycle length of 24 hours
	params.cellCycleLength=14.5*60; //[min]
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

	params.imgSizeX=792; //pixels
	params.imgSizeY=792;
	params.imgResX=3.6f; //pixels per micrometer
	params.imgResY=3.6f;

	params.imgOutlineFilename="Outline%05d.tif";
	params.imgPhantomFilename="Phantom%05d.tif";
	params.imgMaskFilename="Mask%05d.tif";
	params.imgFluoFilename="FluoImg%05d.tif";
	params.imgPhCFilename="PhCImg%05d.tif";
	params.tracksFilename="tracks.txt";

#ifdef RENDER_TO_IMAGES
	initiateImageFile();
#endif

	//init agents accordingly
    initializeAgents(0);
    //initializeAgents(1);

	//fire up the simulation
	if (!initializeGL()) return(-1);
	loopGL();
	closeGL();

	//close
	closeAgents();
	return(0);
}
