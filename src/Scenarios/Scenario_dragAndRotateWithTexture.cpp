#include "../Agents/NucleusNSAgent.hpp"
#include "../Agents/util/Texture.hpp"
#include "../Agents/util/TextureFunctions.hpp"
#include "../DisplayUnits/FlightRecorderDisplayUnit.hpp"
#include "../DisplayUnits/SceneryBufferedDisplayUnit.hpp"
#include "../Geometries/Spheres.hpp"
#include "../Geometries/util/SpheresFunctions.hpp"
#include "common/Scenarios.hpp"
#include <fmt/core.h>

class myDragAndTextureNucleus_common : public NucleusNSAgent, Texture {
  public:
	myDragAndTextureNucleus_common(const int _ID,
	                               const std::string& _type,
	                               const Spheres& shape,
	                               const float _currTime,
	                               const float _incrTime)
	    : NucleusNSAgent(_ID, _type, shape, _currTime, _incrTime),
	      // TextureQuantized(60000, Vector3d<float>(2.0f,2.0f,2.0f), 8) //does
	      // not tear the texture in phantoms  (5x slower)
	      Texture(20000) // tears the texture a bit in phantom images but it is
	                     // not apparent in finalPreviews (5x faster)
	{
		cytoplasmWidth = 0.0f;

		/*
		for (int i=2; i < shape.getNoOfSpheres(); ++i)
		{
		    TextureFunctions::addTextureAlongLine(dots,
		futureGeometry.getCentres()[0],futureGeometry.getCentres()[i],2000);
		    TextureFunctions::addTextureAlongLine(dots,
		futureGeometry.getCentres()[1],futureGeometry.getCentres()[i],2000);
		}
		*/
		TextureFunctions::addTextureAlongGrid(
		    dots, geometryAlias, Geometry::point_t(0), Geometry::point_t(3),
		    Vector3d<float>(0.5f));
	}

	void advanceAndBuildIntForces(const float futureGlobalTime) override {
		// add own forces (mutually opposite) to "head&tail" spheres (to spin
		// it)
		/*
		Vector3d<float> rotationVelocity;
		rotationVelocity.y = 1.5f* std::cos(currTime/30.f * 6.28f);
		rotationVelocity.z = 1.5f* std::sin(currTime/30.f * 6.28f);
		*/

		/*
		const int radialPos = (ID-1) % 3;
		const float velocity = (float)radialPos*0.4f + 0.6f; - rings of
		different velocities
		*/
		const float velocity = 1.0f;
		// report::message(fmt::format("ID= {} radialPos={} velocity={}", ID,
		// radialPos, velocity));
		Vector3d<float> travellingVelocity(0, velocity, 0);
		exertForceOnSphere(
		    1, (weights[0] / velocity_PersistenceTime) * travellingVelocity,
		    ftype_drive);

		/*
		rotationVelocity *= -1;
		forces.emplace_back(
		    (weights[3]/velocity_PersistenceTime) * rotationVelocity,
		    futureGeometry.getCentres()[3],3, ftype_drive );
		*/

		// define own velocity
		/*
		velocity_CurrentlyDesired.y = 1.0f* std::cos(currTime/10.f * 6.28f);
		velocity_CurrentlyDesired.z = 1.0f* std::sin(currTime/10.f * 6.28f);
		*/

		// call the original method... takes care of own velocity, spheres
		// mis-alignments, etc.
		NucleusNSAgent::advanceAndBuildIntForces(futureGlobalTime);
	}

	void adjustGeometryByExtForces(void) override {
		NucleusNSAgent::adjustGeometryByExtForces();

		// update only just before the texture rendering event... (to save some
		// comp. time)
		if (Officer->willRenderNextFrame()) {
			updateTextureCoords(dots, futureGeometry);

			// correct for outlying texture particles
			int dotOutliers = collectOutlyingDots(futureGeometry);
			report::debugMessage(fmt::format(
			    "{} ({} %) dots had to be moved inside the current "
			    "geometry",
			    dotOutliers, 100.0 * dotOutliers / double(dots.size())));
		}
	}

	virtual void updateTextureCoords(std::vector<Dot>&, const Spheres&) {
		report::error("don't you dare to call me!");
	}

	void drawTexture(i3d::Image3d<float>& phantom,
	                 i3d::Image3d<float>&) override {
		renderIntoPhantom(phantom);
	}

	void drawMask(i3d::Image3d<i3d::GRAY16>& img) override {
		// shortcuts to the mask image parameters
		const Vector3d<float> res(img.GetResolution().GetRes());
		const Vector3d<float> off(img.GetOffset());

		// shortcuts to our Own spheres
		const std::vector<Geometry::point_t>& centresO =
		    futureGeometry.getCentres();
		const std::vector<Geometry::precision_t>& radiiO =
		    futureGeometry.getRadii();
		const std::size_t iO = futureGeometry.getNoOfSpheres();

		// project and "clip" this AABB into the img frame
		// so that voxels to sweep can be narrowed down...
		//
		//    sweeping position and boundaries (relevant to the 'img')
		Vector3d<size_t> curPos, minSweepPX, maxSweepPX;
		futureGeometry.AABB.exportInPixelCoords(img, minSweepPX, maxSweepPX);
		//
		// micron coordinate of the running voxel 'curPos'
		Vector3d<float> centre;

		// sweep and check intersection with spheres' volumes
		for (curPos.z = minSweepPX.z; curPos.z < maxSweepPX.z; curPos.z++)
			for (curPos.y = minSweepPX.y; curPos.y < maxSweepPX.y; curPos.y++)
				for (curPos.x = minSweepPX.x; curPos.x < maxSweepPX.x;
				     curPos.x++) {
					// get micron coordinate of the current voxel's centre
					centre.toMicronsFrom(curPos, res, off);

					// check the current voxel against all spheres
					for (std::size_t i = 0; i < iO; ++i) {
						if ((centre - centresO[i]).len() <= radiiO[i]) {
							img.SetVoxel(curPos.x, curPos.y, curPos.z,
							             (i3d::GRAY16)(ID * 100 + i));
						}
					}
				}
	}
};

//==========================================================================
class myDragAndTextureNucleus_NStexture : public myDragAndTextureNucleus_common,
                                          TextureUpdaterNS {
  public:
	myDragAndTextureNucleus_NStexture(const int _ID,
	                                  const std::string& _type,
	                                  const Spheres& shape,
	                                  const float _currTime,
	                                  const float _incrTime,
	                                  const int maxNeighs = 1)
	    : myDragAndTextureNucleus_common(
	          _ID, _type, shape, _currTime, _incrTime),
	      TextureUpdaterNS(shape, maxNeighs) {}

	void updateTextureCoords(std::vector<Dot>& dots,
	                         const Spheres& newGeom) override {
		this->TextureUpdaterNS::updateTextureCoords(dots, newGeom);
	}

	void drawMask(DisplayUnit& du) override {
		// show the usual stuff....
		NucleusNSAgent::drawMask(du);

		// and also the local orientations
		Vector3d<float> orientVec;
		int gdID = DisplayUnit::firstIdForSceneDebugObjects() + ID * 40 + 15000;
		for (std::size_t i = 0; i < futureGeometry.getNoOfSpheres(); ++i) {
			getLocalOrientation(futureGeometry, int(i), orientVec);
			du.DrawVector(gdID++, futureGeometry.getCentres()[i], orientVec, 0);
		}
	}
};

class myDragAndTextureNucleus_2pNStexture
    : public myDragAndTextureNucleus_common,
      TextureUpdater2pNS {
  public:
	myDragAndTextureNucleus_2pNStexture(const int _ID,
	                                    const std::string& _type,
	                                    const Spheres& shape,
	                                    const float _currTime,
	                                    const float _incrTime)
	    : myDragAndTextureNucleus_common(
	          _ID, _type, shape, _currTime, _incrTime),
	      TextureUpdater2pNS(shape, 0, 1) {}

	void updateTextureCoords(std::vector<Dot>& dots,
	                         const Spheres& newGeom) override {
		this->TextureUpdater2pNS::updateTextureCoords(dots, newGeom);
	}

	void drawMask(DisplayUnit& du) override {
		// show the usual stuff....
		NucleusNSAgent::drawMask(du);

		// and also the local orientations
		Vector3d<float> orientVec =
		    futureGeometry.getCentres()[sphereOnMainAxis];
		orientVec -= futureGeometry.getCentres()[sphereAtCentre];
		orientVec.changeToUnitOrZero();
		int gdID = DisplayUnit::firstIdForSceneDebugObjects() + ID * 40 + 15000;
		for (std::size_t i = 0; i < futureGeometry.getNoOfSpheres(); ++i) {
			du.DrawVector(gdID++, futureGeometry.getCentres()[i], orientVec, 0);
		}
	}
};

//==========================================================================
void Scenario_dragRotateAndTexture::initializeAgents(FrontOfficer* fo,
                                                     int p,
                                                     int) {
	if (p != 1) {
		report::message(
		    "Populating only the first FO (which is not this one).");
		return;
	}

	const int howManyToPlace = argc > 2 ? atoi(argv[2]) : 12;
	report::message(fmt::format("will rotate {} nuclei...", howManyToPlace));

	for (int i = 0; i < howManyToPlace; ++i) {
		const float ang = float(i) / float(howManyToPlace);
		const float radius = 40.0f;

		// the wished position relative to [0,0,0] centre
		Vector3d<float> axis(0, std::cos(ang * 6.28f), std::sin(ang * 6.28f));
		Vector3d<float> pos(0, radius * axis.y, radius * axis.z);

		// position is shifted to the scene centre
		pos.x += params->constants.sceneSize.x / 2.0f;
		pos.y += params->constants.sceneSize.y / 2.0f;
		pos.z += params->constants.sceneSize.z / 2.0f;

		// position is shifted due to scene offset
		pos.x += params->constants.sceneOffset.x;
		pos.y += params->constants.sceneOffset.y;
		pos.z += params->constants.sceneOffset.z;

		Spheres s(6);
		// central axis
		s.updateCentre(0, pos);
		s.updateRadius(0, 5.0f);
		s.updateCentre(1, pos + 8.0f * axis);
		s.updateRadius(1, 5.0f);

		// satellites
		s.updateCentre(2, pos + Vector3d<float>(0, +5, 0) + 6.0f * axis);
		s.updateRadius(2, 3.0f);
		s.updateCentre(3, pos + Vector3d<float>(0, 0, +5) + 6.0f * axis);
		s.updateRadius(3, 3.0f);
		s.updateCentre(4, pos + Vector3d<float>(0, -5, 0) + 6.0f * axis);
		s.updateRadius(4, 3.0f);
		s.updateCentre(5, pos + Vector3d<float>(0, 0, -5) + 6.0f * axis);
		s.updateRadius(5, 3.0f);

		fo->startNewAgent(new myDragAndTextureNucleus_NStexture(
		    fo->getNextAvailAgentID(), "nucleus travelling NS (3) texture", s,
		    params->constants.initTime, params->constants.incrTime, 3));

		const Vector3d<float> shiftVec(14, 0, 0);
		for (std::size_t i = 0; i < s.getNoOfSpheres(); ++i)
			s.updateCentre(int(i), s.getCentres()[i] + shiftVec);

		fo->startNewAgent(new myDragAndTextureNucleus_NStexture(
		    fo->getNextAvailAgentID(), "nucleus travelling NS texture", s,
		    params->constants.initTime, params->constants.incrTime));

		for (std::size_t i = 0; i < s.getNoOfSpheres(); ++i)
			s.updateCentre(int(i), s.getCentres()[i] + shiftVec);

		fo->startNewAgent(new myDragAndTextureNucleus_2pNStexture(
		    fo->getNextAvailAgentID(), "nucleus travelling 2pNS texture", s,
		    params->constants.initTime, params->constants.incrTime));
	}
}

void Scenario_dragRotateAndTexture::initializeScene() {
	// override the output images
	params->setOutputImgSpecs(Vector3d<float>(220, 30, 30),
	                         Vector3d<float>(70, 160, 160));

	disks.enableImgMaskTIFFs();
	disks.enableImgPhantomTIFFs();
	disks.enableImgOpticsTIFFs();

	displays.registerDisplayUnit(
	    []() { return std::make_unique<SceneryBufferedDisplayUnit>("localhost:8765"); });
	displays.registerDisplayUnit([]() {
		return std::make_unique<FlightRecorderDisplayUnit>(
		    "/temp/FR_dragRotateAndTexture.txt");
	});

	// displays.registerImagingUnit("localFiji","localhost:54545");
	// displays.enableImgPhantomInImagingUnit("localFiji");
}

std::unique_ptr<SceneControls> Scenario_dragRotateAndTexture::provideSceneControls() const {
	config::scenario::ControlConstants myConstants;

	// override the default stop time
	myConstants.stopTime = 40.2f;

	return std::make_unique<SceneControls>(myConstants);
}
