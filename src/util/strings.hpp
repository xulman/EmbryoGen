#pragma once

#include "../Geometries/Geometry.hpp"
#include "report.hpp"
#include <list>
#include <map>
#include <string>
class FrontOfficer;

/** The buffer length into which any MPI-communicated string must be imprint,
   The original string is padded with zero-value characters if it is too short,
   or trimmed if too long. */
extern const size_t StringsImprintSize;

/** The class that pairs std::string with its hash, that is computed with
   hashedString::hash(). While the hash can be always computed on on-demand
   basis, we opted to have it cached here. In order to maintain the validity of
   the hash, the class "exports" the std::string only in a const (read-only)
   regime. If one wants to adjust the string, she has to read it out, modify its
   copy and import this copy with some of the offered methods. The "import"
   methods of course update the hash accordingly. The hash is computed
   immediately, not lazily, allowing the caller to pass object of this class as
   const reference immediately after the import. */
class hashedString {
  private:
	/** hash of the this->_string, intentionally private so that it
	    appears as read-only to the outside world */
	size_t _hash = 0;

	/** the string, intentionally private so that it
	    can be modified only in a controlled way */
	std::string _string;

	/** a shortcut method to maintain the hash-string pairing */
	inline void updateHash() { _hash = hash(_string); }

  public:
	/** this is the definition of the strings hashing function */
	static inline size_t hash(const std::string& s) {
		return std::hash<std::string>{}(s);
	}

	hashedString() {}

	hashedString(const std::string& s) : _string(s) { updateHash(); }

	hashedString(const char* s) : _string(s) { updateHash(); }

	// note there exists an implicit operator=(hashedString&) which
	// is a copy c'tor from anything that can construct hashedString

	void setString(const std::string& s) {
		_string = s;
		updateHash();
	}

	hashedString& append(const std::string& s) // supports calls chaining
	{
		_string.append(s);
		updateHash();
		return *this;
	}

	hashedString& operator<<(const std::string& s) // supports calls chaining
	{
		_string.append(s);
		updateHash();
		return *this;
	}

	size_t getHash() const { return _hash; }

	const std::string& getString() const { return _string; }

	/** runs the static printIntoBuffer() with this->_string */
	void printIntoBuffer(char* buffer,
	                     const size_t bufLength,
	                     const char padding = 0) const {
		printIntoBuffer(_string, buffer, bufLength, padding);
	}

	/** prints 'sourceStr' into fixed length buffer by, possibly, trimming the
	   input to fit in length, or extending/padding with zero-valued chars to
	   fill the 'buffer' completely */
	static void printIntoBuffer(const std::string& sourceStr,
	                            char* buffer,
	                            const size_t bufLength,
	                            const char padding = 0) {
		// copy and, possibly, trim
		const size_t copySize = std::min(sourceStr.length(), bufLength);
		memcpy((void*)buffer, (void*)sourceStr.c_str(), copySize);

		if (copySize < bufLength)
			memset((void*)(buffer + copySize), padding, bufLength - copySize);
	}
};

std::ostream& operator<<(std::ostream& s, const hashedString& hs);
// defined in strings.cpp (to make linker happy)

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
class StringsDictionary {
  private:
	/** the Dictionary itself is an union of the non-intersecting synced-already
	    and to-be-synced sub-dictionaries */
	std::map<size_t, std::string> knownDictionary, newDictionary;

	// -------------- for agents --------------
  public:
	const std::string& translateIdToString(const size_t ID) const;

	void registerThisString(const std::string& s);
	void registerThisString(const hashedString& s);

	// -------------- for broadcasting --------------
  protected:
	/** sending: returns the number of items added recently since the last
	 * markAllWasBroadcast() */
	size_t howManyShouldBeBroadcast() const { return newDictionary.size(); }

	/** sending: lists the items added recently since the last
	 * markAllWasBroadcast() */
	const std::map<size_t, std::string>& theseShouldBeBroadcast() const {
		return newDictionary;
	}

	/** sending: marks that the recently added items were transferred (to some
	   other Dictionaries to have all of them synchronized); calling
	   howManyShouldBeBroadcast() right after this method will return with 0
	   (zero). */
	void markAllWasBroadcast();

	/** receiving: enlist the given dictionary item (ID,string) directly into
	   knownDictionary; if an item of the same ID is already existing in local
	   Dictionary, check that the given string and already stored string are the
	   same -- consistency checking of the hashes */
	void enlistTheIncomingItem(const size_t hash, const std::string& string);

	/** receiving: enlist the given string, that is, calculate its hash (pretend
	   a full item has arrived) and call the other enlistTheIncomingItem() */
	void enlistTheIncomingItem(const std::string& string) {
		enlistTheIncomingItem(hashedString::hash(string), string);
	}

	/** after sending&receiving, and only after markAllWasBroadcast():
	    it manipulates only this->knownDictionary and removes every dictionary
	   item from it that has no counterpart in the given list -- this has nasty
	   complexity consequence (as list can do only O(n) search), but it prevents
	   from Dictionary growing excessively -> use, but sparsely */
	void cleanUp(const std::list<NamedAxisAlignedBoundingBox>& AABBs);

	/** ugly hack to allow FrontOfficer to reach the protected methods without
	    making them public -- so agents cannot break things because they can
	    operate only on the public methods */
	friend class FrontOfficer;

	// -------------- for reporting --------------
  public:
	const std::map<size_t, std::string>& showKnownDictionary() const {
		return knownDictionary;
	}
	const std::map<size_t, std::string>& showNewDictionary() const {
		return newDictionary;
	}

	void printKnownDictionary() const {
		report::message(fmt::format("Known (already synced) fraction:"));
		for (const auto& i : knownDictionary)
			report::message(fmt::format("{}\t{}", i.first, i.second), {false});
	}
	void printNewDictionary() const {
		report::message(fmt::format("New (to be synced) fraction:"));
		for (const auto& i : newDictionary)
			report::message(fmt::format("{}\t{}", i.first, i.second), {false});
	}

	void printCompleteDictionary() const {
		printKnownDictionary();
		printNewDictionary();
	}
};
