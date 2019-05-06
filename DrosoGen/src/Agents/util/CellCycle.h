#ifndef CELLCYCLE_H
#define CELLCYCLE_H

#include "../../util/rnd_generators.h"

/** A datatype enumerating the particular phases of cell cycle in their order */
typedef enum
{
	newBorn=-1,
	G1Phase=0,
	SPhase=1,
	G2Phase=2,
	Prophase=3,
	Metaphase=4,
	Anaphase=5,
	Telophase=6,
	Cytokinesis=7,
	RestInPeace=8
} ListOfPhases;


/**
 * Utility class that governs proper cycling through a (normal) cell cycle.
 *
 * Derive this class while overriding the default methods for the basic eight
 * phases (aka phase_methods) of the cell cycle; instantiate this new class with
 * the desired full cycle duration via c'tors and the information about the
 * current time (mostly the time when this cell was born) via startCycling(),
 * and iteratively keep calling triggerCycleMethods(currentTime) that will
 * assure that the relevant phase_methods are called in the correct order.
 *
 * Each phase_method has one parameter (phase progress bar ratio) that informs
 * its content how far it should get within the phase; the ratio is always (0;1].
*/
class CellCycle
{
public:
	// --------------------------------------------------
	// construction & phase durations

	/** constructor with default 24h cell cycle */
	CellCycle():
		CellCycle( 24*60 ) {}

	/** a randomizing! no-exact-copy constructor */
	CellCycle(const CellCycle& refCellCycle, const float spreadFactor = 0.17f):
		CellCycle( GetRandomGauss(refCellCycle.fullCycleDuration,
		                          spreadFactor * refCellCycle.fullCycleDuration) ) {}

	/** constructor with (in minutes) given cycle length */
	CellCycle(const float _fullCycleDuration):
		fullCycleDuration(_fullCycleDuration) {}
		//NB: overriding determinePhaseDurations() are not-visible in the c'tor,
		//    hence, this call is postponed into startCycling(), and this method
		//    was made unavoidable as it is the only that provides the global time

	/** the (informative) cell cycle length [min] */
	const float fullCycleDuration;

protected:
	/** the length of individual cell cycle phases [min] */
	float phaseDurations[8] = {0,0,0,0,0,0,0,0};

	/** override-able pie-slicing of the fullCycleDuration, this
	    method is called from every constructor

	    the method must define durations (in minutes) of all eight
	    phases of this->phaseDurations[] and it must hold
	    afterwards that the sum of the individual durations is
	    exactly the value of this->fullCycleDuration */
	virtual
	void determinePhaseDurations(void)
	{
		setNormalPhaseDurations(fullCycleDuration, phaseDurations);
	}

public:
	/** read-only accessor to the lengths of the individual phases */
	float getPhaseDuration(const ListOfPhases phase) const
	{
		return phaseDurations[phase];
	}

	/** normal cell cycle pie-slicing, available for anyone...
	    note that the method follows rules set out for determinePhaseDurations() */
	static
	void setNormalPhaseDurations(const float fullCycleDuration,  //input
	                             float phaseDurations[8])        //output
	{
		DEBUG_REPORT("setting up phase durations for standard cell cycle of "
		             << fullCycleDuration << " mins");

		phaseDurations[G1Phase    ] = 0.5f     * fullCycleDuration;
		phaseDurations[SPhase     ] = 0.3f     * fullCycleDuration;
		phaseDurations[G2Phase    ] = 0.15f    * fullCycleDuration;
		phaseDurations[Prophase   ] = 0.0125f  * fullCycleDuration;
		phaseDurations[Metaphase  ] = 0.0285f  * fullCycleDuration;
		phaseDurations[Anaphase   ] = 0.0025f  * fullCycleDuration;
		phaseDurations[Telophase  ] = 0.00325f * fullCycleDuration;
		phaseDurations[Cytokinesis] = 0.00325f * fullCycleDuration;
		//                            --------
		//                    sums to 1.0f
	}

	/** (debug) report of the internal setting */
	void reportPhaseDurations(void) const
	{
		REPORT("full cycle duration is: " << fullCycleDuration << " minutes ("
		       << fullCycleDuration/60 << " hrs)");

		float phaseTotalTime = 0;
		for (int i=G1Phase; i <= Cytokinesis; ++i)
			phaseTotalTime += phaseDurations[i];
		REPORT("all cycles duration is: " << phaseTotalTime << " minutes");

		REPORT("  G1Phase    : " << phaseDurations[G1Phase    ] << " mins\t ("
		       << phaseDurations[G1Phase    ]/fullCycleDuration << "%)");
		REPORT("  SPhase     : " << phaseDurations[SPhase     ] << " mins\t ("
		       << phaseDurations[SPhase     ]/fullCycleDuration << "%)");
		REPORT("  G2Phase    : " << phaseDurations[G2Phase    ] << " mins\t ("
		       << phaseDurations[G2Phase    ]/fullCycleDuration << "%)");
		REPORT("  Prophase   : " << phaseDurations[Prophase   ] << " mins\t ("
		       << phaseDurations[Prophase   ]/fullCycleDuration << "%)");
		REPORT("  Metaphase  : " << phaseDurations[Metaphase  ] << " mins\t ("
		       << phaseDurations[Metaphase  ]/fullCycleDuration << "%)");
		REPORT("  Anaphase   : " << phaseDurations[Anaphase   ] << " mins\t ("
		       << phaseDurations[Anaphase   ]/fullCycleDuration << "%)");
		REPORT("  Telophase  : " << phaseDurations[Telophase  ] << " mins\t ("
		       << phaseDurations[Telophase  ]/fullCycleDuration << "%)");
		REPORT("  Cytokinesis: " << phaseDurations[Cytokinesis] << " mins\t ("
		       << phaseDurations[Cytokinesis]/fullCycleDuration << "%)");
	}

	// --------------------------------------------------
	// cycling through the cell cycle phases

protected:
	/** the currently being processed cycle phase */
	ListOfPhases curPhase = newBorn;

	/** the global time when the phase now in 'curPhase' has started */
	float lastPhaseChangeGlobalTime = -1;

public:
	/** this one actually starts the cycling, provide it with the
	    current "wall time" (in 'currentGlobalTime') */
	void startCycling(const float currentGlobalTime)
	{
		if (curPhase != newBorn)
			throw ERROR_REPORT("Re-initializing already initialized cell cycle");

		//define the phase durations
		determinePhaseDurations();
		//NB: overriding determinePhaseDurations() are not-visible in the c'tor,
		//    hence, this call is postponed into startCycling(), and this method
		//    was made unavoidable as it is the only that provides the global time

		lastPhaseChangeGlobalTime = currentGlobalTime;
		curPhase = G1Phase;
		startG1Phase();
	}

	/** this one calls the phase_methods (see docs of this class) in the correct order,
	    until the cycle progresses to the given "wall time" (in 'currentGlobalTime') */
	void triggerCycleMethods(const float currentGlobalTime)
	{
		if (curPhase == newBorn)
			throw ERROR_REPORT("Not yet fully initialized cell cycle");

		//beyond the cell cycle, do nothing
		if (curPhase == RestInPeace) return;

		bool tryNextPhase = true;
		while (tryNextPhase)
		{
			const float progress = (currentGlobalTime - lastPhaseChangeGlobalTime)
			                     / phaseDurations[curPhase];
			DEBUG_REPORT("progress=" << progress);

			if (progress < 0)
			{
				throw ERROR_REPORT("lastPhaseChange is ahead given currentGlobalTime, quitting confusedly");
			}
			else if (progress == 0)
			{
				//there's really no progress required from us, just skip this call
				DEBUG_REPORT("skipping this call...");
				tryNextPhase = false;
			}
			else if (progress < 1)
			{
				//call normally as some progress is required
				switch (curPhase)
				{
				case G1Phase:
					runG1Phase(progress);
					break;

				case SPhase:
					runSPhase(progress);
					break;

				case G2Phase:
					runG2Phase(progress);
					break;

				case Prophase:
					runProphase(progress);
					break;

				case Metaphase:
					runMetaphase(progress);
					break;

				case Anaphase:
					runAnaphase(progress);
					break;

				case Telophase:
					runTelophase(progress);
					break;

				case Cytokinesis:
					runCytokinesis(progress);
					break;

				default:
					throw ERROR_REPORT("Unknown cycle state!");
				}

				//couldn't cross the phase boundaries, yet has done its portion of the work
				tryNextPhase = false;
			}
			else
			{
				//got over a boundary between phases, then
				//call normally to finish to reach 1.0, close, incr. curPhase, open
				switch (curPhase)
				{
				case G1Phase:
					runG1Phase(1.0);
					closeG1Phase();
					//
					lastPhaseChangeGlobalTime += phaseDurations[curPhase];
					curPhase = SPhase;
					startSPhase();
					break;

				case SPhase:
					runSPhase(1.0);
					closeSPhase();
					//
					lastPhaseChangeGlobalTime += phaseDurations[curPhase];
					curPhase = G2Phase;
					startG2Phase();
					break;

				case G2Phase:
					runG2Phase(1.0);
					closeG2Phase();
					//
					lastPhaseChangeGlobalTime += phaseDurations[curPhase];
					curPhase = Prophase;
					startProphase();
					break;

				case Prophase:
					runProphase(1.0);
					closeProphase();
					//
					lastPhaseChangeGlobalTime += phaseDurations[curPhase];
					curPhase = Metaphase;
					startMetaphase();
					break;

				case Metaphase:
					runMetaphase(1.0);
					closeMetaphase();
					//
					lastPhaseChangeGlobalTime += phaseDurations[curPhase];
					curPhase = Anaphase;
					startAnaphase();
					break;

				case Anaphase:
					runAnaphase(1.0);
					closeAnaphase();
					//
					lastPhaseChangeGlobalTime += phaseDurations[curPhase];
					curPhase = Telophase;
					startTelophase();
					break;

				case Telophase:
					runTelophase(1.0);
					closeTelophase();
					//
					lastPhaseChangeGlobalTime += phaseDurations[curPhase];
					curPhase = Cytokinesis;
					startCytokinesis();
					break;

				case Cytokinesis:
					runCytokinesis(1.0);
					closeCytokinesis();
					//
					lastPhaseChangeGlobalTime += phaseDurations[curPhase];
					curPhase = RestInPeace;
					break;

				default:
					throw ERROR_REPORT("Unknown cycle state!");
				}

				//it might, however, still not be the end of this:
				//if lastPhaseChangeGlobalTime is still "multiple" whole phases
				//beyond the given currentGlobalTime, we still need "multiple" iterations
			}
		} //end of while (tryNextPhase)
	}    //end of the method triggerCycleMethods()

	// --------------------------------------------------
	// the phase_methods

	virtual void startG1Phase(void) {}
	virtual void runG1Phase(const float) {}
	virtual void closeG1Phase(void) {}

	virtual void startSPhase(void) {}
	virtual void runSPhase(const float) {}
	virtual void closeSPhase(void) {}

	virtual void startG2Phase(void) {}
	virtual void runG2Phase(const float) {}
	virtual void closeG2Phase(void) {}

	virtual void startProphase(void) {}
	virtual void runProphase(const float) {}
	virtual void closeProphase(void) {}

	virtual void startMetaphase(void) {}
	virtual void runMetaphase(const float) {}
	virtual void closeMetaphase(void) {}

	virtual void startAnaphase(void) {}
	virtual void runAnaphase(const float) {}
	virtual void closeAnaphase(void) {}

	virtual void startTelophase(void) {}
	virtual void runTelophase(const float) {}
	virtual void closeTelophase(void) {}

	virtual void startCytokinesis(void) {}
	virtual void runCytokinesis(const float) {}
	virtual void closeCytokinesis(void) {}
};
#endif
