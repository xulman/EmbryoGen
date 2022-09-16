#include "Spheres.hpp"
#include "VectorImg.hpp"
#include "util/Serialization.hpp"

void VectorImg::getDistance(const Geometry& otherGeometry,
                            std::list<ProximityPair>& l) const
{
	switch (otherGeometry.shapeForm)
	{
	case ListOfShapeForms::Spheres:
		getDistanceToSpheres((class Spheres*)&otherGeometry,l);
		break;
	case ListOfShapeForms::Mesh:
		//TODO: attempt to project mesh vertices into the mask image and look for collision
report::message(fmt::format("this.VectorImg vs Mesh is not implemented yet!" ));
		break;
	case ListOfShapeForms::ScalarImg:
		//find collision "from the other side"
		getSymmetricDistance(otherGeometry,l);
		break;
	case ListOfShapeForms::VectorImg:
		//TODO identity case
report::message(fmt::format("this.VectorImg vs VectorImg is not implemented yet!" ));
		//getDistanceToVectorImg((VectorImg*)&otherGeometry,l);
		break;

	case ListOfShapeForms::undefGeometry:
report::message(fmt::format("Ignoring other geometry of type 'undefGeometry'." ));
		break;
	default:
throw report::rtError("Not supported combination of shape representations.");
	}
}


void VectorImg::getDistanceToSpheres(const class Spheres* otherSpheres,
                                     std::list<ProximityPair>& l) const
{
	//da plan: determine bounding box within this VectorImg where
	//we can potentially see any piece of the foreign Spheres;
	//sweep it and consider voxel centres; construct a thought
	//vector from a sphere's centre to the voxel centre, make it
	//that sphere's radius long and check the end of this vector
	//(when positioned at sphere's centre) if it falls into the
	//currently examined voxel; if it does, we have found a voxel
	//that contains a piece of the sphere's surface
	//
	//if half of a voxel diagonal is pre-calculated, one can optimize
	//by first comparing it against abs.val. of the difference of
	//the sphere's radius from the length of the thought vector

	//sweeping box in micrometers, initiated to match spheres' bounding box
	AxisAlignedBoundingBox sweepBox(otherSpheres->AABB);

	//intersect the sweeping box with this AABB
	//(as only this AABB wraps around interesting information in the mask image,
	// the image might be larger (especially in the GradIN_ZeroOUT model))
	sweepBox.minCorner.elemMax(AABB.minCorner); //in mu
	sweepBox.maxCorner.elemMin(AABB.maxCorner);

	//the sweeping box in pixels, in coordinates of this FF
	Vector3d<size_t> curPos, minSweepPX,maxSweepPX;
	sweepBox.exportInPixelCoords(X, minSweepPX,maxSweepPX);

	//(squared) voxel's volume half-diagonal vector and its length
	//(for detection of voxels that coincide with sphere's surface)
	Vector3d<G_FLOAT> vecPXhd2(0.5f/imgRes.x,0.5f/imgRes.y,0.5f/imgRes.z);
	const G_FLOAT     lenPXhd = vecPXhd2.len();
	vecPXhd2.elemMult(vecPXhd2); //make it squared

	//shortcuts to the otherGeometry's spheres
	const Vector3d<G_FLOAT>* const centresO = otherSpheres->getCentres();
	const G_FLOAT* const radiiO             = otherSpheres->getRadii();
	const int io                          = otherSpheres->getNoOfSpheres();

	//remember the squared lengths observed so far for every foreign sphere...
	G_FLOAT* lengths2 = new G_FLOAT[io];
	switch (policy)
	{ //initialize the array
	case minVec:
		for (int i = 0; i < io; ++i) lengths2[i] = TOOFAR;
		break;
	case maxVec:
	case avgVec:
		for (int i = 0; i < io; ++i) lengths2[i] = 0;
		break;
	case allVec:
	default: ;
		//no init required
	}
	//...what was observed...
	Vector3d<G_FLOAT>* vecs = new Vector3d<G_FLOAT>[io]; //inits to zero vector by default
	//...and where it was observed
	Vector3d<size_t>* hints = new Vector3d<size_t>[io];

	//holder for the currently examined vectors in the FF
	Vector3d<G_FLOAT> vec;

	//aux position vectors: current voxel's centre and somewhat sphere's surface point
	Vector3d<G_FLOAT> centre, surfPoint;

	//finally, sweep and check intersection with spheres' surfaces
	for (curPos.z = minSweepPX.z; curPos.z < maxSweepPX.z; curPos.z++)
	for (curPos.y = minSweepPX.y; curPos.y < maxSweepPX.y; curPos.y++)
	for (curPos.x = minSweepPX.x; curPos.x < maxSweepPX.x; curPos.x++)
	{
		//we are now visiting voxels where some sphere can be seen,
		//get micron coordinate of the current voxel's centre
		centre.toMicronsFrom(curPos, imgRes,imgOff);

		//check the current voxel against all spheres
		for (int i = 0; i < io; ++i)
		{
			surfPoint  = centre;
			surfPoint -= centresO[i];
			//if sphere's surface would be 2*lenPXhd thick, would the voxel's center be in?
			if (std::abs(surfPoint.len() - radiiO[i]) < lenPXhd)
			{
				//found a voxel _nearby_ i-th sphere's true surface,
				//is there really an intersection?

				//a vector pointing from voxel's centre to the nearest sphere surface
				surfPoint *= radiiO[i]/surfPoint.len() -1.0f;
				//NB: surfPoint *= (radiiO[i] - surfPoint.len()) / surfPoint.len()

				//max (squared; only to avoid using std::abs()) distance to the voxel's centre
				//to see if surfPoint's tip is within this voxel
				surfPoint.elemMult(surfPoint);
				if (surfPoint.x <= vecPXhd2.x && surfPoint.y <= vecPXhd2.y && surfPoint.z <= vecPXhd2.z)
				{
					//hooray, a voxel whose volume is intersecting with i-th sphere's surface
					//let's inspect the FF's vector at this position
					vec.x = X.GetVoxel(curPos.x,curPos.y,curPos.z);
					vec.y = Y.GetVoxel(curPos.x,curPos.y,curPos.z);
					vec.z = Z.GetVoxel(curPos.x,curPos.y,curPos.z);

					switch (policy)
					{
					case minVec:
						if (vec.len2() < lengths2[i])
						{
							lengths2[i] = vec.len2();
							vecs[i] = vec;
							hints[i] = curPos;
						}
						break;
					case maxVec:
						if (vec.len2() > lengths2[i])
						{
							lengths2[i] = vec.len2();
							vecs[i] = vec;
							hints[i] = curPos;
						}
						break;
					case avgVec:
						vecs[i] += vec;
						lengths2[i] += 1; //act as count here
						break;
					case allVec:
						//exact point on the sphere's surface
						surfPoint  = centre;
						surfPoint -= centresO[i];
						surfPoint *= radiiO[i]/surfPoint.len();
						surfPoint += centresO[i];

						//create immediately the ProximityPair
						l.emplace_back( surfPoint+vec,surfPoint, vec.len(),
						  (signed)X.GetIndex(curPos.x,curPos.y,curPos.z),i );
						break;
					default: ;
					}

				} //if intersecting voxel was found
			} //if nearby voxel was found
		} //over all foreign spheres
	} //over all voxels in the sweeping box

	//process (potentially) the one vector per sphere
	if (policy == minVec)
	{
		//add ProximityPairs where found some
		for (int i = 0; i < io; ++i)
		if (lengths2[i] < TOOFAR)
		{
			//found one
			//calculate exact point on the sphere's surface (using hints[i])
			surfPoint.toMicronsFrom(hints[i], imgRes,imgOff);
			surfPoint -= centresO[i];
			surfPoint *= radiiO[i]/surfPoint.len();
			surfPoint += centresO[i];

			//this is from VectorImg perspective (local = VectorImg, other = Sphere),
			//it reports index of the relevant foreign sphere
			l.emplace_back( surfPoint+vecs[i],surfPoint, std::sqrt(lengths2[i]),
			  (signed)X.GetIndex(hints[i].x,hints[i].y,hints[i].z),i );
		}
	}
	else if (policy == maxVec)
	{
		//add ProximityPairs where found some
		for (int i = 0; i < io; ++i)
		if (lengths2[i] > 0)
		{
			//found one
			//calculate exact point on the sphere's surface (using hints[i])
			surfPoint.toMicronsFrom(hints[i], imgRes,imgOff);
			surfPoint -= centresO[i];
			surfPoint *= radiiO[i]/surfPoint.len();
			surfPoint += centresO[i];

			//this is from VectorImg perspective (local = VectorImg, other = Sphere),
			//it reports index of the relevant foreign sphere
			l.emplace_back( surfPoint+vecs[i],surfPoint, std::sqrt(lengths2[i]),
			  (signed)X.GetIndex(hints[i].x,hints[i].y,hints[i].z),i );
		}
	}
	else if (policy == avgVec)
	{
		//add ProximityPairs where found some
		for (int i = 0; i < io; ++i)
		if (lengths2[i] > 0)
		{
			//found at least one
			vecs[i] /= lengths2[i];

			//this is from VectorImg perspective (local = VectorImg, other = Sphere),
			//it reports index of the relevant foreign sphere
			l.emplace_back( centresO[i]+vecs[i],centresO[i], vecs[i].len(), 0,i );
			//NB: average vector is no particular existing vector, hence local hint = 0,
			//    and placed in the sphere's centre
		}
	}
	//else allVec -- this has been already done above

	delete[] lengths2;
	delete[] vecs;
	delete[] hints;
}


void VectorImg::updateThisAABB(AxisAlignedBoundingBox& AABB) const
{
	//the AABB is the whole image
	AABB.minCorner = imgOff;
	AABB.maxCorner = imgFarEnd;
}


// ----------------- support for serialization and deserealization -----------------
long VectorImg::getSizeInBytes() const
{
	long size = Serialization::getSizeInBytes(X);
	return 3*size + 2*sizeof(int);
}


void VectorImg::serializeTo(char* buffer) const
{
	long off = Serialization::toBuffer((int)policy,buffer);
	off += Serialization::toBuffer(X,buffer+off);
	off += Serialization::toBuffer(Y,buffer+off);
	off += Serialization::toBuffer(Z,buffer+off);

	Serialization::toBuffer(version, buffer+off);
}


void VectorImg::deserializeFrom(char* buffer)
{
	int ppolicy;
	long off = Deserialization::fromBuffer(buffer,ppolicy);

	if (ppolicy != (int)this->policy)
throw report::rtError(fmt::format("Deserialization mismatch: filling image of choosing policy model {} with model {} from the buffer" , this->policy, (ChoosingPolicy)ppolicy));

	off += Deserialization::fromBuffer(buffer+off,X);
	off += Deserialization::fromBuffer(buffer+off,Y);
	off += Deserialization::fromBuffer(buffer+off,Z);
	updateResOffFarEnd();

	//update Geometry attribs:
	Deserialization::fromBuffer(buffer+off, version);
	updateThisAABB(this->AABB);
}


VectorImg* VectorImg::createAndDeserializeFrom(char* buffer)
{
	//read the choosing policy model
	int ppolicy;
	Deserialization::fromBuffer(buffer,ppolicy);

	//create an object
	VectorImg* vimg = new VectorImg((ChoosingPolicy)ppolicy);
	vimg->deserializeFrom(buffer);
	return vimg;
}


void VectorImg::renderIntoMask(i3d::Image3d<i3d::GRAY16>&, const i3d::GRAY16) const
{
report::message(fmt::format("NOT IMPLEMENTED YED!" ));
}
