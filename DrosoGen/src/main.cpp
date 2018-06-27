#include <iostream>
#include "Simulation.h"

int main(void)
{
	Simulation s;

	try
	{
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
