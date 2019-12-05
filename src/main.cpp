#include <iostream>
#include <i3d/basic.h>
#include "Scenarios/common/Scenario.h"
#include "Scenarios/common/Scenarios.h"
#include "Director.h"
#include "FrontOfficer.h"

int main(int argc, char** argv)
{
	Director* d = NULL;
	FrontOfficer* fo = NULL;

	try
	{
		//the Scenario object is always created because it contains
		//scenario-specific parameters and is used by the Director and all FOs
		Scenario& s = Scenarios(argc,argv).getScenario();

#ifdef DISTRIBUTED
		//these two has to come from MPI stack,
		//for now some fake values:
		const int MPI_noOfNodesInTotal = 72
		const int MPI_IDOfThisInstance = 0

		//this is assuming that MPI clients' IDs form a full
		//integer interval between 0 and MPI_noOfNodesInTotal-1,
		//we further consider 0 to be the Direktor's node,
		//and 1 till MPI_noOfNodesInTotal-1 for IDs for the FOs

		if (MPI_IDOfThisInstance == 0)
		{
			//hoho, I'm the Direktor  (NB: Direktor == main simulation loop)
			d = new Director(s,1,MPI_noOfNodesInTotal-1);
			d->initMPI();  //init the simulation, and render the first frame
			d->execute();  //execute the simulation, and render frames
			d->close();    //close the simulation, deletes agents, and save tracks.txt
		}
		else
		{
			//I'm FrontOfficer:
			int thisFOsID = MPI_IDOfThisInstance;
			int nextFOsID = thisFOsID+1; //builds the round robin schema

			//tell the last FO to send data back to the Direktor
			if (nextFOsID == MPI_noOfNodesInTotal) nextFOsID = 0;

			fo = new FrontOfficer(s,nextFOsID, MPI_IDOfThisInstance,MPI_noOfNodesInTotal-1);
			fo->initMPI(); //populate/create my part of the scene
			fo->execute(); //wait for Direktor's events
			fo->close();   //deletes my agents
		}
#else
		//single machine case
		d = new Director(s,1,1);
		fo = new FrontOfficer(s,0, 1,1);

		fo->connectWithDirektor(d);
		d->connectWithFrontOfficer(fo);

		auto timeHandle = tic();

		d->init1_SMP();  //init the simulation
		fo->init1_SMP(); //create my part of the scene
		d->init2_SMP();  //init the simulation
		fo->init2_SMP(); //populate the simulation
		d->init3_SMP();  //and render the first frame

		//no active waiting for Direktor's events, the respective
		//FOs' methods will be triggered directly from the Direktor
		d->execute();  //execute the simulation, and render frames

		d->close();    //close the simulation, deletes agents, and save tracks.txt
		fo->close();   //deletes my agents

		REPORT("simulation required " << toc(timeHandle));
#endif

		std::cout << "Happy end.\n\n";

		//calls also destructor for very final clean up
		if (fo != NULL) delete fo;
		if (d  != NULL) delete d;
		return (0);
	}
	catch (const char* e)
	{
		std::cout << "Got this message: " << e << "\n\n";
	}
	catch (std::string& e)
	{
		std::cout << "Got this message: " << e << "\n\n";
	}
	catch (std::runtime_error* e)
	{
		std::cout << "RuntimeError: " << e->what() << "\n\n";
	}
	catch (i3d::IOException* e)
	{
		std::cout << "i3d::IOException: " << e->what << "\n\n";
	}
	catch (i3d::LibException* e)
	{
		std::cout << "i3d::LibException: " << e->what << "\n\n";
	}
	catch (std::bad_alloc&)
	{
		std::cout << "Not enough memory.\n\n";
	}
	catch (...)
	{
		std::cout << "System exception.\n\n";
	}

	//calls destructor for very final clean up, which may also call
	//the respective close() methods if they had not been called before...
	if (fo != NULL)
	{
		REPORT("trying to close (and save the most from) the simulation's FO #" << fo->getID());
		delete fo;
	}
	if (d != NULL)
	{
		REPORT("trying to close (and save the most from) the simulation's Direktor");
		delete d;
	}
	return (1);
}