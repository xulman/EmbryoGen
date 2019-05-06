#ifndef CELLCYCLE_H
#define CELLCYCLE_H

#include "../../util/rnd_generators.h"

/** A datatype enumerating the particular phases of cell cycle in their order */
typedef enum
{
	G1Phase=0,
	SPhase=1,
	G2Phase=2,
	Prophase=3,
	Metaphase=4,
	Anaphase=5,
	Telophase=6,
	Cytokinesis=7
} ListOfPhases;


/**
 * Utility class that governs proper cycling through a (normal) cell cycle.
 *
 * Derive this class while overriding the default methods for
 * the basic 8 phases (aka phase_methods) of the cell cycle; instantiate
 * this new class with the desired full cycle duration and the information
 * about the current time (essentially the time when this cell was born),
 * and iteratively keep calling doNextPhase(currentTime) that will assure
 * that the relevant phase_methods are called in the correct order.
 *
 * Each phase_method has one parameter (phase progress bar ratio) that informs
 * its content how far it should get within the phase; the ratio is always (0;1].
*/
class CellCycle
{
public:
	/** constructor with default 24h cell cycle */
	CellCycle():
		CellCycle( 24*60 ) {}

	/** a randomizing! no-exact-copy constructor */
	CellCycle(const CellCycle& refCellCycle, const float spreadFactor = 0.17f):
		CellCycle( GetRandomGauss(refCellCycle.fullCycleDuration,
		                          spreadFactor * refCellCycle.fullCycleDuration) ) {}

	/** constructor with (in minutes) given cycle length */
	CellCycle(const float _fullCycleDuration):
		fullCycleDuration(_fullCycleDuration)
	{
		determinePhaseDurations();
	}

	/** the (informative) cell cycle length [min] */
	const float fullCycleDuration;

protected:
	/** the length of individual cell cycle phases [min] */
	float phaseDurations[8];

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
};
#endif
