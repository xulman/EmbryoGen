#include <iostream>
#include <i3d/basic.h>
#include "Simulation.h"
#include "Scenarios/Scenarios.h"

int main(int argc, char** argv)
{
	Simulation* s = NULL;

	try
	{
		s = Scenarios(argc,argv).getSimulation();

		auto timeHandle = tic();

		s->init();    //init the simulation, and render the first frame
		s->execute(); //execute the simulation, and render frames
		s->close();   //close the simulation, deletes agents, and save tracks.txt

		REPORT("simulation required " << toc(timeHandle));

		std::cout << "Happy end.\n\n";

		delete s;     //calls also destructor for very final clean up
		return (0);
	}
	catch (const char *e)
	{
		std::cout << "Got this message: " << e << "\n\n";
	}
	catch (std::string &e)
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

	//calls destructor for very final clean up, it will also call
	//the Simulation::close() if it has not been called before...
	if (s != NULL)
	{
		REPORT("trying to close (and save the most from) the simulation");
		delete s;
	}
	return (1);
}
