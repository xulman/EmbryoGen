#ifndef AGENTS_H
#define AGENTS_H

#include <list>
#include <vector>
#include <i3d/image3d.h>
#include "params.h"
#include "rnd_generators.h"
#include "DisplayUnits/DisplayUnit.h"


//generic representant of a single cell
class Cell {
 public:
 	///default constructor
	Cell(void) : pos(0.f), orientation(0.f,1.f), bp(), former_bp(),
			weight(1.f), initial_bp(), listOfForces(),
			isSelected(false), listOfFriends()
	{
		colour.r=1.f;
		colour.g=1.f;
		colour.b=0.f;

		//default shape
		bp.push_back(PolarPair(0.785f,3.f));
		bp.push_back(PolarPair(2.356f,3.f));
		bp.push_back(PolarPair(3.927f,3.f));
		bp.push_back(PolarPair(5.498f,3.f));
		outerRadius=4.f;

		//back up the initial settings
		initial_bp=bp;
		initial_weight=weight;

		InitialSettings();
	}

	///copy constructor
	Cell(const Cell& buddy,
		const float R=1.f, const float G=1.f, const float B=0.f);

	///ID of this cell
	int ID;

	///destructor
	~Cell(void)
	{
		bp.clear();
		former_bp.clear();
		initial_bp.clear();
		listOfForces.clear();
		listOfFriends.clear();

		//signal UNdrawing of this cell...:
		//negative radius is an agreed signal to remove the cell
		if (displayUnit) displayUnit->DrawPoint(ID,pos,-1.0f);
	}

	DisplayUnit* displayUnit = NULL;

// -------- current params of the cell -------- 
	///position of the centre of the cell, angles in radians, distances in microns (um)
	Vector3d<float> pos;

	///vector showing from the centre towards "head" of the cell
	PolarPair orientation;

	///list of bp (boundary points) -- shape of the cell
	std::list<PolarPair> bp;

	///radius of circumscribed circle [um]
	float outerRadius;

	///former situation of the cell
	Vector3d<float> former_pos;
	std::list<PolarPair> former_bp;

	 /**
	  * updates the position rigidly, updates
	  * Cell::pos, Cell::orientation, Cell::bp
	  */
    void calculateNewPosition(const Vector3d<float>& translation,float rotation);

// -------- constant params of the cell -------- 
	///weight of the cell [mg]
	float weight;

    ///persistence of motion time [min] -> controls how often change velocity and direction
    float persistenceTime;
	 ///settle down time [min] -> controls how quickly cell adapts to the new situation
    float settleTime;

	///back up of the initial setting: bp list
	std::list<PolarPair> initial_bp;

	///back up of the initial vector showing from the centre towards "head" of the cell
	PolarPair initial_orientation;

	///back up of the initial setting: weight
	float initial_weight;

// -------- current development of the cell (main functions) -------- 
	 /**
	  * simulate shape change after the \e timeDelta time
	  *
	  * This function, ideally, should project its own will or need
	  * into a form of forces acting on its boundary... but this is a future from now
	  *
	  * Now, it only determines its new desired velocity and governs
	  * calling of the cell-cycle-phase functions below.
	  */
	 void adjustShape(const float timeDelta);
	 //since calculateNewPosition() should be called at once per simulation tick,
	 //we must forward the relevant info to some later stage
	 float rotateBy;

	 ///currently exhibited cell phase
	 ListOfPhases curPhase;
	 ///global simulated time when the last change occurred [min]
	 float lastPhaseChange;
	 ///global simulated time when the next change should occur [min]
	 float nextPhaseChange;
	 ///value between [0,1] reporting on the progress within the current phase
	 float phaseProgress;

	 ///updates Cell::curPhase and Cell::nextPhaseChange appropriately
	 void rotatePhase(void);

	 ///phase realizing functions: merely only change shape
	 ///initialize: get how long the phase is going to last
	 void initializeG1Phase(const float duration);
	 void initializeSPhase(const float duration);
	 void initializeG2Phase(const float duration);
	 void initializeProphase(const float duration);
	 void initializeMetaphase(const float duration);
	 void initializeAnaphase(const float duration);
	 void initializeTelophase(const float duration);
	 void initializeCytokinesis(const float duration);

	 ///progress: develop forward within the phase,
	 ///Cell::phaseProgress tells how far we are
	 void progressG1Phase(void);
	 void progressSPhase(void);
	 void progressG2Phase(void);
	 void progressProphase(void);
	 void progressMetaphase(void);
	 void progressAnaphase(void);
	 void progressTelophase(void);
	 void progressCytokinesis(void);

	 ///finalize: cleanup after the phase is over,
	 ///must make sure the final shape is acquired
	 void finalizeG1Phase(void);
	 void finalizeSPhase(void);
	 void finalizeG2Phase(void);
	 void finalizeProphase(void);
	 void finalizeMetaphase(void);
	 void finalizeAnaphase(void);
	 void finalizeTelophase(void);
	 void finalizeCytokinesis(void);

// -------- current development of the cell (helper functions) -------- 
	 /// one step of Cytokinesis
	 void doCytokinesis(const float progress);

	 /** 
	  * given one boundary point, search its neighbourhood and find the point
	  * that is the nearest to the cell center
	  */
	 void FindNearest(std::list<PolarPair>::iterator&,
							std::list<PolarPair>::iterator&,
							std::list<PolarPair>::iterator&,
							float angleTolerance);

	///finds boundary point whose azimuth is closest to the \e angle,
	///and returns the absolute angular distance
	float FindNearest(const float angle,
							std::list<PolarPair>::iterator& p);

	 /**
	  * Find two point that forms minor axis - axis perpendicular the main
	  * direction of the cell
	  */
	 void FindMinorAxis(std::list<PolarPair>::iterator&,
							  std::list<PolarPair>::iterator&);

	 /// normalize the angle
	 float Normalize(float angle);

	 /// measure the cell diameter (along one specified direction) - v tehle funkci je asi bug :-(
	 float GetCellDiameter(PolarPair &a, PolarPair &b);

	///helper variables to aid telophase and cytokinesis
	std::list<PolarPair>::iterator referenceBP;
	float referenceBP_base, referenceBP_rate;

	 ///helper variables to aid G1 growth
    int grow_iterations;
    int cur_grow_it;
    int p_iter;
    void Grow(void);

// -------- current movement of the cell -------- 
	 ///currently acting forces [N]
    std::vector<ForceVector3d<float> > listOfForces;
	 ///sum of the currently acting forces [N]
	 ForceVector3d<float> force;

	 ///last sustainable stress situation
	 float lastEasyForcesTime;

    ///current acceleration [um/min^2]
    Vector3d<float> acceleration;

    ///current velocity of the cell [um/min]
    Vector3d<float> velocity;

    ///desired direction azimuth [in radians] of the cell
    float desiredDirection;
    //desired speed of the cell [um/min]
    float desiredSpeed;

	 ///quasi-periodically changes (ala random-walk) the desired direction of a cell 
    void ChangeDirection(const float timeDelta);
	 ///length of the period [min]
    float directionChangeInterval;
    ///progress within the period [min]
    float directionChangeCounter;
	 ///the last direction change was this one:
	 float directionChangeLastAngle;

// -------- visualization stuff -------- 
	///RGB colour of the cell, range is 0 from 1
	struct {
		float r,g,b;
	} colour;

	///are we focused on this particular cell?
	bool isSelected;

	///do we want to remove this cell?
	bool shouldDie;

	///tells whether given coordinate is inside this cell
	bool IsPointInCell(const Vector3d<float>& coord);

	///draws the cell and forces with OpenGL commands
	/*
	void DrawCell(const int showCell) const;
	void DrawForces(const int whichForces,const float stretchF=1.f,
	                const float stretchV=1.f) const;
	*/

	//renders the sphere into this image
	void RasterInto(i3d::Image3d<i3d::GRAY8>& img) const;

	//renders the sphere into this display unit
	void DrawIntoDisplayUnit(void) const;

	//only forwards the flush request to the underlying/registered display unit
	void FlushDisplayUnit(void) const
	{ if (displayUnit) displayUnit->Flush(); }


	///lists content of the bp list
	void ListBPs(void) const;

	///lists some internal attributes
	void ReportState(void) const;

// -------- simulation stuff -------- 
    void InitialSettings(const bool G1start=false);

	 /**
	  * rough (but fast) under-estimation of the distance
	  * between this cell and the \e buddy
	  *
	  * basically: d_ij - (r_i + r_j)
	  */
	 float CalcRadiusDistance(const Cell& buddy);

    ///lambda to calculate social force
    float lambda;
    ///A,B parameters for social force
    float A,B;
	 ///repulsive force from a cell located at (\e ang,\e dist) polar coordinate from me
    void calculateSocialForce(const float ang, const float dist);

	 ///weight factor for the attraction force
	 float C;
	 ///sensing distance between cells [microns]
	 float delta_f;
	 /**
	  * list of cells forming a cluster to which this cell belongs too
	  *
	  * The attraction force is evaluated always against at least these cells.
	  */
	 struct myFriend {
	 	const Cell* buddy; 	//the buddy
		float timeLeft;		//how much remains of the friendship 

		myFriend(const Cell* buddy_,float timeLeft_) :
				buddy(buddy_), timeLeft(timeLeft_) {}

		myFriend(const myFriend& f) { buddy=f.buddy; timeLeft=f.timeLeft; }
	 };
	 std::list<Cell::myFriend> listOfFriends;
	 /** 
	  * calculate total attraction force based on the Cell::listOfFriends
	  *
	  * and decrease their friendship period myFriend::timeLeft by
	  * the parameter \e timeDelta
	  */
    void calculateAttractionForce(const float timeDelta);

	 ///weight factor for the Ph1 force - body force
	 float k;
	 ///allowed penetration depth [microns]
	 float delta_o;
	 ///counter-deformation force from a cell located at (\e ang,\e dist) polar coordinate from me
    void calculateBodyForce(const float ang, const float dist);
    void calculateBoundaryForce(void);

	 ///weight factor for the Ph2 force - sliding friction force
	 float kappa;
	 ///sliding friction force from a cell located the coordinate moving with given velocity
    void calculateSlidingFrictionForce(const float ang, const float dist,
			const Vector3d<float>& buddyVelocity);

	 /**
	  * calculates force expressing the cell's will to align with its own
	  * desired direction and velocity, also handles a friction term (as
	  * a kind of cell breaking/slowing_down mechanism)
	  */
    void calculateDesiredForce(void);

	 /**
	  * calculates all acting forces and fills appropriate structures
	  *
	  * the parameter \e timeDelta is here only to be
	  * forwarded to the calculateAttractionForce(timeDelta), which
	  * decreases remaining friendship period by this parameter
	  */
    void calculateForces(const float timeDelta);

	 ///simulate mainly motion due to forces after the \e timeDelta time
	 void applyForces(const float timeDelta);

     /**
      * @brief roundCell rounds the cell into a circle, preserving volume
      */
     void roundCell(void);
     /**
      * variables that control rounding of the cell
      */
     float round_cell_progress, round_cell_duration, shape_cell_progress, shape_cell_duration;
     float volume;

     int prog;
     float new_rotation;
     std::list<PolarPair> rotate_bp;
     bool rotate;
     int DirChCount;
     int DirChInter;
     void InitialRotation(float rotation);
     void ProgressRotationShrink();
     void ProgressRotationFinalizeShrink();
     void ProgressRotationStretch();
     void FinalizeRotation(float rotation);
};


#endif
