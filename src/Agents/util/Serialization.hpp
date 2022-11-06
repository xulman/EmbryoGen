#pragma once
#include "../../util/report.hpp"
#include "../AbstractAgent.hpp"
#include <memory>
#include <vector>

inline std::unique_ptr<ShadowAgent>
deserialize_agent(std::span<const std::byte> bytes) {
	agent_class ag_cls =
	    static_cast<agent_class>(std::to_integer<int>(bytes.back()));
	bytes = bytes.first(bytes.size() - 1);

	switch (ag_cls) {
	case agent_class::ShadowAgent:
		return std::make_unique<ShadowAgent>(ShadowAgent::deserialize(bytes));
	}

	throw report::rtError("Obtained unsupported type for serialization");
}

inline std::vector<std::byte> serialize_agent(const ShadowAgent& ag) {
	agent_class ag_cls = ag.getAgentClass();
	auto data = ag.serialize();
	data.emplace_back(static_cast<int>(ag_cls));
	return data;
}