#include "Communication/DistributedCommunicator.hpp"
#include "Director.hpp"
#include "FrontOfficer.hpp"
#include "Scenarios/common/Scenario.hpp"
#include "Scenarios/common/Scenarios.hpp"
#include "git_hash.hpp"
#include <i3d/basic.h>
#include <iostream>

#ifdef DISTRIBUTED
#define REPORT_EXCEPTION(x)                                                    \
	report::message(                                                           \
	    fmt::format("Broadcasting the following expection:\n{}\n\n", x));      \
	if (d)                                                                     \
		d->broadcast_throwException(fmt::format("Outside exception: {}", x));  \
	if (fo)                                                                    \
		fo->broadcast_throwException(fmt::format("Outside exception: {}", x));
#else
#define REPORT_EXCEPTION(x) report::message(fmt::format("{}\n\n", x));
#endif

int main(int argc, char** argv) {
	report::message(
	    fmt::format("This is EmbryoGen at commit rev {}.", gitCommitHash),
	    {false});

	std::unique_ptr<Director> d;
	std::unique_ptr<FrontOfficer> fo;
	Scenarios scenarios(argc, const_cast<const char**>(argv));

	try {
		// the Scenario object paradigm: there's always (independent) one per
		// Direktor and each FO; thus, it is always created inline in respective
		// c'tor calls (scenarios.getScenario() returns reference to copy)

#ifdef DISTRIBUTED
		DistributedCommunicator* dc = new MPI_Communicator(argc, argv);

		// these two has to come from MPI stack,
		// for now some fake values:
		const int MPI_IDOfThisInstance = dc->getInstanceID();
		const int MPI_noOfNodesInTotal = dc->getNumberOfNodes();

		// this is assuming that MPI clients' IDs form a full
		// integer interval between 0 and MPI_noOfNodesInTotal-1,
		// we further consider 0 to be the Direktor's node,
		// and 1 till MPI_noOfNodesInTotal-1 for IDs for the FOs

		if (MPI_IDOfThisInstance == 0) {
			// hoho, I'm the Direktor  (NB: Direktor == main simulation loop)
			d = std::make_unique<Director>(scenarios.getScenario(), 1,
			                               MPI_noOfNodesInTotal - 1, dc);
			d->initMPI(); // init the simulation, and render the first frame
			d->execute(); // execute the simulation, and render frames
			d->close();   // close the simulation, deletes agents, and save
			              // tracks.txt
		} else {
			// I'm FrontOfficer:
			int thisFOsID = MPI_IDOfThisInstance;
			int nextFOsID = thisFOsID + 1; // builds the round robin schema

			// tell the last FO to send data back to the Direktor
			if (nextFOsID == MPI_noOfNodesInTotal)
				nextFOsID = 0;

			fo = std::make_unique<FrontOfficer>(scenarios.getScenario(),
			                                    nextFOsID, MPI_IDOfThisInstance,
			                                    MPI_noOfNodesInTotal - 1, dc);
			fo->initMPI(); // populate/create my part of the scene
			report::message(
			    fmt::format("Multi node case, init MPI: {} nodes, instance {}",
			                MPI_noOfNodesInTotal, MPI_IDOfThisInstance));
			fo->execute(); // wait for Direktor's events
			fo->close();   // deletes my agents
		}
#else
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
#endif

		std::cout << "Happy end.\n\n";

		return 0;

	} catch (const char* e) {
		REPORT_EXCEPTION(fmt::format("Got this message: {}", e));
	} catch (std::string& e) {
		REPORT_EXCEPTION(fmt::format("Got this message: {}", e));
	} catch (std::runtime_error* e) {
		REPORT_EXCEPTION(fmt::format("RuntimeError: {}", e->what()));
	} catch (i3d::IOException* e) {
		REPORT_EXCEPTION(fmt::format("i3d::IOException: {}", e->what));
	} catch (const i3d::IOException& e) {
		REPORT_EXCEPTION(fmt::format("i3d::IOException: {}", e.what));
	} catch (i3d::LibException* e) {
		REPORT_EXCEPTION(fmt::format("i3d::LibException: {}", e->what));
	} catch (std::bad_alloc&) {
		REPORT_EXCEPTION("Not enough memory.");
	}

	/*
	catch (...) {
		REPORT_EXCEPTION("System exception.");
	}
	*/
	

	return 1;
}
