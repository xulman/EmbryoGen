#include "../util/Vector3d.h"
#include "../Geometries/Spheres.h"
#include "../Simulation.h"
#include "../Agents/NucleusAgent.h"
#include "../Agents/util/CellCycle.h"
#include "Scenarios.h"

class FirstFourCells: public NucleusAgent, CellCycle
{
public:
	FirstFourCells(const int _ID, const std::string& _type,
	               const Spheres& shape,
	               const float _currTime, const float _incrTime,
	               const int _generation)
		: NucleusAgent(_ID,_type, shape, _currTime,_incrTime),
		  CellCycle(10.f,0.17f), generation(_generation)
	{
		if (shape.getNoOfSpheres() != 2)
			throw ERROR_REPORT("Was expecting 2-spheres initial geometry.");
		startCycling(_currTime);

		REPORT(IDSIGN << "c'tor @ generation " << generation);
	}

	~FirstFourCells(void)
	{ REPORT(IDSIGN << "d'tor @ generation " << generation); }

	const int generation = -1;

	//shortcut to define cyto width from the outside
	void setCytoplasmWidth(const float cytoWidth)
	{
		cytoplasmWidth = cytoWidth;
	}


	//this is how you can override lenghts of the cell phases
	void determinePhaseDurations(void) override
	{
		phaseDurations[G1Phase    ] = 0.2f * fullCycleDuration;
		phaseDurations[SPhase     ] = 0.0f * fullCycleDuration;
		phaseDurations[G2Phase    ] = 0.0f * fullCycleDuration;
		phaseDurations[Prophase   ] = 0.0f * fullCycleDuration;
		phaseDurations[Metaphase  ] = 0.0f * fullCycleDuration;
		phaseDurations[Anaphase   ] = 0.6f * fullCycleDuration;
		phaseDurations[Telophase  ] = 0.0f * fullCycleDuration;
		phaseDurations[Cytokinesis] = 0.2f * fullCycleDuration;
	}

	float ana_initCentreDist, ana_targetCentreDist;
	float ana_initRadius[2],  ana_targetRadius[2];
	float ana_initCytoRadius, ana_targetCytoRadius;

	void startAnaphase(void) override
	{
		ana_initCentreDist = (futureGeometry.getCentres()[0] - futureGeometry.getCentres()[1]).len();
		ana_targetCentreDist = 0.65f * (futureGeometry.getRadii()[0] + futureGeometry.getRadii()[1]);
		DEBUG_REPORT("initDist=" << ana_initCentreDist << ", targetDist=" << ana_targetCentreDist);

		const float decreaseRadius = 0.8f;

		ana_initRadius[0]   = futureGeometry.getRadii()[0];
		ana_initRadius[1]   = futureGeometry.getRadii()[1];
		ana_targetRadius[0] = decreaseRadius * ana_initRadius[0];
		ana_targetRadius[1] = decreaseRadius * ana_initRadius[1];

		ana_initCytoRadius = cytoplasmWidth;
		ana_targetCytoRadius = decreaseRadius * cytoplasmWidth;
	}

	void runAnaphase(const float time) override
	{
		Vector3d<FLOAT> disp = futureGeometry.getCentres()[1] - futureGeometry.getCentres()[0];
		float currDist = disp.len();
		DEBUG_REPORT("currDist=" << currDist << " @ time=" << time);

		disp.changeToUnitOrZero();
		disp *= (time*(ana_targetCentreDist-ana_initCentreDist) +ana_initCentreDist) -currDist;

		futureGeometry.updateCentre(0, futureGeometry.getCentres()[0] - 0.5f * disp );
		futureGeometry.updateCentre(1, futureGeometry.getCentres()[1] + 0.5f * disp );

		for (int s=0; s < futureGeometry.getNoOfSpheres(); ++s)
		{
			float newRadius = time * (ana_targetRadius[s] - ana_initRadius[s]) + ana_initRadius[s];
			futureGeometry.updateRadius(s, newRadius);
		}

		cytoplasmWidth = time * (ana_targetCytoRadius - ana_initCytoRadius) + ana_initCytoRadius;
	}

	void closeAnaphase(void) override
	{
		ana_initCentreDist = (futureGeometry.getCentres()[0] - futureGeometry.getCentres()[1]).len();
		DEBUG_REPORT("currDist=" << ana_initCentreDist << ", targetDist=" << ana_targetCentreDist);
	}


	void startCytokinesis(void) override {}
	void runCytokinesis(const float) override {}
	void closeCytokinesis(void) override
	{
			DEBUG_REPORT("======================= Dividing... =======================");

			Vector3d<FLOAT> disp = futureGeometry.getCentres()[1] - futureGeometry.getCentres()[0];
			disp.changeToUnitOrZero();
			disp *= 0.1f;

			char agentType[] = "nucleusX";
			agentType[7] = (char)(generation+49);

			Spheres s(2);
			s.updateCentre(0, futureGeometry.getCentres()[0] - disp);
			s.updateRadius(0, futureGeometry.getRadii()[0]);
			s.updateCentre(1, futureGeometry.getCentres()[0] + disp);
			s.updateRadius(1, futureGeometry.getRadii()[0]);
			FirstFourCells* agA = new FirstFourCells(Officer->getNextAvailAgentID(),agentType,s,currTime+incrTime,incrTime, generation+1);
			agA->setCytoplasmWidth( cytoplasmWidth );

			s.updateCentre(0, futureGeometry.getCentres()[1] - disp);
			s.updateRadius(0, futureGeometry.getRadii()[1]);
			s.updateCentre(1, futureGeometry.getCentres()[1] + disp);
			s.updateRadius(1, futureGeometry.getRadii()[1]);
			FirstFourCells* agB = new FirstFourCells(Officer->getNextAvailAgentID(),agentType,s,currTime+incrTime,incrTime, generation+1);
			agB->setCytoplasmWidth( cytoplasmWidth );

			Officer->closeMotherStartDaughters(this,agA,agB);
	}


	void advanceAgent(const float gTime) override
	{ triggerCycleMethods(gTime); }

	void collectExtForces(void) override
	{
		NucleusAgent::collectExtForces();

		//scheduler, please give me AGAIN! ShadowAgents that are not further than ignoreDistance
		std::list<const ShadowAgent*> nearbyAgents;
		Officer->getNearbyAgents(this,ignoreDistance, nearbyAgents);

		//additionally deal with the egg shell
		for (const auto sa : nearbyAgents)
		if (sa->getAgentType()[0] == 'e')
		{
			//calculate how much any of my spheres is outlying the egg shell,
			//and define potentially respective force
			for (int s=0; s < geometryAlias.getNoOfSpheres(); ++s)
			{
				//shortcut (and recasting!) to the Sphere geometry, which we can afford
				//because we know we are now dealing with the EggShapeConstraint (which
				//is of the Spheres geometry)
				const Spheres* egg = (const Spheres*)(&sa->getGeometry());

				Vector3d<FLOAT> direction = egg->getCentres()[0] - geometryAlias.getCentres()[s];
				FLOAT dist = direction.len();
				dist += geometryAlias.getRadii()[s];
				dist -= egg->getRadii()[0];

				DEBUG_REPORT("egg distance (negative means inside): " << dist);

				//this sphere is outside by 'dist' microns
				if (dist > 0.f)
				{
					//simulate "body force"
					FLOAT fScale = fstrength_overlap_level;
					if (dist > fstrength_overlap_depth)
					{
						//in the non-calm response zone (where force increases with the penetration depth)
						fScale += fstrength_overlap_scale * (dist - fstrength_overlap_depth);
					}

					//TRAgen paper, eq. (5) -- body force -- note: direction cannot be zero, since dist != 0
					forces.emplace_back( fScale * direction.changeToUnitOrZero(),
						geometryAlias.getCentres()[s],s, ftype_hinter );
#ifdef DEBUG
					//append forces to forcesForDisplay, make a copy (push_back, not emplace_back)!
					forcesForDisplay.push_back(forces.back());
#endif
				}
			}
		}
	}

	void drawMask(DisplayUnit& du) override
	{
		int color = 2;

		int dID  = ID << 17;
		int ldID = dID | 1 << 16;  //local debug bit

		//draw spheres and velocities
		for (int i=0; i < futureGeometry.getNoOfSpheres(); ++i)
		{
			du.DrawPoint(dID++, futureGeometry.getCentres()[i],futureGeometry.getRadii()[i],color);
			if (!detailedDrawingMode)
				du.DrawVector(ldID++, futureGeometry.getCentres()[i],velocities[i],0); //white color
		}

		if (!detailedDrawingMode)
		{
			color = 4;
			SphereSampler<float> ss;
			Vector3d<float> periPoint;
			int periPointCnt=0;

			for (int S = 0; S < geometryAlias.getNoOfSpheres(); ++S)
			{
				ss.resetByStepSize(geometryAlias.getRadii()[S], 2.6f);
				while (ss.next(periPoint))
				{
					periPoint += geometryAlias.getCentres()[S];

					//draw the periPoint only if it collides with no (and excluding this) sphere
					if (geometryAlias.collideWithPoint(periPoint, S) == -1)
					{
						du.DrawPoint(ldID++, periPoint, 0.3f, color);
						++periPointCnt;
					}
				}
			}
			DEBUG_REPORT(IDSIGN << "surface consists of " << periPointCnt << " spheres");
		}
	}
};


class EggShapeConstraint: public AbstractAgent
{
public:
	EggShapeConstraint(const int ID, const std::string& type,
	                   const Spheres& shape,
	                   const float currTime, const float incrTime)
		: AbstractAgent(ID,type, geometryAlias, currTime,incrTime),
		  geometryAlias(shape)
	{
		geometryAlias.Geometry::updateOwnAABB();
		DEBUG_REPORT("EmbryoShell with ID=" << ID << " was just created");
	}

	~EggShapeConstraint(void)
	{
		DEBUG_REPORT("EmbryoShell with ID=" << ID << " was just deleted");
	}

protected:
	/** reference to my exposed geometry ShadowAgents::geometry,
	    is exactly the same as my futureGeometry would be... */
	Spheres geometryAlias;

	// ------------- necessary to implement one round of simulation -------------
	void advanceAndBuildIntForces(const float) override { currTime += incrTime; }
	void adjustGeometryByIntForces(void) override {}

	void collectExtForces(void) override {}
	void adjustGeometryByExtForces(void) override {}

	void publishGeometry(void) override {}

	void drawMask(DisplayUnit& du) override
	{
		const int color = 6;
		int dID = 1 << 16; //global debug bit

		SphereSampler<float> ss;
		Vector3d<float> periPoint;
		int periPointCnt=0;

		for (int S = 0; S < geometryAlias.getNoOfSpheres(); ++S)
		{
			ss.resetByStepSize(geometryAlias.getRadii()[S], 10.0f);
			while (ss.next(periPoint))
			{
				periPoint += geometryAlias.getCentres()[S];
				du.DrawPoint(dID++, periPoint, 0.4f, color);
				++periPointCnt;
			}
		}
		DEBUG_REPORT(IDSIGN << "EmbryoShell's surface consists of " << periPointCnt << " spheres");
	}
};


void Scenario_pseudoDivision::initializeScenario(void)
{
	//the wished position of the simulation relative to [0,0,0] centre
	Vector3d<float> pos(0,0,0);
	const float eggShellRadius = 40.f;

	//position is shifted to the scene centre
	pos.x += sceneSize.x/2.0f;
	pos.y += sceneSize.y/2.0f;
	pos.z += sceneSize.z/2.0f;

	//position is shifted due to scene offset
	pos.x += sceneOffset.x;
	pos.y += sceneOffset.y;
	pos.z += sceneOffset.z;

	//the egg shell itself
	Spheres egg(1);
	egg.updateCentre(0,pos);
	egg.updateRadius(0,eggShellRadius);
	startNewAgent( new EggShapeConstraint(getNextAvailAgentID(),"eggShell",egg,currTime,incrTime), false );

	//the first cell:
	//determine its "random" position within the egg //TODO
	pos += Vector3d<float>(0,4,2);

	Spheres s(2);
	s.updateCentre(0,pos+Vector3d<float>(-0.1f,0,0));
	s.updateRadius(0,10.0f);
	s.updateCentre(1,pos+Vector3d<float>(+0.1f,0,0));
	s.updateRadius(1,10.0f);

	FirstFourCells* ag = new FirstFourCells(getNextAvailAgentID(),"nucleus1",s,currTime,incrTime,1);
	ag->setCytoplasmWidth(20.f);
	startNewAgent(ag);

	stopTime = 100.0f;
}
