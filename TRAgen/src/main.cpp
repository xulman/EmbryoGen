#include <list>
#include <string.h>

#include "params.h"		//for data structures
#include "agents.h"

#include "simulation.h" //for initializeAgents()
#include "graphics.h"	//for initializeGL()

//create single object (an instance of the global parameters class)
ParamsClass params;

//the list of all agents
std::list<Cell*> agents;


int main(void)
{
	//set up the environment
    params.sceneOffset=Vector3d<float>(0.f);
    params.sceneSize=Vector3d<float>(200.f,200.f,1.f); //az se vyladi sily mezi nima
    //params.sceneSize=Vector3d<float>(60.f,60.f,1.f); //testing with mode=1
	params.sceneOuterBorder=Vector3d<float>(10.f);
	params.sceneBorderColour.r=0.5f;
	params.sceneBorderColour.g=0.5f;
	params.sceneBorderColour.b=0.5f;

    params.numberOfAgents=169;

	params.friendshipDuration=10.f;
	params.maxCellSpeed=0.20f;

	//adapted for cell cycle length of 24 hours
	params.cellCycleLength=14.5*60; //[min]
	params.cellPhaseDuration[G1Phase]=0.5 * params.cellCycleLength;
	params.cellPhaseDuration[SPhase]=0.3 * params.cellCycleLength;
	params.cellPhaseDuration[G2Phase]=0.15 * params.cellCycleLength;
	params.cellPhaseDuration[Prophase]=0.0125 * params.cellCycleLength;
	params.cellPhaseDuration[Metaphase]=0.0285 * params.cellCycleLength;
	params.cellPhaseDuration[Anaphase]=0.0025 * params.cellCycleLength;
	params.cellPhaseDuration[Telophase]=0.00325 * params.cellCycleLength;
	params.cellPhaseDuration[Cytokinesis]=0.00325 * params.cellCycleLength;

	params.currTime=0.f; //all three are in units of minutes
	params.incrTime=0.1f;
	params.stopTime=5000.f;

	params.imgSizeX=792; //pixels
	params.imgSizeY=792;
	params.imgResX=3.6f; //pixels per micrometer
	params.imgResY=3.6f;
	params.imgTestingFilename="img%05d.tif";
	params.imgMaskFilename="mask%05d.tif";
	params.imgOutlineFilename="outline%05d.tif";

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
