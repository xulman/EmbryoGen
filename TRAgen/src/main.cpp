#include <list>

#ifdef __APPLE__
#  include <GLUT/glut.h>
#else
#  include "GL/glut.h"
#endif

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
    params.sceneSize=Vector3d<float>(1152.f,921.f,1.f); //az se vyladi sily mezi nima
    //params.sceneSize=Vector3d<float>(60.f,60.f,1.f); //testing with mode=1
	params.sceneOuterBorder=Vector3d<float>(25.f);
	params.sceneBorderColour.r=0.5f;
	params.sceneBorderColour.g=0.5f;
	params.sceneBorderColour.b=0.5f;

    params.numberOfAgents=150;

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

	params.currTime=0.f;
	params.incrTime=0.1f;
	params.stopTime=1000.f; //100 steps x 10mins

	//init agents accordingly
    initializeAgents(10);
    //initializeAgents(1);

	//fire up the simulation
	initializeGL();
	glutMainLoop();

	//close
	closeAgents();
	return(0);
}
