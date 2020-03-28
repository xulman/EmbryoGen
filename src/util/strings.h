#ifndef STRINGS_H
#define STRINGS_H

#include <string>
#include <map>
#include "report.h"

/** The buffer length into which any MPI-communicated string must be imprint, The original
    string is padded with zero-value characters if it is too short, or trimmed if too long. */
const size_t StringsImprintSize = 256;

/** The class that pairs std::string with its hash, that is computed with hashedString::hash().
    While the hash can be always computed on on-demand basis, we opted to have it cached here.
    In order to maintain the validity of the hash, the class "exports" the std::string only
    in a const (read-only) regime. If one wants to adjust the string, she has to read it out,
    modify its copy and import this copy with some of the offered methods. The "import" methods
    of course update the hash accordingly. The hash is computed immediately, not lazily, allowing
    the caller to pass object of this class as const reference immediately after the import. */
class hashedString
{
private:
	/** hash of the this->_string, intentionally private so that it
	    appears as read-only to the outside world */
	size_t _hash = 0;

	/** the string, intentionally private so that it
	    can be modified only in a controlled way */
	std::string _string;

	/** a shortcut method to maintain the hash-string pairing */
	inline void updateHash()
	{ _hash = hash(_string); }

public:
	/** this is the definition of the strings hashing function */
	static inline size_t hash(const std::string& s)
	{ return std::hash<std::string>{}(s); }

	hashedString() {}

	hashedString(const std::string& s) : _string(s)
	{ updateHash(); }

	hashedString(const char* s) : _string(s)
	{ updateHash(); }

	//note there exists an implicit operator=(hashedString&) which
	//is a copy c'tor from anything that can construct hashedString

	void setString(const std::string& s)
	{ _string = s; updateHash(); }

	hashedString& append(const std::string& s)        //supports calls chaining
	{ _string.append(s); updateHash(); return *this; }

	hashedString& operator<<(const std::string& s)    //supports calls chaining
	{ _string.append(s); updateHash(); return *this; }


	size_t getHash() const
	{ return _hash; }

	const std::string& getString() const
	{ return _string; }


	/** prints this->_string into fixed length buffer by, possibly, trimming the input to fit
	    in length, or extending/padding with zero-valued chars to fill the 'buffer' completely */
	void printIntoBuffer(char* buffer, const size_t bufLength, const char padding = 0) const
	{
		//copy and, possibly, trim
		const size_t copySize = std::min(_string.length(), bufLength);
		memcpy((void*)buffer, (void*)_string.c_str(), copySize);

		if (copySize < bufLength) memset((void*)(buffer+copySize), padding, bufLength-copySize);
	}
};


/** A Dictionary of std::strings and their IDs. Technically, it is
    a map from ID to std::string. The original purpose for this to exist was
    to replace variable-length strings with fixed-length IDs as an effective
    mean to reference and/or exchange the strings between multiple peers,
    assuming that Dictionaries in all peers are synchronized.

    Dictionary cannot remove any item, it is a growing-only container.

    To facilitate the synchronization, the Dictionary is recording all newly
    added items since last synchronization event, and offers an immutable
    collection of these for the caller to implement their transfers between
    the peers (and their Dictionaries).

    Classes, that transfer data in order to implement the synchronization of
    Dictionaries, must extend this class in order to reach the set of protected
    methods that are devoted to the synchronization as such.

    Simulation agents just call directly the public methods. */
class StringsDictionary
{
private:
	/** the Dictionary itself is an union of the non-intersecting synced-already
	    and to-be-synced sub-dictionaries */
	std::map<size_t, std::string> knownDictionary, newDictionary;

public:
	const std::string& translateIdToString(const size_t ID) const
	{
		if (knownDictionary.find(ID) != knownDictionary.end())
		{
			return knownDictionary.at(ID);
		}
		else
		{
			if (newDictionary.find(ID) != newDictionary.end())
			{
				return newDictionary.at(ID);
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

	void registerThisString(const std::string& s)
	{
		const size_t ID = hashedString::hash(s);

		//don't add anything if it is already in the Dictionary
		if (  newDictionary.find(ID) == newDictionary.end()
		 && knownDictionary.find(ID) == knownDictionary.end() )
			newDictionary[ID] = s;
	}

	void registerThisString(const hashedString& s)
	{
		//don't add anything if it is already in the Dictionary
		if (  newDictionary.find(s.getHash()) == newDictionary.end()
		 && knownDictionary.find(s.getHash()) == knownDictionary.end() )
			newDictionary[s.getHash()] = s.getString();
	}


	// -------------- for reporting --------------
	const std::map<size_t, std::string>& showKnownDictionary() const { return knownDictionary; }
	const std::map<size_t, std::string>& showNewDictionary() const { return newDictionary; }

	void printKnownDictionary() const
	{
		REPORT("Known (already synced) fraction:");
		for (const auto& i : knownDictionary)
			REPORT_NOHEADER(i.first << "\t" << i.second);
	}
	void printNewDictionary() const
	{
		REPORT("New (to be synced) fraction:");
		for (const auto& i : newDictionary)
			REPORT_NOHEADER(i.first << "\t" << i.second);
	}

	void printCompleteDictionary() const
	{
		printKnownDictionary();
		printNewDictionary();
	}


	// -------------- for broadcasting --------------
protected:
	/** sending: returns the number of items added recently since the last markAllWasBroadcast() */
	size_t howManyShouldBeBroadcast() const
	{
		return newDictionary.size();
	}

	/** sending: lists the items added recently since the last markAllWasBroadcast() */
	const std::map<size_t, std::string>& theseShouldBeBroadcast() const
	{
		return newDictionary;
	}

	/** sending: marks that the recently added items were transferred (to some other
	    Dictionaries to have all of them synchronized); calling howManyShouldBeBroadcast()
	    right after this method will return with 0 (zero). */
	void markAllWasBroadcast()
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

	/** receiving: enlist the given dictionary item (ID,string) directly into knownDictionary;
	    if an item of the same ID is already existing in local Dictionary, check that the given
	    string and already stored string are the same -- consistency checking of the hashes */
	void enlistTheIncomingItem(const size_t hash, const std::string& string)
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

	/** receiving: enlist the given string, that is, calculate its hash (pretend a full item
	    has arrived) and call the other enlistTheIncomingItem() */
	void enlistTheIncomingItem(const std::string& string)
	{
		enlistTheIncomingItem(hashedString::hash(string),string);
	}
};
#endif
