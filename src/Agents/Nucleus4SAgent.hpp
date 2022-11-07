#pragma once

#include "NucleusAgent.hpp"
#include <array>

class Nucleus4SAgent : public NucleusAgent {
  public:
	Nucleus4SAgent(const int ID,
	               std::string type,
	               const Spheres& shape,
	               const float currTime,
	               const float incrTime)
	    : NucleusAgent(ID, std::move(type), shape, currTime, incrTime) {
		if (shape.getNoOfSpheres() != 4)
			throw report::rtError(
			    "Cannot construct Nucleus4SAgent on non-four sphere geometry.");

		// init centreDistances based on the initial geometry
		for (std::size_t i = 0; i < centreDistance.size(); ++i)
			centreDistance[i] = float(
			    (geometryAlias->centres[i + 1] - geometryAlias->centres[i])
			        .len());
	}

	Nucleus4SAgent(const Nucleus4SAgent&) = delete;
	Nucleus4SAgent& operator=(const Nucleus4SAgent&) = delete;

	// Geometry alias does not update :(
	Nucleus4SAgent(Nucleus4SAgent&&) = delete;
	Nucleus4SAgent& operator=(Nucleus4SAgent&&) = delete;

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
		return agent_class::NucleusNSAgent;
	}

	static std::unique_ptr<Nucleus4SAgent>
	deserialize(std::span<const std::byte> bytes) {
		auto sa_size = extract<std::size_t>(bytes);

		auto sa = ShadowAgent::deserialize(bytes.first(sa_size));
		bytes = bytes.last(bytes.size() - sa_size);

		auto currTime = extract<float>(bytes);
		auto incrTime = extract<float>(bytes);

		auto na = std::make_unique<Nucleus4SAgent>(
		    sa->getID(), sa->getAgentType(),
		    dynamic_cast<const Spheres&>(sa->getGeometry()), currTime,
		    incrTime);
		--na->geometry->version;
		return na;
	}

  protected:
	// ------------- internals state -------------

	// ------------- internals geometry -------------
	/** canonical distance between the four cell centres that this agent
	    should maintain during geometry changes during the simulation */
	std::array<float, 3> centreDistance;

	/** assuming that the four spheres (that make up this nucleus) should lay
	    on a line with their centres apart as given in centreDistances[], this
	    method computes vector for every sphere centre that show how much
	    it is displaced from its expected position given that the assumed line
	    goes through the inner two spheres */
	void getCurrentOffVectorsForCentres(Vector3d<float> offs[4]);

	// ------------- externals geometry -------------

	// ------------- to implement one round of simulation -------------
	void advanceAndBuildIntForces(const float futureGlobalTime) override;

  protected:
	// ------------- rendering -------------
	void drawForDebug(DisplayUnit& du) override;
};
