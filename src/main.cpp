#include <iostream>
#include <i3d/basic.h>
#include "Communication/DistributedCommunicator.h"
#include "Scenarios/common/Scenario.h"
#include "Scenarios/common/Scenarios.h"
#include "Director.h"
#include "FrontOfficer.h"

#ifdef DISTRIBUTED
	#define REPORT_EXCEPTION(x) \
		REPORT("Broadcasting the following expection:"); \
		std::cout << x << "\n\n"; \
		if ( d != NULL)  d->broadcast_throwException(buildStringFromStream("Outside exception: " << x).c_str()); \
		if (fo != NULL) fo->broadcast_throwException(buildStringFromStream("Outside exception: " << x).c_str());
#else
	#define REPORT_EXCEPTION(x) \
		std::cout << x << "\n\n";
#endif

int main(int argc, char** argv)
{
	Director* d = NULL;
	FrontOfficer* fo = NULL;

	try
	{
		//the Scenario object paradigm: there's always (independent) one per Direktor
		//and each FO; thus, it is always created inline in respective c'tor calls

#ifdef DISTRIBUTED
		DistributedCommunicator * dc = new MPI_Communicator(argc, argv);

		//these two has to come from MPI stack,
		//for now some fake values:
                const int MPI_IDOfThisInstance = dc->getInstanceID();
                const int MPI_noOfNodesInTotal = dc->getNumberOfNodes();

		//this is assuming that MPI clients' IDs form a full
		//integer interval between 0 and MPI_noOfNodesInTotal-1,
		//we further consider 0 to be the Direktor's node,
		//and 1 till MPI_noOfNodesInTotal-1 for IDs for the FOs

		if (MPI_IDOfThisInstance == 0)
		{
			//hoho, I'm the Direktor  (NB: Direktor == main simulation loop)
			d = new Director(Scenarios(argc,argv).getScenario(), 1,MPI_noOfNodesInTotal-1, dc);
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

			fo = new FrontOfficer(Scenarios(argc,argv).getScenario(), nextFOsID, MPI_IDOfThisInstance,MPI_noOfNodesInTotal-1, dc);
			fo->initMPI(); //populate/create my part of the scene
			REPORT("Multi node case, init MPI: " << MPI_noOfNodesInTotal << " nodes, instance " << MPI_IDOfThisInstance);
			fo->execute(); //wait for Direktor's events
			fo->close();   //deletes my agents
		}
#else
		//single machine case
		d  = new     Director(Scenarios(argc,argv).getScenario(), 1,1);
		fo = new FrontOfficer(Scenarios(argc,argv).getScenario(), 0, 1,1);

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
		REPORT_EXCEPTION("Got this message: " << e)
	}
	catch (std::string& e)
	{
		REPORT_EXCEPTION("Got this message: " << e)
	}
	catch (std::runtime_error* e)
	{
		REPORT_EXCEPTION("RuntimeError: " << e->what())
	}
	catch (i3d::IOException* e)
	{
		REPORT_EXCEPTION("i3d::IOException: " << e->what)
	}
	catch (i3d::LibException* e)
	{
		REPORT_EXCEPTION("i3d::LibException: " << e->what)
	}
	catch (std::bad_alloc&)
	{
		REPORT_EXCEPTION("Not enough memory.")
	}
	catch (...)
	{
		REPORT_EXCEPTION("System exception.")
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
