#ifndef SCENARIOS_H
#define SCENARIOS_H

#include "../Simulation.h"

#define CLASS_DECLARATION : public Simulation { void initializeAgents(void) override; };
#define AVAILABLE_SCENARIO(n,c) \
	availableScenarios.emplace_back(std::string((n)));                         \
	if (simulation == NULL &&                                                  \
	    availableScenarios.back().find(argv[1]) != std::string::npos)          \
	{                                                                          \
		REPORT("Going to use the scenario: " << availableScenarios.back());     \
		simulation = new (c)();                                                 \
	}


//    regular new class name             just copy this
class Scenario_AFewAgents                CLASS_DECLARATION
class Scenario_DrosophilaRegular         CLASS_DECLARATION
class Scenario_DrosophilaRandom          CLASS_DECLARATION
class Scenario_pseudoDivision            CLASS_DECLARATION
class Scenario_dragAndRotate             CLASS_DECLARATION
class Scenario_withCellCycle             CLASS_DECLARATION
//  ---> ADD NEW SCENARIO HERE (and bellow) <---

class Scenarios
{
private:
	/** list of system-recognized scenarios, actually simulations */
	std::list<std::string> availableScenarios;

	/** the scenario/simulation that is going to be used */
	Simulation* simulation = NULL;

public:
	/** find the right scenario to initialize the simulation with */
	Scenarios(int argc, char** argv)
	{
		//initiates this->registeredScenarios, defined in Scenario.h

		if (argc == 1)
		{
			//no args given, not even a scenario... going for default one
			REPORT("Going to use the default scenario: regularDrosophila");
			simulation = new Scenario_DrosophilaRegular();
		}
		else
		{
			//NB: argc > 1
			//
			//                   nickname            regular class name
			AVAILABLE_SCENARIO( "aFewAgents",        Scenario_AFewAgents )
			AVAILABLE_SCENARIO( "regularDrosophila", Scenario_DrosophilaRegular )
			AVAILABLE_SCENARIO( "randomDrosophila",  Scenario_DrosophilaRandom )
			AVAILABLE_SCENARIO( "pseudoDivision",    Scenario_pseudoDivision )
			AVAILABLE_SCENARIO( "dragAndRotate",     Scenario_dragAndRotate )
			AVAILABLE_SCENARIO( "cellCycle",         Scenario_withCellCycle )
			//  ---> ADD NEW SCENARIO HERE (and add definition *cpp file, re-run cmake!) <---

			if (simulation == NULL)
			{
				REPORT("Couldn't match command-line scenario '" << argv[1] << "' to any known scenario.");
				std::cout << "Currently known scenarios are: ";
				for (auto rs : availableScenarios)
					std::cout << rs << ", ";
				std::cout << "\n";

				throw ERROR_REPORT("Unmatched scenario requested.");
			}
		}

		//pass the CLI params inside, to be
		//possibly considered by initializeAgents()
		simulation->setArgs(argc,argv);
	}


	Simulation* getSimulation(void)
	{
		return simulation;
	}
};
#endif
