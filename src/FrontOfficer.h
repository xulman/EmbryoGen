#ifndef FRONTOFFICER_H
#define FRONTOFFICER_H

#include "Scenarios/common/Scenario.h"
class Director;

/** has access to Simulation, to reach its initializeAgents() */
class FrontOfficer//: public Simulation
{
public:
	FrontOfficer(Scenario& s, const int nextFO, const int myPortion, const int allPortions)
		: scenario(s), ID(myPortion), nextFOsID(nextFO), FOsCount(allPortions)
	{}

protected:
	Scenario& scenario;

public:
	const int ID, nextFOsID, FOsCount;

	/** good old getter for otherwise const'ed public ID */
	int getID() { return ID; }

	// ==================== simulation methods ====================
	// these are implemented in:
	// FrontOfficer.cpp

	/** scene heavy inits and adds agents */
	void init(void)
	{
		REPORT("FO #" << ID << " initializing now...");

		scenario.initializeScene();
		scenario.initializeAgents(this,ID,FOsCount);

		REPORT("FO #" << ID << " initialized");
	}

	/** does the simulation loops, i.e. calls AbstractAgent's methods */
	void execute(void);

	/** frees simulation agents */
	void close(void)
	{
		//mark before closing is attempted...
		isProperlyClosedFlag = true;

		//TODO
	}

	/** attempts to clean up, if not done earlier */
	~FrontOfficer(void)
	{
		DEBUG_REPORT("FrontOfficer #" << ID << " already closed? " << (isProperlyClosedFlag ? "yes":"no"));
		if (!isProperlyClosedFlag) this->close();
	}

protected:
	/** flag to run-once the closing routines */
	bool isProperlyClosedFlag = false;

	// ==================== communication methods ====================
	// these are implemented in either exactly one of the two:
	// Communication/FrontOfficerSMP.cpp
	// Communication/FrontOfficerMPI.cpp

	void triggered_publishGeometry();
	void triggered_renderNextFrame();

#ifndef DISTRIBUTED
	Director* Direktor = NULL;

public:
	void connectWithDirektor(Director* d)
	{
		if (d == NULL) ERROR_REPORT("Provided Director is actually NULL.");
		Direktor = d;
	}
#endif
};
#endif
