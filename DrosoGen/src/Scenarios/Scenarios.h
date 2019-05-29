#ifndef SCENARIOS_H
#define SCENARIOS_H

#include "../Simulation.h"

#define CLASS_DECLARATION : public Simulation {                               \
	void initializeScenario(void) override; };
//
#define CLASS_DECLARATION_WithOwnSynthoscopy : public Simulation {            \
	void initializeScenario(void) override; void doPhaseIIandIII(void) override; };

#define AVAILABLE_SCENARIO(n,c) \
	availableScenarios.emplace_back(std::string((n)));                         \
	if (argc > 1 && simulation == NULL &&                                      \
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
class Scenario_withTexture               CLASS_DECLARATION_WithOwnSynthoscopy
class Scenario_dragRotateAndTexture      CLASS_DECLARATION
class Scenario_phaseIIandIII             CLASS_DECLARATION
class Scenario_PerlinShowCase            CLASS_DECLARATION
//  ---> ADD NEW SCENARIO HERE AND ALSO BELOW <---

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
		//                   nickname            regular class name
		AVAILABLE_SCENARIO( "aFewAgents",        Scenario_AFewAgents )
		AVAILABLE_SCENARIO( "regularDrosophila", Scenario_DrosophilaRegular )
		AVAILABLE_SCENARIO( "randomDrosophila",  Scenario_DrosophilaRandom )
		AVAILABLE_SCENARIO( "pseudoDivision",    Scenario_pseudoDivision )
		AVAILABLE_SCENARIO( "dragAndRotate",     Scenario_dragAndRotate )
		AVAILABLE_SCENARIO( "cellCycle",         Scenario_withCellCycle )
		AVAILABLE_SCENARIO( "fluoTexture",       Scenario_withTexture )
		AVAILABLE_SCENARIO( "dragFluoTexture",   Scenario_dragRotateAndTexture )
		AVAILABLE_SCENARIO( "synthoscopy",       Scenario_phaseIIandIII )
		AVAILABLE_SCENARIO( "PerlinShowCase",    Scenario_PerlinShowCase )
		//  ---> ADD NEW SCENARIO HERE (and add definition *cpp file, re-run cmake!) <---

		if (simulation == NULL)
		{
			if (argc == 1)
			{
				REPORT("Please run again and choose some of the available scenarios, e.g. as");
				REPORT(argv[0] << " regularDrosophila");
			}
			else
				REPORT("Couldn't match command-line scenario '" << argv[1] << "' to any known scenario.");

			std::cout << "Currently known scenarios are: ";
			for (auto rs : availableScenarios)
				std::cout << rs << ", ";
			std::cout << "\n";

			throw ERROR_REPORT("Unmatched scenario.");
		}

		//pass the CLI params inside, to be
		//possibly considered by initializeScenario()
		simulation->setArgs(argc,argv);
	}


	Simulation* getSimulation(void)
	{
		return simulation;
	}
};
#endif
