#include "Director.hpp"
#include "FrontOfficer.hpp"
#include "Scenarios/common/Scenario.hpp"
#include "Scenarios/common/Scenarios.hpp"
#include "git_hash.hpp"
#include <i3d/basic.h>
#include <iostream>

int main(int argc, char** argv) {
	report::message(
	    fmt::format("This is EmbryoGen at commit rev {}.", gitCommitHash),
	    {false});

	std::unique_ptr<Director> d;
	std::unique_ptr<FrontOfficer> fo;
	Scenarios scenarios(argc, const_cast<const char**>(argv));

	// the Scenario object paradigm: there's always (independent) one per
	// Direktor and each FO; thus, it is always created inline in respective
	// c'tor calls (scenarios.getScenario() returns reference to copy)

	// single machine case
	d = std::make_unique<Director>(scenarios.getScenario(), 1, 1);
	fo = std::make_unique<FrontOfficer>(scenarios.getScenario(), 0, 1, 1);

	fo->connectWithDirektor(d.get());
	d->connectWithFrontOfficer(fo.get());

	auto timeHandle = tic();

	d->init1_SMP();  // init the simulation
	fo->init1_SMP(); // create my part of the scene
	d->init2_SMP();  // init the simulation
	fo->init2_SMP(); // populate the simulation
	d->init3_SMP();  // and render the first frame

	// no active waiting for Direktor's events, the respective
	// FOs' methods will be triggered directly from the Direktor
	d->execute(); // execute the simulation, and render frames

	d->close(); // close the simulation, deletes agents, and save tracks.txt
	fo->close(); // deletes my agents

	report::message(fmt::format("simulation required {}", toc(timeHandle)));
	std::cout << "Happy end.\n\n";
}
