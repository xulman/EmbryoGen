#include "../Agents/NucleusNSAgent.hpp"
#include "../Agents/util/Texture.hpp"
#include "../DisplayUnits/SceneryBufferedDisplayUnit.hpp"
#include "../Geometries/Spheres.hpp"
#include "../Geometries/util/SpheresFunctions.hpp"
#include "../util/texture/texture.hpp"
#include "common/Scenarios.hpp"

class TetrisNucleus : public NucleusNSAgent, TextureUpdaterNS {
  public:
	TetrisNucleus(const int _ID,
	              const std::string& _type,
	              const Spheres& shape,
	              const int _activeSphereIdx,
	              const float _currTime,
	              const float _incrTime)
	    : NucleusNSAgent(_ID, _type, shape, _currTime, _incrTime),
	      TextureUpdaterNS(shape, 1), activeSphereIdx(_activeSphereIdx)
	// Texture(60000)
	// TextureQuantized(60000, Vector3d<float>(2.0f,2.0f,2.0f), 8)
	{
		// texture img: resolution -- makes sense to match it with the phantom
		// img resolution
		/*
		createPerlinTexture(futureGeometry, Vector3d<float>(2.0f),
		                    5.0,8,4,6,    //Perlin
		                    1.0f,         //texture intensity range centre
		                    0.1f, true);  //quantization and
		shouldCollectOutlyingDots
		*/
	}

	/*
	void drawTexture(i3d::Image3d<float>& phantom, i3d::Image3d<float>&)
	override
	{
	    renderIntoPhantom(phantom);
	}
	*/

	void drawMask(DisplayUnit& du) override {
		int dID = DisplayUnit::firstIdForAgentObjects(ID);
		int ldID = DisplayUnit::firstIdForAgentDebugObjects(ID);

		// spheres all green, except: 0th is white, "active" is red
		du.DrawPoint(dID++, futureGeometry.getCentres()[0],
		             futureGeometry.getRadii()[0], 0);
		for (std::size_t i = 1; i < futureGeometry.getNoOfSpheres(); ++i)
			du.DrawPoint(dID++, futureGeometry.getCentres()[i],
			             futureGeometry.getRadii()[i],
			             int(i) == activeSphereIdx ? 1 : 2);

		// sphere orientations as local debug, white vectors
		Vector3d<G_FLOAT> orientVec;
		for (std::size_t i = 0; i < futureGeometry.getNoOfSpheres(); ++i) {
			getLocalOrientation(futureGeometry, int(i), orientVec);
			du.DrawVector(ldID++, futureGeometry.getCentres()[i], orientVec, 0);
		}
		/*
		NucleusNSAgent::drawMask(du);
		*/

		drawForDebug(du);
	}

	void advanceAgent(float time) override {
		const G_FLOAT velocity = (G_FLOAT)1.0f;
		const Vector3d<G_FLOAT> travellingVelocity(0, velocity, 0);
		if (((int)time % 18) < 8) {
			exertForceOnSphere(
			    activeSphereIdx,
			    (weights[activeSphereIdx] / velocity_PersistenceTime) *
			        travellingVelocity,
			    ftype_drive);
		}
	}

  private:
	const int activeSphereIdx;
};

//==========================================================================
void Scenario_Tetris::initializeAgents(FrontOfficer* fo, int p, int) {
	if (p != 1) {
		report::message(fmt::format(
		    "Populating only the first FO (which is not this one)."));
		return;
	}

	// shapes: 2x2 box, 1x4 line, big-T, random bulk
	// arranged in a 4x4 grid (that is, boundaries of 5x5 blocks)
	const Vector3d<float> gridStart(
	    Vector3d<float>(params.constants.sceneSize).elemMathOp([](float e) {
		    return e / 5;
	    }));
	const Vector3d<float> gridStep(gridStart);
	Vector3d<float> currGridPos;

	Spheres spheres(7);
	std::string agentName;

	const float sDist = 4;
	const float sRadius = 3;

	for (int x = 0; x < 4; ++x)
		for (int y = 0; y < 4; ++y) {
			currGridPos.from(x, y, 2).elemMult(gridStep);
			currGridPos += gridStart;

			switch (x) {
			case -1:
				agentName = fmt::format("{} __Box", y);
				spheres.updateCentre(0, currGridPos);
				spheres.updateCentre(
				    1, currGridPos + Vector3d<float>(-sDist, -sDist / 2.0f, 0));
				spheres.updateCentre(
				    2, currGridPos + Vector3d<float>(-sDist, +sDist / 2.0f, 0));
				spheres.updateCentre(
				    3, currGridPos + Vector3d<float>(0, -sDist / 1.5f, 0));
				spheres.updateCentre(
				    4, currGridPos + Vector3d<float>(0, +sDist / 1.5f, 0));
				spheres.updateCentre(
				    5, currGridPos + Vector3d<float>(+sDist, -sDist / 2.0f, 0));
				spheres.updateCentre(
				    6, currGridPos + Vector3d<float>(+sDist, +sDist / 2.0f, 0));
				break;
			case 0:
				agentName = fmt::format("{} __Line", y);
				spheres.updateCentre(0, currGridPos);
				spheres.updateCentre(
				    1, currGridPos +
				           Vector3d<float>(-sDist * 3, -sDist * 0.9f, 0));
				spheres.updateCentre(
				    2, currGridPos +
				           Vector3d<float>(-sDist * 2, -sDist * 0.6f, 0));
				spheres.updateCentre(
				    3, currGridPos +
				           Vector3d<float>(-sDist * 1, -sDist * 0.3f, 0));
				spheres.updateCentre(
				    4, currGridPos +
				           Vector3d<float>(+sDist * 1, +sDist * 0.3f, 0));
				spheres.updateCentre(
				    5, currGridPos +
				           Vector3d<float>(+sDist * 2, +sDist * 0.6f, 0));
				spheres.updateCentre(
				    6, currGridPos +
				           Vector3d<float>(+sDist * 3, +sDist * 0.9f, 0));
				break;
			case 1:
				agentName = fmt::format("{} __BigT", y);
				spheres.updateCentre(0, currGridPos);
				spheres.updateCentre(1, currGridPos +
				                            Vector3d<float>(-sDist * 2, 0, 0));
				spheres.updateCentre(2, currGridPos +
				                            Vector3d<float>(-sDist * 1, 0, 0));
				spheres.updateCentre(3, currGridPos +
				                            Vector3d<float>(0, -sDist * 1, 0));
				spheres.updateCentre(4, currGridPos +
				                            Vector3d<float>(0, -sDist * 2, 0));
				spheres.updateCentre(5, currGridPos +
				                            Vector3d<float>(+sDist * 1, 0, 0));
				spheres.updateCentre(6, currGridPos +
				                            Vector3d<float>(+sDist * 2, 0, 0));
				break;
			case 2:
				agentName = fmt::format("{} __Bulk", y);
				for (std::size_t i = 0; i < spheres.getNoOfSpheres(); ++i) {
					spheres.updateCentre(
					    int(i),
					    currGridPos +
					        Vector3d<float>(
					            GetRandomUniform(-0.8f * sDist, +0.8f * sDist),
					            GetRandomUniform(-0.8f * sDist, +0.8f * sDist),
					            GetRandomUniform(-0.8f * sDist,
					                             +0.8f * sDist)));
				}
				break;
			default:
				report::message(
				    fmt::format("WARNING: no agents at grid pos {},{}", y, x));
			}

			for (std::size_t i = 0; i < spheres.getNoOfSpheres(); ++i)
				spheres.updateRadius(int(i),
				                     sRadius + GetRandomUniform(-0.8f, +0.8f));

			fo->startNewAgent(new TetrisNucleus(
			    fo->getNextAvailAgentID(), agentName, spheres,
			    y < 2 ? y + 1 : int(spheres.getNoOfSpheres()) + y - 4,
			    params.constants.initTime, params.constants.incrTime));
		}

	// right-most column with 2S
	int x = 3;
	for (int y = 0; y < 4; ++y) {
		currGridPos.from(x, y, 2).elemMult(gridStep);
		currGridPos += gridStart;
		agentName = fmt::format("{} __2SModel", y);

		const int connLines = 5;
		const int inbetweeners = 5;
		Spheres twoS(2); // + connLines*inbetweeners);

		std::vector<Vector3d<G_FLOAT>*> lineUps;
		for (int i = 0; i < inbetweeners; ++i)
			lineUps.push_back(new Vector3d<G_FLOAT>());

		const float maxAxisDev = 1.4f;
		twoS.updateCentre(
		    0, currGridPos +
		           Vector3d<float>(-9.f,
		                           GetRandomUniform(-maxAxisDev * sDist,
		                                            +maxAxisDev * sDist),
		                           GetRandomUniform(-maxAxisDev * sDist,
		                                            +maxAxisDev * sDist)));
		twoS.updateCentre(
		    1, currGridPos +
		           Vector3d<float>(+9.f,
		                           GetRandomUniform(-maxAxisDev * sDist,
		                                            +maxAxisDev * sDist),
		                           GetRandomUniform(-maxAxisDev * sDist,
		                                            +maxAxisDev * sDist)));
		twoS.updateRadius(0, 3.5);
		twoS.updateRadius(1, 4.5);

		report::message(fmt::format("-------------------------------------"));
		SpheresFunctions::LinkedSpheres<float> builder(
		    twoS, Vector3d<float>(0, 1, 0));
		float minAngle = (float)-M_PI_2;
		float maxAngle = (float)+M_PI_2;
		float stepAngle = (maxAngle - minAngle) / (float)connLines;
		builder.resetAllAzimuthsToExtrusions(minAngle, stepAngle, maxAngle);
		builder.resetNoOfSpheresInAllAzimuths(inbetweeners);
		// builder.defaultNoOfSpheresOnConnectionLines=2;
		// builder.addOrChangeAzimuthToExtrusion(M_PI-0.2);
		// builder.addOrChangeAzimuthToExtrusion(M_PI+0.2);
		// builder.addOrChangeAzimuth(M_PI, builder.defaultPosNoAdjustment,
		// [](float,float){return 2;}, 4);
		// builder.addOrChangeAzimuth(M_PI,[](Vector3d<float>& v,float f){v +=
		// Vector3d<float>(0,0,f-0.5f +15);},builder.defaultRadiusNoChg, 4);
		// builder.addOrChangeAzimuth(M_PI, builder.defaultPosNoAdjustment,
		// builder.defaultRadiusNoChg, 4); builder.removeAzimuth(M_PI+0.2);
		// builder.addToPlan(0,1,3); //content of Interpolator is forbidden in
		// LinkedSpheres
		builder.printPlan();
		report::message(fmt::format("necessary cnt: {}",
		                            builder.getNoOfNecessarySpheres()));
		Spheres manyS(int(builder.getNoOfNecessarySpheres()));
		builder.buildInto(manyS);
		builder.printPlan();

		/*
		fo->startNewAgent( new TetrisNucleus(
		        fo->getNextAvailAgentID(),
		        agentName,twoS,y < 2 ? 0 : 1,
		        params.constants.initTime,params.constants.incrTime
		    ));
		*/
		fo->startNewAgent(new TetrisNucleus(
		    fo->getNextAvailAgentID(), agentName, manyS, 0,
		    params.constants.initTime, params.constants.incrTime));

		for (auto v : lineUps)
			delete v;
	}
}

void Scenario_Tetris::initializeScene() {
	displays.registerDisplayUnit(
	    []() { return new SceneryBufferedDisplayUnit("localhost:8765"); });
	// displays.registerDisplayUnit( [](){ return new
	// FlightRecorderDisplayUnit("/temp/FR_tetris.txt"); } );

	// disks.enableImgMaskTIFFs();
	// disks.enableImgPhantomTIFFs();
	// disks.enableImgOpticsTIFFs();

	// displays.registerImagingUnit("localFiji","localhost:54545");
	// displays.enableImgPhantomInImagingUnit("localFiji");
}

SceneControls& Scenario_Tetris::provideSceneControls() {
	SceneControls::Constants myConstants;

	// override the default stop time
	myConstants.stopTime = 40.2f;
	myConstants.sceneSize.from(250, 250,
	                           50); // microns, offset is at [0,0,0]
	// image res 2px/1um

	return *(new SceneControls(myConstants));
}
