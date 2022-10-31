#include "strings.hpp"
#include "../Geometries/Geometry.hpp"
#include <list>
#include <string>

std::ostream& operator<<(std::ostream& s, const hashedString& hs) {
	s << hs.getString();
	return s;
}

// ------------- StringsDictionary methods -------------
const std::string&
StringsDictionary::translateIdToString(const size_t ID) const {
	//(re)use iterator from find (both in another find() and in return)
	auto resFind = knownDictionary.find(ID);
	if (resFind != knownDictionary.end()) {
		return resFind->second;
	} else {
		resFind = newDictionary.find(ID);
		if (resFind != newDictionary.end()) {
			return resFind->second;
		} else {
			// we have to return a string but here we have none at hand,
			// so we sadly have to throw an exception
			throw report::rtError(fmt::format(
			    "String with ID/hash {} is not existing in the dictionary.",
			    ID));
		}
	}
}

void StringsDictionary::registerThisString(const std::string& s) {
	const size_t ID = hashedString::hash(s);

	// don't add anything if it is already in the Dictionary
	if (newDictionary.find(ID) == newDictionary.end() &&
	    knownDictionary.find(ID) == knownDictionary.end())
		newDictionary[ID] = s;
}

void StringsDictionary::registerThisString(const hashedString& s) {
	// don't add anything if it is already in the Dictionary
	if (newDictionary.find(s.getHash()) == newDictionary.end() &&
	    knownDictionary.find(s.getHash()) == knownDictionary.end())
		newDictionary[s.getHash()] = s.getString();
}

bool StringsDictionary::isInDictionary(std::size_t s) const {
	return knownDictionary.contains(s) || newDictionary.contains(s);
}

void StringsDictionary::markAllWasBroadcast() {
	for (const auto& nItem : newDictionary) {
#ifndef NDEBUG
		if (knownDictionary.find(nItem.first) != knownDictionary.end())
			report::message(
			    fmt::format("new to already known >>{}", nItem.second));
#endif
		knownDictionary[nItem.first] = nItem.second;
	}
	newDictionary.clear();
}

void StringsDictionary::enlistTheIncomingItem(const size_t hash,
                                              const std::string& string) {
	if (knownDictionary.find(hash) == knownDictionary.end()) {
		// new item
		knownDictionary[hash] = string;
	} else {
		report::debugMessage(
		    fmt::format("received already known >>{}", string));

		// known item (IDs/hashes are matching), check that strings are matching
		// too
		if (string.compare(knownDictionary[hash]) != 0)
			throw report::rtError(
			    fmt::format("Hashing malfunction: Have >>{}<< and got >>{}<<, "
			                "both of the same hash = {}",
			                knownDictionary[hash], string, hash));
	}
}
