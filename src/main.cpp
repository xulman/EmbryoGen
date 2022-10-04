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

	Scenarios scenarios(argc, const_cast<const char**>(argv));

	// the Scenario object paradigm: there's always (independent) one per
	// Direktor and each FO; thus, it is always created inline in respective
	// c'tor calls (scenarios.getScenario() returns copy as an unique_ptr)

	auto timeHandle = tic();
	{
		Director d([&]() { return scenarios.getScenario(); });

		d.init();
		// no active waiting for Director's events, the respective
		// FOs' methods will be triggered directly from the Director
		d.execute(); // execute the simulation, and render frames
	}

	report::message(fmt::format("simulation required {}", toc(timeHandle)));
	report::message("Happy end.\n");
}
