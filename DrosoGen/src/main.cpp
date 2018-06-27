#include "Simulation.h"

int main(void)
{
	Simulation s;    //init and render the first frame
	s.execute();     //execute the simulation
	return(0);       //closes the simulation (as a side effect)
}
