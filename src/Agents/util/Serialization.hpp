#pragma once
#include "../../util/report.hpp"
#include "../AbstractAgent.hpp"
#include "../Nucleus4SAgent.hpp"
#include "../NucleusNSAgent.hpp"
#include "../ShapeHinter.hpp"
#include "../TrajectoriesHinter.hpp"
#include <memory>
#include <vector>

inline std::unique_ptr<ShadowAgent>
deserializeAgent(std::span<const std::byte> bytes) {
	agent_class ag_cls =
	    static_cast<agent_class>(std::to_integer<int>(bytes.back()));
	bytes = bytes.first(bytes.size() - 1);

	switch (ag_cls) {
	case agent_class::ShadowAgent:
		return ShadowAgent::deserialize(bytes);
	case agent_class::AbstractAgent:
		throw report::rtError("Abstract agent cannot be serialized");
	case agent_class::NucleusAgent:
		return NucleusAgent::deserialize(bytes);
	case agent_class::NucleusNSAgent:
		return NucleusNSAgent::deserialize(bytes);
	case agent_class::Nucleus4SAgent:
		return Nucleus4SAgent::deserialize(bytes);
	case agent_class::ShapeHinter:
		return ShapeHinter::deserialize(bytes);
	case agent_class::TrajectoriesHinter:
		return TrajectoriesHinter::deserialize(bytes);
	}

	throw report::rtError("Obtained unsupported type for serialization");
}

inline std::vector<std::byte> serializeAgent(const ShadowAgent& ag) {
	agent_class ag_cls = ag.getAgentClass();
	auto data = ag.serialize();
	data.push_back(std::byte(static_cast<int>(ag_cls)));
	return data;
}