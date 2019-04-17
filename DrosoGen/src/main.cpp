#include <iostream>
#include <i3d/basic.h>
#include "Simulation.h"
#include "Scenarios/Scenarios.h"

int main(int argc, char** argv)
{
	try
	{
		Simulation& s = Scenarios(argc,argv).getSimulation();

		s.init();    //init the simulation, and render the first frame
		s.execute(); //execute the simulation, and render frames
		s.close();   //close the simulation, and save tracks.txt

		std::cout << "Happy end.\n";
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

	//also attempts to save tracks.txt
	return (1);
}
