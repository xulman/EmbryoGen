#include <string>
#include <list>
#include "strings.h"
#include "../Geometries/Geometry.h"

/** The buffer length into which any MPI-communicated string must be imprint, The original
    string is padded with zero-value characters if it is too short, or trimmed if too long. */
const size_t StringsImprintSize = 256;


std::ostream& operator<<(std::ostream& s, const hashedString& hs)
{
	s << hs.getString();
	return s;
}


// ------------- StringsDictionary methods -------------
const std::string& StringsDictionary::translateIdToString(const size_t ID) const
{
	//(re)use iterator from find (both in another find() and in return)
	auto resFind = knownDictionary.find(ID);
	if (resFind != knownDictionary.end())
	{
		return resFind->second;
	}
	else
	{
		resFind = newDictionary.find(ID);
		if (resFind != newDictionary.end())
		{
			return resFind->second;
		}
		else
		{
			//we have to return a string but here we have none at hand,
			//so we sadly have to throw an exception
			throw ERROR_REPORT("String with ID/hash "
			  << ID << " is not existing in the dictionary.");
		}
	}
}


void StringsDictionary::registerThisString(const std::string& s)
{
	const size_t ID = hashedString::hash(s);

	//don't add anything if it is already in the Dictionary
	if (  newDictionary.find(ID) == newDictionary.end()
	 && knownDictionary.find(ID) == knownDictionary.end() )
		newDictionary[ID] = s;
}


void StringsDictionary::registerThisString(const hashedString& s)
{
	//don't add anything if it is already in the Dictionary
	if (  newDictionary.find(s.getHash()) == newDictionary.end()
	 && knownDictionary.find(s.getHash()) == knownDictionary.end() )
		newDictionary[s.getHash()] = s.getString();
}


void StringsDictionary::markAllWasBroadcast()
{
	for (const auto& nItem : newDictionary)
	{
#ifdef DEBUG
		if (knownDictionary.find(nItem.first) != knownDictionary.end())
			REPORT("new to already known >>" << nItem.second << "<<");
#endif
		knownDictionary[nItem.first] = nItem.second;
	}
	newDictionary.clear();
}


void StringsDictionary::enlistTheIncomingItem(const size_t hash, const std::string& string)
{
	if (knownDictionary.find(hash) == knownDictionary.end())
	{
		//new item
		knownDictionary[hash] = string;
	}
	else
	{
		DEBUG_REPORT("received already known >>" << string << "<<");

		//known item (IDs/hashes are matching), check that strings are matching too
		if (string.compare(knownDictionary[hash]) != 0)
			throw ERROR_REPORT("Hashing malfunction: Have >>" << knownDictionary[hash] << "<< and got >>"
			  << string << "<<, both of the same hash = " << hash);
	}
}


void StringsDictionary::cleanUp(const std::list<NamedAxisAlignedBoundingBox>& AABBs)
{
	auto dict = knownDictionary.begin();
	while (dict != knownDictionary.end())
	{
		//is there a box with the same nameID?
		bool found = false;
		for (auto naabb : AABBs)
			if (dict->first == naabb.nameID) { found = true; break; }

		if (!found) dict = knownDictionary.erase(dict);
		else ++dict;
	}
}
