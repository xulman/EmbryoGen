#include "../../util/texture/texture.hpp"
#include "../../util/report.hpp"
#include "Texture.hpp"
#include <cmath>

template <typename VT>
void Texture::sampleDotsFromImage(const i3d::Image3d<VT>& img,
                                  const Spheres& geom,
                                  const VT quantization) {
	const Vector3d<float> res(img.GetResolution().GetRes());
	const Vector3d<float> off(img.GetOffset());

	// sigmas such that Gaussian will spread just across one voxel
	const Vector3d<float> chaossSigma(
	    Vector3d<float>(1.0f / 6.0f).elemDivBy(res));
#ifndef NDEBUG
	long missedDots = 0;
#endif

	// sweeping stuff...
	const VT* imgPtr = img.GetFirstVoxelAddr();
	Vector3d<size_t> pxPos;
	Vector3d<float> umPos;

	for (pxPos.z = 0; pxPos.z < img.GetSizeZ(); ++pxPos.z)
		for (pxPos.y = 0; pxPos.y < img.GetSizeY(); ++pxPos.y)
			for (pxPos.x = 0; pxPos.x < img.GetSizeX(); ++pxPos.x, ++imgPtr) {
				umPos.toMicronsFrom(pxPos, res, off);

				if (geom.collideWithPoint(umPos) >= 0) {
					checkAndIncreaseCapacity();

					// displace randomly within this voxel
					for (int i = int(*imgPtr / quantization); i > 0; --i) {
						dots.emplace_back(umPos);
						dots.back().pos += Vector3d<float>(
						    GetRandomGauss(0.f, chaossSigma.x, rngState),
						    GetRandomGauss(0.f, chaossSigma.y, rngState),
						    GetRandomGauss(0.f, chaossSigma.z, rngState));

#ifndef NDEBUG
						// test if the new position still fits inside the
						// original voxel
						Vector3d<size_t> pxBackPos;
						dots.back().pos.fromMicronsTo(pxBackPos, res, off);
						if (!pxBackPos.elemIsPredicateTrue(
						        pxPos, [](float l, float r) { return l == r; }))
							++missedDots;
#endif
					}
				}
			}

#ifndef NDEBUG
	report::message(fmt::format(
	    "there are {} ({} %) dots placed outside its original voxel",
	    missedDots, (float)(100 * missedDots) / (float)dots.size()));
	report::message(
	    fmt::format("there are currently {} registered dots", dots.size()));
#endif
}

void Texture::createPerlinTexture(const Spheres& geom,
                                  const Vector3d<float>& textureResolution,
                                  const double var,
                                  const double alpha,
                                  const double beta,
                                  const int n,
                                  const float textureAverageIntensity,
                                  const float quantization,
                                  const bool shouldCollectOutlyingDots) {
	// setup the aux texture image
	i3d::Image3d<float> img;
	setupImageForRasterizingTexture(img, textureResolution, geom);

	// sanity check... if the 'geom' is "empty", no texture image is "wrapped"
	// around it, we do no creation of the texture then...
	if (img.GetImageSize() == 0) {
		report::debugMessage(fmt::format("WARNING: Wrapping texture image is "
		                                 "of zero size... stopping here."));
		return;
	}

	// fill the aux image
	DoPerlin3D(img, var, alpha, beta, n);

	// get the current average intensity and adjust to the desired one
	double sum = 0;
	float* i = img.GetFirstVoxelAddr();
	float* const iL = i + img.GetImageSize();
	for (; i != iL; ++i)
		sum += *i;
	sum /= (double)img.GetImageSize();
	//
	const float textureIntShift = textureAverageIntensity - (float)sum;
	for (i = img.GetFirstVoxelAddr(); i != iL; ++i) {
		*i += textureIntShift;
		//*i  = std::max( *i, 0.f );
	}
	sampleDotsFromImage(img, geom, quantization);
	// NB: the function ignores any negative-valued texture image voxels,
	//     no dots are created for such voxels

	if (shouldCollectOutlyingDots) {
#ifndef DEBUG
		collectOutlyingDots(geom);
#else
		const int dotOutliers = collectOutlyingDots(geom);
		report::debugMessage(fmt::format(
		    "{} ({} %) dots had to be moved inside the initial geometry",
		    dotOutliers, (float)(100 * dotOutliers) / (float)dots.size()));
#endif
	}
}

int Texture::collectOutlyingDots(const Spheres& geom) {
	int count = 0;
#ifndef NDEBUG
	double outDist = 0;
	double inDist = 0;
	int postCorrectionsCnt = 0;

	auto stopWatch = tic();
#endif

	Vector3d<float> tmp;
	float tmpLen;

	for (auto& dot : dots) {
		bool foundInside = false;
		float nearestDist = TOOFAR;
		int nearestIdx = -1;

		for (std::size_t i = 0; i < geom.getNoOfSpheres() && !foundInside;
		     ++i) {
			// test against the i-th sphere
			tmp = geom.centres[i];
			tmp -= dot.pos;
			tmpLen = tmp.len() - geom.radii[i];

			foundInside = tmpLen <= 0;

			if (!foundInside && tmpLen < nearestDist) {
				// update nearest distance
				nearestDist = tmpLen;
				nearestIdx = int(i);
			}
		}

		if (!foundInside) {
			// correct dot's position according to the nearestIdx:
			// random position inside the (zero-centered) sphere
			dot.pos.x =
			    GetRandomGauss(0.f, geom.radii[nearestIdx] / 2.f, rngState);
			dot.pos.y =
			    GetRandomGauss(0.f, geom.radii[nearestIdx] / 2.f, rngState);
			dot.pos.z =
			    GetRandomGauss(0.f, geom.radii[nearestIdx] / 2.f, rngState);

			// make sure we're inside this sphere
			if (dot.pos.len() > geom.radii[nearestIdx]) {
				dot.pos.changeToUnitOrZero() *= 0.9f * geom.radii[nearestIdx];
				// NB: it held and holds for sure: dot.pos.len() > 0
#ifndef NDEBUG
				++postCorrectionsCnt;
#endif
			}

#ifndef NDEBUG
			outDist += nearestDist;
			inDist += dot.pos.len();
#endif
			dot.pos += geom.centres[nearestIdx];
			++count;
		}
	}

#ifndef NDEBUG
	if (count > 0) {
		report::message(
		    fmt::format("average outside-to-surface distance {} um (in {})",
		                outDist / (double)count, toc(stopWatch)));
		report::message(fmt::format("average new-pos-to-centre  distance {} um",
		                            inDist / (double)count));
		report::message(fmt::format(
		    "secondary corrections of {}/{} ({} %) dots", postCorrectionsCnt,
		    count, (float)(100 * postCorrectionsCnt) / (float)count));
	} else {
		report::message(fmt::format("no corrections were necessary (in {})",
		                            toc(stopWatch)));
	}
#endif
	return count;
}

/** implements some fake function 1 -> 0+ to (poorly) simulate the ever
    decreasing photon budget, or just always returns 1.0 iff photobleaching
    shall be ignored (photobleaching feature is disabled */
#ifdef ENABLED_PHOTOBLEACHING
inline float getBleachFactor(const short cntOfExcitations) {
	return std::exp((float)-cntOfExcitations);
}
#else
inline float getBleachFactor(const short) { return 1.0f; }
#endif

void Texture::renderIntoPhantom(i3d::Image3d<float>& phantoms,
                                const float quantization) {
	report::debugMessage(fmt::format("going to render {} dots", dots.size()));

	// cache some output image params
	const Vector3d<float> res(phantoms.GetResolution().GetRes());
	const Vector3d<float> off(phantoms.GetOffset());
	const Vector3d<size_t> imgSize(phantoms.GetSize());
	Vector3d<size_t> imgPos;

#ifndef NDEBUG
	auto stopWatch = tic();

	// a variable for debugging the photobleaching
	double meanIntContribution = 0.0f;
	long meanIntContCounter = 0;
#endif

	float* const paddr = phantoms.GetFirstVoxelAddr();
	for (auto& dot : dots) {
		// note that this dot is yet again excited to give some light
		++(dot.cntOfExcitations);

		// turn micron position to phantom pixel one
		dot.pos.fromMicronsTo(imgPos, res, off);

		// is the pixel inside the input image?
		// stdandard way:
		// if (phantoms.Include((int)imgPos.x,(int)imgPos.y,(int)imgPos.z))
		//
		// or, lower bound (but for _unsigned_ makes no sense):
		// if (imgPos.elemIsPredicateTrue([](const size_t x){ return x >= 0; })
		//
		// plus upper bound (tests also underflows... "negative" coordinates)
		if (imgPos.elemIsLessThan(imgSize)) {
			const float fval =
			    quantization * getBleachFactor(dot.cntOfExcitations);
#ifndef NDEBUG
			meanIntContribution += fval;
			++meanIntContCounter;
#endif
			// add the results together
			*(paddr + imgPos.toImgIndex(imgSize)) += fval;
		} else {
			report::debugMessage(fmt::format(
			    "ups, dot {} is outside the phantom image", toString(dot.pos)));
		}
	}

#ifndef NDEBUG
	report::message(fmt::format(
	    "added total cell intensity: {} with mean dot intensity: {}",
	    meanIntContribution, meanIntContribution / (double)meanIntContCounter));
	report::message(fmt::format("rendering dots took {}", toc(stopWatch)));
#endif
}

void TextureQuantized::renderIntoPhantom(i3d::Image3d<float>& phantoms) {
	report::debugMessage(
	    fmt::format("going to render {} quantum dots", dots.size()));

	// cache some output image params
	const Vector3d<float> res(phantoms.GetResolution().GetRes()); // in px/um
	const Vector3d<float> off(phantoms.GetOffset());              // in um

	// sweeping-related vars
	const int xLine = (signed)phantoms.GetSizeX();           // in px
	const int xyPlane = (signed)phantoms.GetSizeY() * xLine; // in px
	const Vector3d<float> boxStep(Vector3d<float>(boxSize.x / (float)qCounts.x,
	                                              boxSize.y / (float)qCounts.y,
	                                              boxSize.z / (float)qCounts.z)
	                                  .elemMult(res)); // in px
	Vector3d<float> imgPos;                            // in px

	// image boundaries after "applying retention zone"
	const Vector3d<float> halfBoxSize(
	    (0.5f * boxSize).elemMult(res)); // also a lower bound, px
	const Vector3d<float> imgSize(       //       upper bound, px
	    Vector3d<size_t>(phantoms.GetSize()).to<float>().elemMult(res) -
	    halfBoxSize);

#ifndef NDEBUG
	auto stopWatch = tic();

	// a variable for debugging the photobleaching
	double meanIntContribution = 0.0f;
	const float quantumElems =
	    (float)qCounts.x * (float)qCounts.y * (float)qCounts.z;
	long meanIntContCounter = 0;
#endif

	float* const paddr = phantoms.GetFirstVoxelAddr();
	for (auto& dot : dots) {
		// note that this dot is yet again excited to give some light
		++(dot.cntOfExcitations);

		// turn micron position to phantom pixel one
		imgPos.from(dot.pos).toPixels(res, off);

		// test against image boundaries now and skip this dot if it does not
		// fall _completely_ inside the phantom image; otherwise we would need
		// to conduct such test with every single quantum -> very slow; this
		// does not hurt since the occurrence of the agent near (essentially
		// touching) the boundary should be rather unlikely
		if (imgPos.elemIsGreaterOrEqualThan(halfBoxSize) &&
		    imgPos.elemIsLessThan(imgSize)) {
			const float fval = getBleachFactor(dot.cntOfExcitations);
#ifndef NDEBUG
			meanIntContribution += quantumElems * fval;
			++meanIntContCounter;
#endif
			// std::cout << "--- centre " << imgPos << " um\n";

			// iterate the quantum box from this (float) voxel position
			imgPos -= halfBoxSize;

			// populate the "quantum cube":
			for (short iz = 0; iz < qCounts.z; ++iz) {
				// NB: the Vector3d<>::fromMicronsTo()'s
				// real-px-coord-to-int-px-coord policy
				const int Z = int(imgPos.z + (float)iz * boxStep.z);

				for (short iy = 0; iy < qCounts.y; ++iy) {
					const int Y = int(imgPos.y + (float)iy * boxStep.y);

					float* const T = paddr + Z * xyPlane + Y * xLine;
					for (short ix = 0; ix < qCounts.x; ++ix) {
						const int X = int(imgPos.x + (float)ix * boxStep.x);
						T[X] += fval;

						// std::cout << "populating at [" << X << "," << Y <<
						// "," << Z << "] px because "
						//           << (
						//           imgPos+Vector3d<float>(ix,iy,iz).elemMult(boxStep)
						//           ) << " um\n";
					}
				}
			}
		} else {
			report::debugMessage(fmt::format(
			    "ups, dot {} is outside the phantom image", toString(dot.pos)));
		}
	}

#ifndef NDEBUG
	report::message(fmt::format(
	    "added total cell intensity: {} with mean dot intensity: {}",
	    meanIntContribution, meanIntContribution / (double)meanIntContCounter));
	report::message(
	    fmt::format("rendering quantum dots took {}", toc(stopWatch)));
#endif
}

// ------------- explicit instantiations -------------
template void Texture::sampleDotsFromImage(const i3d::Image3d<i3d::GRAY8>& img,
                                           const Spheres& geom,
                                           const i3d::GRAY8 quantization);
template void Texture::sampleDotsFromImage(const i3d::Image3d<i3d::GRAY16>& img,
                                           const Spheres& geom,
                                           const i3d::GRAY16 quantization);
template void Texture::sampleDotsFromImage(const i3d::Image3d<float>& img,
                                           const Spheres& geom,
                                           const float quantization);
template void Texture::sampleDotsFromImage(const i3d::Image3d<double>& img,
                                           const Spheres& geom,
                                           const double quantization);
// ------------- explicit instantiations -------------

void TextureUpdater4S::updateTextureCoords(std::vector<Dot>& dots,
                                           const Spheres& newGeom) {
#ifndef NDEBUG
	if (newGeom.getNoOfSpheres() != 4)
		throw report::rtError(
		    "Cannot update coordinates for non-four sphere geometry.");
#endif
	// backup: last geometry for which user coordinates were valid
	for (unsigned int i = 0; i < 4; ++i) {
		prevCentre[i] = cu[i].prevCentre;
		prevRadius[i] = cu[i].prevRadius;
	}

	// prepare the updating routines...
	cu[0].prepareUpdating(newGeom.centres[0], newGeom.radii[0],
	                      newGeom.centres[0] - newGeom.centres[1]);

	cu[1].prepareUpdating(newGeom.centres[1], newGeom.radii[1],
	                      newGeom.centres[0] - newGeom.centres[2]);

	cu[2].prepareUpdating(newGeom.centres[2], newGeom.radii[2],
	                      newGeom.centres[1] - newGeom.centres[3]);

	cu[3].prepareUpdating(newGeom.centres[3], newGeom.radii[3],
	                      newGeom.centres[2] - newGeom.centres[3]);

	// aux variables
	float weights[4];
	float sum;
	Vector3d<float> tmp, newPos;
#ifndef NDEBUG
	int outsideDots = 0;
#endif
	// shift texture particles
	for (auto& dot : dots) {
		// determine the weights
		for (unsigned int i = 0; i < 4; ++i) {
			tmp = dot.pos;
			tmp -= prevCentre[i];
			weights[i] = std::max(prevRadius[i] - tmp.len(), 0.0f);
		}

		// normalization factor
		sum = weights[0] + weights[1] + weights[2] + weights[3];

		if (sum > 0) {
			// apply the weights
			newPos = 0;
			for (unsigned int i = 0; i < 4; ++i)
				if (weights[i] > 0) {
					tmp = dot.pos;
					cu[i].updateCoord(tmp);
					newPos += (weights[i] / sum) * tmp;
				}
			dot.pos = newPos;
		} else {
#ifndef NDEBUG
			++outsideDots;
#endif
		}
	}

#ifndef NDEBUG
	if (outsideDots > 0)
		report::message(fmt::format(
		    "{} dots could not be updated (no matching sphere found, weird...)",
		    outsideDots));
#endif
}

void TextureUpdater2pNS::updateTextureCoords(std::vector<Dot>& dots,
                                             const Spheres& newGeom) {
#ifndef NDEBUG
	if (int(newGeom.getNoOfSpheres()) != noOfSpheres)
		throw report::rtError(
		    fmt::format("Cannot update coordinates for {} sphere geometry, "
		                "expected {} spheres.",
		                newGeom.getNoOfSpheres(), noOfSpheres));
#endif
	// backup: last geometry for which texture coordinates were valid
	// and prepare the updating routines where "orientation is global"
	Vector3d<float> tmp(newGeom.centres[sphereOnMainAxis]);
	tmp -= newGeom.centres[sphereAtCentre];
	for (int i = 0; i < noOfSpheres; ++i) {
		prevCentre[i] = cu[i].prevCentre;
		prevRadius[i] = cu[i].prevRadius;
		cu[i].prepareUpdating(newGeom.centres[i], newGeom.radii[i], tmp);
	}

	// aux variables
	float sum;
	Vector3d<float> newPos;
#ifndef NDEBUG
	int outsideDots = 0;
#endif

	// shift texture particles
	for (auto& dot : dots) {
		// determine the weights
		sum = 0;
		for (int i = 0; i < noOfSpheres; ++i) {
			tmp = dot.pos;
			tmp -= prevCentre[i];
			__weights[i] = std::max(prevRadius[i] - tmp.len(), 0.0f);
			sum += __weights[i];
		}

		if (sum > 0) {
			// apply the weights
			newPos = 0;
			for (int i = 0; i < noOfSpheres; ++i)
				if (__weights[i] > 0) {
					tmp = dot.pos;
					cu[i].updateCoord(tmp);
					tmp *= __weights[i] / sum;
					newPos += tmp;
				}
			dot.pos = newPos;
		} else {
#ifndef NDEBUG
			++outsideDots;
#endif
		}
	}

#ifndef NDEBUG
	if (outsideDots > 0)
		report::message(fmt::format(
		    "{} dots could not be updated (no matching sphere found, weird...)",
		    outsideDots));
#endif
}

void TextureUpdaterNS::resetNeigWeightMatrix(const Spheres& spheres,
                                             int maxNoOfNeighs) {
#ifndef NDEBUG
	if (int(spheres.getNoOfSpheres()) != noOfSpheres)
		throw report::rtError(
		    fmt::format("Cannot update coordinates for {} sphere geometry, "
		                "expected {} spheres.",
		                spheres.getNoOfSpheres(), noOfSpheres));

	if (maxNoOfNeighs < 1 || maxNoOfNeighs >= noOfSpheres)
		throw report::rtError(fmt::format("requesting maxNoOfNeighs={} when "
		                                  "only {}-1 neighbors are available",
		                                  maxNoOfNeighs, noOfSpheres));
#endif

	struct myGreaterThan_t {
		myGreaterThan_t(const Spheres& s) : spheres(s) {}
		const Spheres& spheres;

		int refIdx;
		bool operator()(const int l, const int r) const {
			// make sure the refIdx is sorted away to the far-most end of the
			// sequence, that is, the refIdx is always "smaller than"
			if (l == refIdx)
				return false;
			if (r == refIdx)
				return true;

			// need second test?
			if (spheres.radii[l] == spheres.radii[r]) {
				// secondary test: need to calculate overlaps
				const float refRadius = spheres.radii[refIdx];

				float otherRadius = spheres.radii[l];
				float lOverlapSize = refRadius + otherRadius;
				lOverlapSize -=
				    (spheres.centres[refIdx] - spheres.centres[l]).len();
				lOverlapSize = std::min(
				    lOverlapSize, std::min(2 * refRadius, 2 * otherRadius));

				otherRadius = spheres.radii[r];
				float rOverlapSize = refRadius + otherRadius;
				rOverlapSize -=
				    (spheres.centres[refIdx] - spheres.centres[r]).len();
				rOverlapSize = std::min(
				    rOverlapSize, std::min(2 * refRadius, 2 * otherRadius));

				return lOverlapSize > rOverlapSize;
			} else
				// primary test
				return spheres.radii[l] > spheres.radii[r];
		}
	} myGreaterThan(spheres);

	// prepare the array of all available indices,
	// the array will always be some permutation of all sphere indices
	std::vector<int> sortedIndices(noOfSpheres);
	for (int i = 0; i < noOfSpheres; ++i)
		sortedIndices[i] = i;

	// for every sphere:
	for (int row = 0; row < noOfSpheres; ++row) {
		// re-sort indices
		myGreaterThan.refIdx = row;
		std::sort(sortedIndices.begin(), sortedIndices.end(), myGreaterThan);

		// list indices of spheres in the order optimal for "mating"
		report::debugMessage(fmt::format("sphere #{}:", row), {true, false});

		int foundNeighs = 0;
		for (int sortedIdx = 0; sortedIdx < noOfSpheres - 1; ++sortedIdx)
		// NB: avoid self-eval: row idx is always at
		// sortedIndices[noOfSpheres-1]
		{
			const int col = sortedIndices[sortedIdx];
			// NB: col != row (unless sortedIdx+1 == noOfSpheres)

			if (foundNeighs >= maxNoOfNeighs) {
				// this makes sure the complete matrix row was touched
				*neigWeightMatrix(row, col) = 0;
				report::debugMessage(fmt::format("  {}(n)", col),
				                     {false, false});
				continue;
			}

			// only when not enough of neighs has been discovered so far,
			// but consider only overlapping neighs!
			const float overlap =
			    std::max(-(spheres.centres[row] - spheres.centres[col]).len() +
			                 spheres.radii[row] + spheres.radii[col],
			             0.0f);
			if (overlap > 0) {
				*neigWeightMatrix(row, col) = 1;
				++foundNeighs;
				report::debugMessage(fmt::format("  {}(y)", col),
				                     {false, false});
			} else {
				*neigWeightMatrix(row, col) = 0;
				report::debugMessage(fmt::format("  {}(n)", col),
				                     {false, false});
			}
		}
		*neigWeightMatrix(row, row) = (float)foundNeighs;
		report::debugMessage(fmt::format(" = {} links", foundNeighs), {false});

		if (foundNeighs == 0)
			throw report::rtError(
			    "Unsupported sphere configuration. Every sphere must overlap "
			    "with at least one another.");
	}
}

void TextureUpdaterNS::setNeigWeightMatrix(
    const SpheresFunctions::SquareMatrix<float>& newWeightMatrix) {
#ifndef NDEBUG
	if (neigWeightMatrix.side != newWeightMatrix.side)
		throw report::rtError("Attempting to set matrix of different size.");
#endif

	const int totalLength = neigWeightMatrix.side * neigWeightMatrix.side;
	for (int i = 0; i < totalLength; ++i)
		neigWeightMatrix.data[i] = newWeightMatrix.data[i];
}

void TextureUpdaterNS::printNeigWeightMatrix() {
	report::message(fmt::format("weights among neighboring spheres:"));
	neigWeightMatrix.print();
}

void TextureUpdaterNS::getLocalOrientation(const Spheres& spheres,
                                           const int idx,
                                           Vector3d<float>& orientVec) {
	Vector3d<float> tmpVec;

	orientVec = 0;
	for (std::size_t i = 0; i < spheres.getNoOfSpheres(); ++i)
		if (int(i) != idx && *neigWeightMatrix(idx, int(i)) > 0) {
			// vec from 'idx' to 'i'
			tmpVec = spheres.centres[i];
			tmpVec -= spheres.centres[idx];

			// weight this contribution
			tmpVec.changeToUnitOrZero();
			tmpVec *=
			    *neigWeightMatrix(idx, int(i)) / *neigWeightMatrix(idx, idx);

			orientVec += tmpVec;
		}
	orientVec.changeToUnitOrZero();
}

void TextureUpdaterNS::updateTextureCoords(std::vector<Dot>& dots,
                                           const Spheres& newGeom) {
#ifndef NDEBUG
	if (int(newGeom.getNoOfSpheres()) != noOfSpheres)
		throw report::rtError(
		    fmt::format("Cannot update coordinates for {} sphere geometry, "
		                "expected {} spheres.",
		                newGeom.getNoOfSpheres(), noOfSpheres));
#endif
	// backup: last geometry for which user coordinates were valid
	// and prepare the updating routines...
	Vector3d<float> tmp;
	for (int i = 0; i < noOfSpheres; ++i) {
		prevCentre[i] = cu[i].prevCentre;
		prevRadius[i] = cu[i].prevRadius;
		getLocalOrientation(newGeom, i, tmp);
		cu[i].prepareUpdating(newGeom.centres[i], newGeom.radii[i], tmp);
	}

	// aux variables
	float sum;
	Vector3d<float> newPos;
#ifndef NDEBUG
	int outsideDots = 0;
#endif

	// shift texture particles
	for (auto& dot : dots) {
		// determine the weights
		sum = 0;
		for (int i = 0; i < noOfSpheres; ++i) {
			tmp = dot.pos;
			tmp -= prevCentre[i];
			__weights[i] = std::max(prevRadius[i] - tmp.len(), 0.0f);
			sum += __weights[i];
		}

		if (sum > 0) {
			// apply the weights
			newPos = 0;
			for (int i = 0; i < noOfSpheres; ++i)
				if (__weights[i] > 0) {
					tmp = dot.pos;
					cu[i].updateCoord(tmp);
					tmp *= __weights[i] / sum;
					newPos += tmp;
				}
			dot.pos = newPos;
		} else {
#ifndef NDEBUG
			++outsideDots;
#endif
		}
	}

#ifndef NDEBUG
	if (outsideDots > 0)
		report::message(fmt::format(
		    "{} dots could not be updated (no matching sphere found, weird...)",
		    outsideDots));
#endif
}
