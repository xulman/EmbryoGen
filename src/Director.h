#ifndef DIREKTOR_H
#define DIREKTOR_H

#include "Scenarios/common/Scenario.h"
class FrontOfficer;

/** has access to Simulation, to reach its doPhaseIIandIII() */
class Director
{
public:
	Director(Scenario& s, const int allPortions)
		: scenario(s), FOsCount(allPortions)
	{}

protected:
	Scenario& scenario;

public:
	const int FOsCount;

	// ==================== simulation methods ====================
	// these are implemented in:
	// Director.cpp

	/** allocates output images, renders the first frame */
	/** scene heavy inits and synthoscopy warm up */
	void init(void)
	{
		REPORT("Direktor initializing now...");

		//a bit of stats before we start...
		const auto& sSum = scenario.params.constants.sceneSize;
		Vector3d<float> sSpx(sSum);
		sSpx.elemMult(scenario.params.constants.imgRes);
		REPORT("scenario suggests that scene size will be: "
		  << sSum.x << " x " << sSum.y << " x " << sSum.z
		  << " um -> "
		  << sSpx.x << " x " << sSpx.y << " x " << sSpx.z << " px");

		scenario.initializeScene();
		scenario.initializePhaseIIandIII();

		REPORT("Direktor initialized");
	}

	/** does the simulation loops, i.e. triggers calls of AbstractAgent's methods in the right order */
	void execute(void);

	/** frees simulation agents, writes the tracks.txt file */
	void close(void)
	{
		//mark before closing is attempted...
		isProperlyClosedFlag = true;

		//TODO
	}

	/** attempts to clean up, if not done earlier */
	~Director(void)
	{
		DEBUG_REPORT("Direktor already closed? " << (isProperlyClosedFlag ? "yes":"no"));
		if (!isProperlyClosedFlag) this->close();
	}

protected:
	/** flag to run-once the closing routines */
	bool isProperlyClosedFlag = false;

	// ==================== communication methods ====================
	// these are implemented in either exactly one of the two:
	// Communication/DirectorSMP.cpp
	// Communication/DirectorMPI.cpp

	void trigger_publishGeometry();
	void trigger_renderNextFrame();

#ifndef DISTRIBUTED
	FrontOfficer* FO = NULL;

public:
	void connectWithFrontOfficer(FrontOfficer* fo)
	{
		if (fo == NULL) ERROR_REPORT("Provided FrontOfficer is actually NULL.");
		FO = fo;
	}
#endif
};
#endif
