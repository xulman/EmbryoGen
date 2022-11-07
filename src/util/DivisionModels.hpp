#pragma once

#include "report.hpp"
#include "rnd_generators.hpp"
#include <cmath>
#include <fmt/core.h>
#include <fstream>
#include <iostream>
#include <vector>

/** timelapse configurations of sphere-based geometry (for agents
    made of N spheres) that consists of TB (Before division) configurations
    for mother and TA (After dividion) configurations for D daughters */
template <int N, int TB, int TA, int D>
struct DivisionModel {
	// N  = how many spheres are involved
	// TB = how many timepoints Before division
	// TA = how many timepoints After division
	// D  = how many daughters

	/** series of "delta times" (in minutes) before the division occurs
	    at which the model's model is established */
	float Mtimes[TB];

	float Mradii[TB][N];
	float Mdists[TB][N - 1];

	/** series of "delta times" (in minutes) after the division occurs
	    at which the daughter's model is established */
	float Dtimes[D][TA];

	float Dradii[D][TA][N];
	float Ddists[D][TA][N - 1];

	void printModel() const {
		for (int i = 0; i < TB; ++i) {
			report::message(fmt::format("mother at {}: {}", Mtimes[i],
			                            Mradii[i][0], {true, false, false}));
			for (int n = 0; n < N - 1; ++n)
				report::message(
				    fmt::format("-{}-{}", Mdists[i][n], Mradii[i][n + 1]),
				    {false, false, false});
			report::message("", {false, true});
		}
		report::message(
		    fmt::format("# division here (DBG entry ptr={})", long(Mtimes)));

		for (int d = 0; d < D; ++d)
			for (int i = 0; i < TA; ++i) {
				report::message(fmt::format("daughter{} at {}: {}", d + 1,
				                            Dtimes[d][i], Dradii[d][i][0]),
				                {true, false});
				for (int n = 0; n < N - 1; ++n)
					report::message(fmt::format("-{}-{}", Ddists[d][i][n],
					                            Dradii[d][i][n + 1]),
					                {false, false});
				report::message("", {false, true});
			}
	}

	float getMotherRadius(const float time, const int sphereNo) const {
#ifndef NDEBUG
		if (time >= 0)
			report::message("WARNING: Mother's configuration not available for "
			                "positive time.");
#endif
		return getArrayValue<TB, N>(time, sphereNo, Mtimes, Mradii);
	}

	float getMotherDist(const float time, const int sphereNo) const {
#ifndef NDEBUG
		if (time >= 0)
			report::message("WARNING: Mother's configuration not available for "
			                "positive time.");
#endif
		return getArrayValue<TB, N - 1>(time, sphereNo, Mtimes, Mdists);
	}

	float getDaughterRadius(const float time,
	                        const int daughterNo,
	                        const int sphereNo) const {
#ifndef NDEBUG
		if (time < 0)
			report::message("WARNING: Daughter's configuration not available "
			                "for negative time.");
		if (daughterNo < 0 || daughterNo >= D)
			throw report::rtError(fmt::format(
			    "Wrong index ({}) of the asked daughter.", daughterNo));
#endif
		return getArrayValue<TA, N>(time, sphereNo, Dtimes[daughterNo],
		                            Dradii[daughterNo]);
	}

	float getDaughterDist(const float time,
	                      const int daughterNo,
	                      const int sphereNo) const {
#ifndef NDEBUG
		if (time < 0)
			report::message("WARNING: Daughter's configuration not available "
			                "for negative time.");
		if (daughterNo < 0 || daughterNo >= D)
			throw report::rtError(fmt::format(
			    "Wrong index ({}) of the asked daughter.", daughterNo));
#endif
		return getArrayValue<TA, N - 1>(time, sphereNo, Dtimes[daughterNo],
		                                Ddists[daughterNo]);
	}

  private:
	template <int T, int I> // T = no. of Time points, I = no. of Items
	                        // (Spheres) per time point
	float getArrayValue(const float time,
	                    const int itemIdx,
	                    const float times[T],
	                    const float values[T][I]) const {
#ifndef NDEBUG
		if (itemIdx < 0 || itemIdx >= I)
			throw report::rtError(
			    fmt::format("Wrong index ({}) of the asked sphere.", itemIdx));
#endif

		//(time-wise) before the values? never mind, return the first value
		if (time <= times[0])
			return values[0][itemIdx];

		int i = 1;
		while (i < T && time > times[i])
			++i;
		if (i < T)
			return getInterpolatedValue(times[i - 1], time, times[i],
			                            values[i - 1][itemIdx],
			                            values[i][itemIdx]);
		else
			//(time-wise) after the values, return the last value then
			return values[T - 1][itemIdx];
	}

	float getInterpolatedValue(const float posA,
	                           const float pos,
	                           const float posB,
	                           const float valA,
	                           const float valB) const {
		float wA = (posB - pos) / (posB - posA);
		return (wA * valA + (1.f - wA) * valB);
	}
};

/* example file for N=2, TB=2, TA=2, D=2
# timepoint
#  role (0 - mother, 1 - left daughter, 2 - right daughter)
#    leftID
#        rightID
#           leftRadius
#                             centreDistance
#                                               rightRadius

# first instance
49 0 317 49 21.43588810000003 2.2902253805749173 21.43588810000003
50 0 52 50 20.438317370604814 7.538759466241433 20.43831737060481
# division here at timepoint 51
51 2 51 54 16.105100000000025 7.760972927959179 16.891171380665135
51 1 70 69 10.488088481701531 12.121693026062871 11.000000000000016
52 2 62 55 17.71561000000003 9.739686675217522 16.105100000000025
52 1 77 53 12.10000000000002 7.00834676074424 12.10000000000002

# second instance, leave at least one blank line to separate instances
55 0 206 218 16.105100000000043 23.982937211386194 13.310000000000029
56 0 207 210 16.105100000000046 24.6379098491594 13.310000000000029
# division here at timepoint 57
57 2 219 220 12.69058706285886 14.925289993936484 12.100000000000026
57 1 208 221 14.64100000000004 11.068127778263854 16.891171380665156
58 2 240 233 12.69058706285886 15.350205891912541 12.100000000000026
58 1 209 222 14.641000000000044 11.409328841363463 17.71561000000005
*/
template <int N, int TB, int TA, int D>
class DivisionModels {
  public:
	typedef DivisionModel<N, TB, TA, D> DivModelType;

	std::vector<DivModelType> models;
	const float timeStep;

	DivisionModels(const char* filename, const float _timeStep = 1.0f)
	    : timeStep(_timeStep) {
		models.reserve(20);
		std::ifstream f(filename);

		while (rewindToNextDataBlock(f)) {
			models.emplace_back();
			readOneDataBlock(f, models.back());
		}

		f.close();
	}

	// ---------------------- immutable original models ----------------------
	const DivModelType& getModel(const int id) const {
		try {
			return models.at(id);
		} catch (std::out_of_range* e) {
			throw report::rtError(
			    fmt::format("Out of range with model id {}", id));
		}
	}

	const DivModelType& getRandomModel() const {
		const int id = (int)GetRandomUniform(0, (float)models.size() - 0.001f);
		return getModel(id);
	}

	// ---------------------- mutable original models ----------------------
	DivModelType& getModel(const int id) {
		try {
			return models.at(id);
		} catch (std::out_of_range* e) {
			throw report::rtError(
			    fmt::format("Out of range with model id {}", id));
		}
	}

	DivModelType& getRandomModel() {
		const int id = (int)GetRandomUniform(0, (float)models.size() - 0.001f);
		return getModel(id);
	}

	// ---------------------- mutable artificial (mixed) models
	// ----------------------
	/** friendly front-end method to obtain mixed model from the given number of
	   original models, the mixing is done with flat weights which effectively
	   emulated averaging */
	void getAveragedModel(
	    DivModelType& m /*, const int fromThisNoOfRandomModels = 2 */) const {
		constexpr int fromThisNoOfRandomModels = 2;

		float weights[TB + TA][fromThisNoOfRandomModels];
		setFlatWeights<TB + TA, fromThisNoOfRandomModels>(weights);

		getMixedFromRandomModels<TB + TA, fromThisNoOfRandomModels>(m, weights);
	}

	/** friendly front-end method to obtain mixed model from the given number of
	   original models, the mixing is done with gaussian shaped weights where
	   the mean of the gaussian is shifting between time points which
	   effectively emulates merging dominant model with the remaining models
	   while the choice of the dominant model is changing while the
	   resulting/output model is being built */
	void getShiftingGaussWeightedModel(
	    DivModelType& m /*, const int fromThisNoOfRandomModels = 4*/) const {
		constexpr int fromThisNoOfRandomModels = 4;

		float weights[TB + TA][fromThisNoOfRandomModels];
		setSineShiftingGaussWeights<TB + TA, fromThisNoOfRandomModels>(weights);

		getMixedFromRandomModels<TB + TA, fromThisNoOfRandomModels>(m, weights);
	}

	/** in principle the same as getGaussRndWalkedModel() except that a peak
	   (box of width = 1) is representing the currently dominant model and that
	   is shifted, the peak weight should be boxAmpl-times larger than the
	   non-peaked weights */
	void getShiftingBoxWeightedModel(
	    DivModelType& m,
	    const float boxAmpl =
	        3 /*, const int fromThisNoOfRandomModels = 4*/) const {
		constexpr int fromThisNoOfRandomModels = 4;

		float weights[TB + TA][fromThisNoOfRandomModels];
		setSineShiftingBoxWeights<TB + TA, fromThisNoOfRandomModels>(weights,
		                                                             boxAmpl);

		getMixedFromRandomModels<TB + TA, fromThisNoOfRandomModels>(m, weights);
	}

	/** provides a model averaged from randomly (with repetition) chosen
	   original models and mixes them according to the given weights */
	template <int T,
	          int M> // T = no. of Time points, M = no. of models to be mixed
	void getMixedFromRandomModels(DivModelType& m,
	                              const float weights[T][M]) const {
		// list of reference models to be mixed together
		const DivModelType* models[M];
		for (int i = 0; i < M; ++i)
			models[i] = &getRandomModel();

		getMixedFromGivenModels<T, M>(m, weights, models);
	}

	/** provides a model averaged from user given models and mixes them
	   according to the user given weights -- this is the workhorse method */
	template <int T,
	          int M> // T = no. of Time points, M = no. of models to be mixed
	void getMixedFromGivenModels(DivModelType& m,
	                             const float weights[T][M],
	                             const DivModelType* models[M]) const {
		// times will be taken as such from the first reference model
		for (int t = 0; t < TB; ++t)
			m.Mtimes[t] = models[0]->Mtimes[t];

		// values of Radii will be mixed
		for (int t = 0; t < TB; ++t)
			for (int s = 0; s < N; ++s) {
				m.Mradii[t][s] = weights[t][0] * models[0]->Mradii[t][s];
				for (int n = 1; n < M; ++n)
					m.Mradii[t][s] +=
					    weights[t][n] *
					    models[n]->getMotherRadius(m.Mtimes[t], s);
			}

		// values of Distances will be mixed
		for (int t = 0; t < TB; ++t)
			for (int s = 0; s < N - 1; ++s) {
				m.Mdists[t][s] = weights[t][0] * models[0]->Mdists[t][s];
				for (int n = 1; n < M; ++n)
					m.Mdists[t][s] += weights[t][n] *
					                  models[n]->getMotherDist(m.Mtimes[t], s);
			}

		// times will be taken as such from the first reference model
		for (int d = 0; d < D; ++d)
			for (int t = 0; t < TA; ++t)
				m.Dtimes[d][t] = models[0]->Dtimes[d][t];

		// values of Radii will be mixed
		for (int d = 0; d < D; ++d)
			for (int t = 0; t < TA; ++t)
				for (int s = 0; s < N; ++s) {
					m.Dradii[d][t][s] =
					    weights[t][0] * models[0]->Dradii[d][t][s];
					for (int n = 1; n < M; ++n)
						m.Dradii[d][t][s] +=
						    weights[t][n] *
						    models[n]->getDaughterRadius(m.Dtimes[d][t], d, s);
				}

		// values of Distances will be mixed
		for (int d = 0; d < D; ++d)
			for (int t = 0; t < TA; ++t)
				for (int s = 0; s < N - 1; ++s) {
					m.Ddists[d][t][s] =
					    weights[t][0] * models[0]->Ddists[d][t][s];
					for (int n = 1; n < M; ++n)
						m.Ddists[d][t][s] +=
						    weights[t][n] *
						    models[n]->getDaughterDist(m.Dtimes[d][t], d, s);
				}
	}

	template <int T,
	          int M> // T = no. of Time points, M = no. of models to be mixed
	void setFlatWeights(float w[T][M]) const {
		// it must hold Sum w[fixed_t][*] = 1
		for (int t = 0; t < T; ++t)
			for (int m = 0; m < M; ++m)
				w[t][m] = float(1.f / M);
	}

	template <int T,
	          int M> // T = no. of Time points, M = no. of models to be mixed
	void setSineShiftingGaussWeights(float w[T][M]) const {
		// it must hold Sum w[fixed_t][*] = 1
		for (int t = 0; t < T; ++t) {
			const float pos =
			    (M - 1.f) * (0.5f * std::cos(6.28 * t / T) + 0.5f);
			setGaussWeights<M>(w[t], pos);
		}
	}

	template <int T,
	          int M> // T = no. of Time points, M = no. of models to be mixed
	void setSineShiftingBoxWeights(float w[T][M],
	                               const float boxAmpl = 3) const {
		// it must hold Sum w[fixed_t][*] = 1
		for (int t = 0; t < T; ++t) {
			const float pos =
			    (M - 1.f) * (0.5f * std::cos(6.28 * t / T) + 0.5f);
			setBoxWeights<M>(w[t], pos, boxAmpl);
		}
	}

	/** meanPos MUST BE [0,M-1], esp. it MUST NOT BE negative */
	template <int M>
	void setGaussWeights(float w[M], const float meanPos) const {
		// factor = -0.5 / sigma*sigma  and  M = 4*sigma
		const double factor = -0.5 * (4 * 4) / double(M * M);

		double sum = 0, dp;
		for (int m = 0; m < M; ++m) {
			dp = meanPos - (double)m; //(signed) distance
			if (dp < 0)
				dp = -dp;              // abs()
			dp = std::min(dp, M - dp); // handle circularity
			dp *= dp;

			dp = std::exp(factor * dp * dp);
			w[m] = (float)dp;
			sum += dp;
		}
		for (int m = 0; m < M; ++m)
			w[m] /= (float)sum;
	}

	/** boxPos MUST BE [0,M-1], esp. it MUST NOT BE negative */
	template <int M>
	void setBoxWeights(float w[M],
	                   const float boxPos,
	                   const float boxMagnitude) const {
		const float norm = 1.f / (float(M) - 1.f + boxMagnitude);
		for (int m = 0; m < M; ++m)
			w[m] = norm;

		// spread the extra "(boxMagnitude-1) * norm" value onto weights that
		// are around the boxPos
		const float remainder = boxPos - std::floor(boxPos);
		w[(int)boxPos] += (boxMagnitude - 1.f) * norm * (1.f - remainder);
		w[((int)boxPos + 1) % M] += (boxMagnitude - 1.f) * norm * remainder;
	}

	template <int T,
	          int M> // T = no. of Time points, M = no. of models to be mixed
	void printWeights(const float w[T][M]) const {
		for (int t = 0; t < T; ++t) {
			double sum = 0;
			report::message(fmt::format("{}:", t), {true, false});
			for (int m = 0; m < M; ++m) {
				report::message(fmt::format("\t{}", w[t][m]), {false, false});
				sum += w[t][m];
			}
			report::message(fmt::format("\t(sum = {})", sum), {false});
		}
	}

	template <int T,
	          int M> // T = no. of Time points, M = no. of models to be mixed
	void printWeightsWithModelsVertically(const float w[T][M]) const {
		double sum[T];
		for (int t = 0; t < T; ++t)
			sum[t] = 0;

		for (int m = 0; m < M; ++m) {
			report::message(fmt::format("{}:", m), {true, false});
			for (int t = 0; t < T; ++t) {
				report::message(fmt::format("\t{}", w[t][m]), {false, false});
				sum[t] += w[t][m];
			}
			report::message("", {false, true});
		}

		report::message("sums:", {true, false});
		for (int t = 0; t < T; ++t)
			report::message(fmt::format("\t{}", sum[t]), {false, false});
		report::message("", {false, true});
	}

	// ---------------------- IO stuff ----------------------
	void printModel(const int id) const {
		try {
			models.at(id).printModel();
		} catch (std::out_of_range* e) {
			throw report::rtError(
			    fmt::format("Out of range with model id {}", id));
		}
	}

	bool rewindToNextDataBlock(std::ifstream& f) const {
		int firstChar;
		char skipLine[1024];

		firstChar = f.get();
		while (firstChar != std::ifstream::traits_type::eof() &&
		       !isdigit(firstChar)) {
			f.putback(static_cast<char>(firstChar));
			f.getline(skipLine, 1024);
			firstChar = f.get();
		}
		if (firstChar != std::ifstream::traits_type::eof()) {
			f.putback(static_cast<char>(firstChar));
			// std::cout << "scrolled down till: " << (char)firstChar << "\n";
			return true;
		}

		// std::cout << "reached end of file\n";
		return false;
	}

	void readOneDataBlock(std::ifstream& f, DivModelType& dm) const {
		int firstChar;
		char skipLine[512];

		// number of processed lines for every part of the model: 0 (mother) + D
		// daughters
		int lines[D + 1];
		for (int d = 0; d < D + 1; ++d)
			lines[d] = 0;

		firstChar = f.get();
		while (firstChar != std::ifstream::traits_type::eof() &&
		       std::isdigit(firstChar)) {
			f.putback(static_cast<char>(firstChar));

			// read the line's first part
			float time, spotA, spotB, r, dist;
			int role;
			f >> time >> role >> spotA >> spotB;
			// std::cout << time << " " << role << " " << spotA << " " << spotB
			// << "\n";

			if (role == 0) {
				if (lines[0] == TB)
					throw report::rtError(
					    "Too many mother (role 0) lines defined.");

				// filling up mother record, role should be 0
				dm.Mtimes[lines[0]] =
				    -timeStep * static_cast<float>(TB - lines[0]);
			} else if (role < 0 || role > D) {
				throw report::rtError(
				    fmt::format("Found definition for daughter{} but can store "
				                "only {} daughters.",
				                role, D));
			} else {
				// filling up some daughter record
				if (lines[role] == TA)
					throw report::rtError(fmt::format(
					    "Too many daughter (role {}) lines defined.", role));

				dm.Dtimes[role - 1][lines[role]] =
				    timeStep * static_cast<float>(lines[role]);
			}

			// read the radii+dist pairs for the N-1 spheres
			for (int n = 0; n < N - 1; ++n) {
				f >> r >> dist;

				// NB: all bounds test have passed since we got here
				if (role == 0) {
					dm.Mradii[lines[0]][n] = r;
					dm.Mdists[lines[0]][n] = dist;
				} else {
					dm.Dradii[role - 1][lines[role]][n] = r;
					dm.Ddists[role - 1][lines[role]][n] = dist;
				}
			}

			// read the radii for the last sphere
			f >> r;
			if (role == 0)
				dm.Mradii[lines[0]][N - 1] = r;
			else
				dm.Dradii[role - 1][lines[role]][N - 1] = r;

			// update line counter now that the full (= meaningful part of the)
			// line is read
			++lines[role];

			// read the rest of the line (incl. the endline)
			f.getline(skipLine, 512);

			// skip over lines with comments, as comments are allowed inside
			// model blocks
			firstChar = f.get();
			while (firstChar != std::ifstream::traits_type::eof() &&
			       firstChar == '#') {
				f.putback(static_cast<char>(firstChar));
				f.getline(skipLine, 512);
				firstChar = f.get();
			}
		}

		// control-check:
		if (lines[0] != TB)
			throw report::rtError(
			    fmt::format("Not enough mother (role 0) lines defined."));

		for (int d = 1; d < D + 1; ++d)
			if (lines[d] != TA)
				throw report::rtError(fmt::format(
				    "Not enough daughter (role {}) lines defined.", d));
	}
};

/** dedicated type for 2-sphered agents that divide into 2 daughters */
template <int TB, int TA>
class DivisionModels2S : public DivisionModels<2, TB, TA, 2> {
  public:
	DivisionModels2S(const char* filename, const float _timeStep)
	    : DivisionModels<2, TB, TA, 2>(filename, _timeStep) {}
};

/** dedicated type for 4-sphered agents that divide into 2 daughters */
template <int TB, int TA>
class DivisionModels4S : public DivisionModels<4, TB, TA, 2> {
  public:
	DivisionModels4S(const char* filename, const float _timeStep)
	    : DivisionModels<4, TB, TA, 2>(filename, _timeStep) {}
};

/** dedicated type for sphered agents that divide into 2 daughters */
template <int N, int TB, int TA>
class DivisionModelsNS : public DivisionModels<N, TB, TA, 2> {
  public:
	DivisionModelsNS(const char* filename, const float _timeStep)
	    : DivisionModels<N, TB, TA, 2>(filename, _timeStep) {}
};