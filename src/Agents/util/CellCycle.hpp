#pragma once

#include "../../util/report.hpp"
#include <fmt/core.h>

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

	// --------------------------------------------------
	// construction & phase durations

	/** constructor with default 24 hrs cell cycle */
	CellCycle()
		: CellCycle( 24*60 ) {}

	/** an exact-copy constructor - useful when two instances that act exactly the same
	    are used in one agent (e.g. one cycle to control geometry changes, one cycle
	    to control texture development); note that one has to provide the second
	    parameter (with any value) to reach out for this particular c'tor */
	CellCycle(const CellCycle& refCellCycle)
		: CellCycle( refCellCycle.fullCycleDuration ) {}

	/** a randomizing! no-exact-copy constructor, nice default is 'spreadFactor' = 0.17 */
	CellCycle(const CellCycle& refCellCycle, const float spreadFactor)
		: CellCycle( RandomizeCellCycleDuration(refCellCycle.fullCycleDuration, spreadFactor) ) {}

	/** a randomizing! constructor with given basis cycle duration (in minutes),
	    nice default is 'spreadFactor' = 0.17 */
	CellCycle(const float refCycleDuration, const float spreadFactor)
		: CellCycle( RandomizeCellCycleDuration(refCycleDuration, spreadFactor) ) {}

	/** constructor with given exact cycle duration (in minutes) */
	CellCycle(const float _fullCycleDuration)
		: fullCycleDuration(_fullCycleDuration) {}
		//NB: overriding determinePhaseDurations() are not-visible in the c'tor,
		//    hence, this call is postponed into startCycling() -- and this method
		//    was made unavoidable as it is the only that provides the global time

	/** the (informative) cell cycle duration [min] */
	const float fullCycleDuration;

protected:
	float RandomizeCellCycleDuration(const float refDuration, const float spreadFactor);

	/** the durations of individual cell cycle phases [min] */
	float phaseDurations[8] = {0,0,0,0,0,0,0,0};

	/** Override-able pie-slicing of the fullCycleDuration, this
	    method is called from the startCycling() method.

	    The method must define durations (in minutes) of all eight
	    phases of this->phaseDurations[] and it must hold
	    afterwards that the sum of the individual durations is
	    exactly the value of this->fullCycleDuration. */
	virtual
	void determinePhaseDurations(void)
	{
		setNormalPhaseDurations(fullCycleDuration, phaseDurations);
	}

public:
	/** read-only accessor to the durations of the individual phases */
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
report::debugMessage(fmt::format("setting up phase durations for standard cell cycle of {} mins" , fullCycleDuration));

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
	void reportPhaseDurations(void) const;

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
	void startCycling(const float currentGlobalTime);

	/** this one calls the phase_methods (see docs of this class) in the correct order,
	    until the cycle progresses to the given "wall time" (in 'currentGlobalTime'),
	    it makes sure that no run_phase_method is called with zero progress ratio */
	void triggerCycleMethods(const float currentGlobalTime);

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
