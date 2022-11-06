#pragma once

#include "../Geometries/Spheres.hpp"
#include "../util/report.hpp"
#include "AbstractAgent.hpp"
#include <fmt/core.h>
#include <list>
#include <vector>

const ForceName_t ftype_s2s = "sphere-sphere"; // internal forces
const ForceName_t ftype_drive = "desired movement";
const ForceName_t ftype_friction = "friction";

const ForceName_t ftype_repulsive =
    "repulsive"; // due to external events with nuclei
const ForceName_t ftype_body = "no overlap (body)";
const ForceName_t ftype_slide = "no sliding";

const ForceName_t ftype_hinter =
    "sphere-hinter"; // due to external events with shape hinters

constexpr float fstrength_body_scale = 0.4f;    // [N/um]      TRAgen: N/A
constexpr float fstrength_overlap_scale = 0.2f; // [N/um]      TRAgen: k
constexpr float fstrength_overlap_level = 0.1f; // [N]         TRAgen: A
constexpr float fstrength_overlap_depth =
    0.5f;                                   // [um]        TRAgen: delta_o (do)
constexpr float fstrength_rep_scale = 0.6f; // [1/um]      TRAgen: B
constexpr float fstrength_slide_scale = 1.0f;   // unitless
constexpr float fstrength_hinter_scale = 0.25f; // [1/um^2]

/** Color coding:
 0 - white
 1 - red
 2 - green
 3 - blue
 4 - cyan
 5 - magenta
 6 - yellow
 */

class NucleusAgent : public AbstractAgent {
  public:
	NucleusAgent(const int ID,
	             std::string type,
	             const Spheres& shape,
	             const float currTime,
	             const float incrTime)
	    : AbstractAgent(ID,
	                    std::move(type),
	                    std::make_unique<Spheres>(shape),
	                    currTime,
	                    incrTime),
	      geometryAlias(dynamic_cast<Spheres*>(geometry.get())),
	      futureGeometry(shape), accels(shape.getNoOfSpheres()),
	      // NB: relies on the fact that geometryAlias.noOfSpheres ==
	      // futureGeometry.noOfSpheres NB: accels[] and velocities[] together
	      // form one buffer (cache friendlier)
	      velocities(shape.getNoOfSpheres()), weights(shape.getNoOfSpheres()) {
		// update AABBs
		geometryAlias->Geometry::updateOwnAABB();
		futureGeometry.Geometry::updateOwnAABB();

		// estimate of number of forces (per simulation round):
		// 10(all s2s) + 4(spheres)*2(drive&friction) +
		// 10(neigs)*4(spheres)*4("outer" forces), and "up-rounded"...
		forces.reserve(200);
		velocity_CurrentlyDesired = 0; // no own movement desired yet
		velocity_PersistenceTime = 2.0f;

		for (std::size_t i = 0; i < shape.getNoOfSpheres(); ++i)
			weights[i] = 1.0f;

		report::debugMessage(
		    fmt::format("Nucleus with ID={} was just created", ID));
	}
	NucleusAgent(const NucleusAgent&) = delete;
	NucleusAgent& operator=(const NucleusAgent&) = delete;

	NucleusAgent(NucleusAgent&&) = default;
	NucleusAgent& operator=(NucleusAgent&&) = default;

	~NucleusAgent() {
		report::debugMessage(
		    fmt::format("Nucleus with ID={} was just deleted", ID));
	}

	const Vector3d<float>& getVelocityOfSphere(const long index) const {
#ifndef NDEBUG
		if (index >= long(geometryAlias->getNoOfSpheres()))
			throw report::rtError("requested sphere index out of bound.");
#endif

		return velocities[index];
	}

	/** --------- Serialization support --------- */
	virtual std::vector<std::byte> serialize() const override {
		std::vector<std::byte> out;

		auto sa = ShadowAgent::serialize();
		out += Serialization::toBytes(sa.size());
		out += sa;

		out += Serialization::toBytes(currTime);
		out += Serialization::toBytes(incrTime);

		return out;
	}
	virtual agent_class getAgentClass() const override {
		return agent_class::NucleusAgent;
	}

	static NucleusAgent deserialize(std::span<const std::byte> bytes) {
		auto sa_size = extract<std::size_t>(bytes);

		auto sa = ShadowAgent::deserialize(bytes.first(sa_size));
		bytes = bytes.last(bytes.size() - sa_size);

		auto currTime = extract<float>(bytes);
		auto incrTime = extract<float>(bytes);

		return NucleusAgent(sa.getID(), sa.getAgentType(),
		                    dynamic_cast<const Spheres&>(sa.getGeometry()),
		                    currTime, incrTime);
	}

  protected:
	/** essentially creates a new version (next iteration) of 'futureGeometry'
	   given the current content of the 'forces'; note that, in this particular
	   agent type, the 'geometryAlias' is kept synchronized with the
	   'futureGeometry' so they seem to be interchangeable, but in general
	   setting the 'futureGeometry' might be more rich representation of the
	   current geometry that is regularly "exported" via publishGeometry() and
	   for which the list of ProximityPairs was built during collectExtForces()
	 */
	void adjustGeometryByForces();

	/** helper method to (correctly) create a force acting on a sphere */
	inline void exertForceOnSphere(const int sphereIdx,
	                               const Vector3d<float>& forceVector,
	                               const ForceName_t forceType) {
		forces.emplace_back(forceVector, futureGeometry.centres[sphereIdx],
		                    sphereIdx, forceType);
	}

	// ------------- to implement one round of simulation -------------
	void advanceAndBuildIntForces(const float futureGlobalTime) override;

	void adjustGeometryByIntForces() override { adjustGeometryByForces(); }

	void collectExtForces() override;

	void adjustGeometryByExtForces() override { adjustGeometryByForces(); }

	void publishGeometry() override {
		// promote my NucleusAgent::futureGeometry to my ShadowAgent::geometry,
		// which happens to be overlaid/mapped-over with
		// NucleusAgent::geometryAlias (see the constructor)
		for (std::size_t i = 0; i < geometryAlias->getNoOfSpheres(); ++i) {
			geometryAlias->centres[i] = futureGeometry.centres[i];
			geometryAlias->radii[i] = futureGeometry.radii[i] + cytoplasmWidth;
		}

		// update AABB
		geometryAlias->Geometry::updateOwnAABB();
	}

	// ------------- rendering -------------
	void drawMask(DisplayUnit& du) override;
	void drawForDebug(DisplayUnit& du) override;
	void drawMask(i3d::Image3d<i3d::GRAY16>& img) override;

	// ------------- internals state -------------
	/** motion: desired current velocity [um/min] */
	Vector3d<float> velocity_CurrentlyDesired;

	/** motion: adaptation time, that is, how fast the desired velocity
	    should be reached (from zero movement); this param is in
	    the original literature termed as persistence time and so
	    we keep to that term [min] */
	float velocity_PersistenceTime;

	// ------------- internals geometry -------------
	/** reference to my exposed geometry ShadowAgents::geometry */
	Spheres* geometryAlias;

	/** my internal representation of my geometry, which is exactly
	    of the same form as my ShadowAgent::geometry, even the same noOfSpheres
	 */
	Spheres futureGeometry;

	/** width of the "retention zone" around nuclei that another nuclei
	    shall not enter; this zone simulates cytoplasm around the nucleus;
	    it actually behaves as if nuclei spheres were this much larger
	     in their radii; the value is in microns */
	float cytoplasmWidth = 2.0f;

	// ------------- externals geometry -------------
	/** limiting distance beyond which I consider no interaction possible
	    with other nuclei */
	float ignoreDistance = 10.0f;

	/** locations of possible interaction with nearby nuclei */
	tools::structures::SmallVector5<ProximityPair> proximityPairs_toNuclei;

	/** locations of possible interaction with nearby yolk */
	tools::structures::SmallVector5<ProximityPair> proximityPairs_toYolk;

	/** locations of possible interaction with guiding trajectories */
	tools::structures::SmallVector5<ProximityPair> proximityPairs_tracks;

	// ------------- forces & movement (physics) -------------
	/** all forces that are in present acting on this agent */
	std::vector<ForceVector3d<float>> forces;

	/** an aux array of acceleration vectors calculated for every sphere, the
	   length of this array must match the length of the spheres in the
	   'futureGeometry' */
	std::vector<Vector3d<float>> accels;

	/** an array of velocities vectors of the spheres, the length of this array
	   must match the length of the spheres that are exposed (geometryAlias) to
	   the outer world */
	std::vector<Vector3d<float>> velocities;

	/** an aux array of weights of the spheres, the length of this array must
	   match the length of the spheres in the 'futureGeometry' */
	std::vector<float> weights;

#ifndef NDEBUG
	/** aux memory of the recently generated forces in
	   advanceAndBuildIntForces() and in collectExtForces(), and displayed via
	   drawForDebug() */
	std::vector<ForceVector3d<float>> forcesForDisplay;
#endif
};
