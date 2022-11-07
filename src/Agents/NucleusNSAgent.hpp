#pragma once

#include "../Geometries/util/SpheresFunctions.hpp"
#include "NucleusAgent.hpp"

/**
 * In the NS model (this class) the mutual arrangement of spheres is...
 */
class NucleusNSAgent : public NucleusAgent {
  public:
	NucleusNSAgent(const int ID,
	               std::string type,
	               const Spheres& shape,
	               const float currTime,
	               const float incrTime)
	    : NucleusAgent(ID, std::move(type), shape, currTime, incrTime),
	      distanceMatrix(int(shape.getNoOfSpheres())) {
		if (shape.getNoOfSpheres() < 2)
			throw report::rtError("Cannot construct NucleusNSAgent with less "
			                      "than two spheres geometry.");
		resetDistanceMatrix();
	}

	NucleusNSAgent(const NucleusNSAgent&) = delete;
	NucleusNSAgent& operator=(const NucleusNSAgent&) = delete;

	NucleusNSAgent(NucleusNSAgent&&) = default;
	NucleusNSAgent& operator=(NucleusNSAgent&&) = default;

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

	static NucleusNSAgent deserialize(std::span<const std::byte> bytes) {
		auto sa_size = extract<std::size_t>(bytes);

		auto sa = ShadowAgent::deserialize(bytes.first(sa_size));
		bytes = bytes.last(bytes.size() - sa_size);

		auto currTime = extract<float>(bytes);
		auto incrTime = extract<float>(bytes);

		auto na = NucleusNSAgent(sa.getID(), sa.getAgentType(),
		                         dynamic_cast<const Spheres&>(sa.getGeometry()),
		                         currTime, incrTime);
		--na.geometry->version;
		return na;
	}

  protected:
	// ------------- internals state -------------

	// ------------- internals geometry -------------
	SpheresFunctions::SquareMatrix<float> distanceMatrix;

	void resetDistanceMatrix();
	void printDistanceMatrix();

	// ------------- externals geometry -------------

	// ------------- to implement one round of simulation -------------
	void advanceAndBuildIntForces(const float futureGlobalTime) override;

#ifndef NDEBUG
	/** indices of spheres that gave rise to s2s force on some other sphere */
	std::vector<int> forces_s2sInducers;
#endif

	// ------------- rendering -------------
	void drawMask(DisplayUnit& du) override;
	void drawForDebug(DisplayUnit& du) override;
};
