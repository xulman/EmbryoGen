#pragma once

#include <functional>
#include <iostream>
#include <map>
#include <vector>

//    encoding labels, original labels
template <typename EL, typename OL>
struct LabelSet {
	// this struct represents one encoding label, that is, one particular
	// combination of original labels; it is not a collection of already
	// existing encoding labels

	// the original labels are always stored in non-decreasing order of
	// their values to facilitate their mutual comparisons

	// construct fake empty encoding label (std::map< ?,LabelSet<> > needs it)
	LabelSet(void) : label(0), array() {}

	// construct from single original label
	LabelSet(const EL eLabel, const OL a) : label(eLabel), array{{a}} {}

	// construct from two original labels
	LabelSet(const EL eLabel, const OL a, const OL b) : label(eLabel), array() {
		array.reserve(2);

		if (a < b) {
			array.push_back(a);
			if (a != b)
				array.push_back(b);
		} else {
			array.push_back(b);
			if (a != b)
				array.push_back(a);
		}
	}

	// construct by adding original label to some existing ones;
	// if 'b' is already present in 'ml', the constructed LabelSet
	// will be the same as 'ml' except for label and usageCnt attributes
	LabelSet(const EL eLabel, const LabelSet<EL, OL>& ml, const OL b)
	    : label(eLabel), array() {
		array.reserve(ml.array.size() + 1);

		std::size_t i = 0;
		while (i < ml.array.size() && ml.array[i] < b)
			array.push_back(ml.array[i++]);

		// add 'b' only if not previously seen in the ml.array
		if (i < ml.array.size() && ml.array[i] > b)
			array.push_back(b);

		while (i < ml.array.size())
			array.push_back(ml.array[i++]);
	}

	/// the encoding label -- represents the list of some original labels
	const EL label;

	/// the list of some original labels -- represented with this encoding label
	std::vector<OL> array;
	// TODO make it read-only -- either as reference to be created from
	// read-write array, or make private and provide public inline operator[]
	// const()

	/// optional user statistics
	std::size_t usageCnt = 0;

	/// just prints the content of this set
	void print(void) const {
		for (OL l : array)
			std::cout << l << ",";
	}
};

//    encoding labels, original labels
template <typename EL, typename OL>
class MultiLabels {
  public:
	MultiLabels(const EL firstEncodingLabel)
	    : firstEncodingLabel(firstEncodingLabel),
	      lastUsedEncodingLabel(firstEncodingLabel - 1), labelSets() {}

	// returns an encoding label that
	// encapsulates 'foundEL' encoding label together with 'insertingOL'
	EL add(const EL foundEL, const OL insertingOL) {
		// adding to previously empty voxel?
		if (foundEL == 0)
			return (EL)insertingOL;

		// adding to voxel that contained just one original label?
		if (foundEL < firstEncodingLabel) {
			// adding over myself...
			if (foundEL == (EL)insertingOL)
				return foundEL;

			// construct the desired (and possibly new) multilabel
			LabelSet<EL, OL> possiblyNewEL(lastUsedEncodingLabel + 1, foundEL,
			                               insertingOL);
			return add(possiblyNewEL);
		}

		// adding to voxel that contained already multiple (2+) original labels
		const LabelSet<EL, OL>* existingEL = getLabelSet(foundEL);
		if (existingEL == NULL) {
			std::cout << "SOMETHING VERY WRONG HERE #1\n";
			return 0;
		}

		// construct the desired (and possibly new) multilabel
		LabelSet<EL, OL> possiblyNewEL(lastUsedEncodingLabel + 1, *existingEL,
		                               insertingOL);
		return add(possiblyNewEL);
	}

	EL addAndUpdateUsage(const EL foundEL, const OL insertingOL) {
		const EL retEL = add(foundEL, insertingOL);

		// new usage case for 'retEL' (as long as it is a LabelSet)
		if (retEL >= firstEncodingLabel)
			labelSets[retEL].usageCnt++;

		// lost usage for 'foundEL' (as long as it is a different LabelSet)
		if (foundEL >= firstEncodingLabel && foundEL != retEL)
			labelSets[foundEL].usageCnt--;

		return retEL;

		// TODO: notify all owners of the original labels that were in
		// *existingEL
		//       that they are now represented with encoding label that is going
		//       to be returned from this function call
	}

	bool isLabelIncluded(const EL foundEL, const OL queriedOL) {
		// single label case
		if (foundEL < firstEncodingLabel)
			return (foundEL == (EL)queriedOL);

		// multiple (encoded) label case
		for (const OL l : labelSets[foundEL].array)
			if (l == queriedOL)
				return true;

		return false;
	}

	const LabelSet<EL, OL>* getLabelSet(const EL someEL) const {
		const auto& ml = labelSets.find(someEL);
		return (ml != labelSets.end() ? &(ml->second) : NULL);
	}

	const LabelSet<EL, OL>* getLabelSet(const std::vector<OL>& someOLs) const {
		for (const auto& ml : labelSets) {
			if (ml.second.array.size() != someOLs.size())
				continue;

			std::size_t i = 0;
			for (; i < someOLs.size(); ++i)
				if (ml.second.array[i] != someOLs[i])
					break;

			if (i == someOLs.size())
				return &(ml.second);
		}

		return NULL;
	}

	void printStatus(void) const {
		std::cout << "started with encoding label: " << firstEncodingLabel
		          << "\n";
		std::cout << "last used encoding label   : " << lastUsedEncodingLabel
		          << "\n";

		EL reallyUsedELs = 0;
		for (const auto& ml : labelSets) {
			const LabelSet<OL, EL>& mls = ml.second;

			std::cout << "EL " << mls.label << " encodes ";
			mls.print();
			std::cout << " usage=" << mls.usageCnt << "\n";

			reallyUsedELs += mls.usageCnt > 0 ? 1 : 0;
		}

		EL usedELs = lastUsedEncodingLabel >= firstEncodingLabel
		                 ? lastUsedEncodingLabel - firstEncodingLabel + 1
		                 : 0;

		std::cout << "should hold: " << reallyUsedELs << " <= " << usedELs
		          << "\n";
	}

	const std::map<EL, LabelSet<EL, OL>>& getLabelSets(void) const {
		return labelSets;
	}

  private:
	const EL firstEncodingLabel;
	EL lastUsedEncodingLabel;

	// this is a collection of already existing encoding labels
	// NB: could have been set but set does not provide non-const iterators
	std::map<EL, LabelSet<EL, OL>> labelSets;

	EL add(LabelSet<EL, OL>& possiblyNewEL) {
		// check if 'possiblyNewEL' is not already recognized
		const LabelSet<EL, OL>* ml = getLabelSet(possiblyNewEL.array);

		if (ml == NULL) {
			// not recognized, indeed a new LabelSet
			++lastUsedEncodingLabel; // NB: now matches/equals
			                         // possiblyNewEL.label
			labelSets.emplace(lastUsedEncodingLabel, possiblyNewEL);
			return lastUsedEncodingLabel;
		} else {
			// recognized, return that label
			return ml->label;
		}
	}
};
