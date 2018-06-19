#include <iostream>
#include <fstream>
#include <list>
#include <i3d/image3d.h>
#include <i3d/morphology.h>

using namespace std;
using namespace i3d;

#define dPI		6.28318

//keeps pair of float numbers: either Cartesian of polar coordinate
class Pair {
 public:
	float a,b;
};

class SortPair {
 public:
	bool operator()(const Pair &a, const Pair &b) const
	{ return(a.a < b.a); }
};


template <class VOXEL>
void Extract(const Image3d<VOXEL>& img, const int ID, const int slice, char* output)
{
	//consistency test
	if ((slice < 0) || (slice >= img.GetSizeZ())) {
		cerr << "image has too few slices\n";
		throw;
	}

	//can we write the output file?
	ofstream file(output);


	//make a "copy out" of the mask into a separate image
	Image3d<bool> tmpI,tmpE; //Initial and Eroded images
	tmpI.MakeRoom(img.GetSizeX(),img.GetSizeY(),1);
	tmpI.GetVoxelData()=false;

	const VOXEL* rI=img.GetVoxelAddr(0,0,slice);
	bool* rW=tmpI.GetFirstVoxelAddr();
	bool found=false;
	if (ID <= 0) {
		for (size_t i=0; i < tmpI.GetImageSize(); ++i, ++rI, ++rW)
			if (*rI > 0) { *rW=true; found=true; }
	} else {
		for (size_t i=0; i < tmpI.GetImageSize(); ++i, ++rI, ++rW)
			if (*rI == ID) { *rW=true; found=true; }
	}

	if (!found) {
		cerr << "mask not found in the image\n";
		throw;
	}

	//erode the copy
	Erosion<bool>(tmpI,tmpE,nb2D_8);


	//here's the list of boundary points
	list<Pair> L;

	//time-savers:
	const float resx=img.GetResolution().GetX();
	const float resy=img.GetResolution().GetY();
	const float offx=img.GetOffset().x;
	const float offy=img.GetOffset().y;

	//geom centre
	double cX=0., cY=0.f;
	long int count=0;

	//see what's been eroded -- these are at the boundary
	bool* pB=tmpI.GetFirstVoxelAddr();
	rW=tmpE.GetFirstVoxelAddr();
	for (size_t i=0; i < tmpI.GetImageSize(); ++i, ++rW, ++pB)
		if ((*pB) && (! *rW))
		{
			//found boundary point
			//get its "real-world" coordinate
			Pair p;
			p.a=tmpI.GetX(i)/resx + offx;
			p.b=tmpI.GetY(i)/resy + offy;

			//save the point on the list
			L.push_back(p);

			//update geom centre vars
			cX+=p.a;
			cY+=p.b;
			++count;
		}

	//calc geom centre
	cX /= double(count);
	cY /= double(count);

	//determine these stats while converting
	float minDist=L.front().b;
	float maxDist=0., maxDistAng=-1.;

	//convert points on the list to polar coords
	list<Pair>::iterator LI=L.begin();
	while (LI != L.end())
	{
		//make (a,b) a vector from cell's centre towards boundary point
		(*LI).a -= cX;
		(*LI).b -= cY;

		//calc its polar coordinate
		float ang=atan2((*LI).b,(*LI).a);
		if (ang < 0) ang+=dPI;
		float rr=sqrt((*LI).a*(*LI).a + (*LI).b*(*LI).b);

		(*LI).a=ang;
		(*LI).b=rr;

		//update stats...
		if (rr < minDist) minDist=rr;
		if (rr > maxDist)
		{
			maxDist=rr;
			maxDistAng=ang;
		}

		LI++;
	}

	//now sort according to the angle
	SortPair ss;
	L.sort(ss);

	//write "header" about the cell
	file << cX << " " << cY << " 0\n";
	file << "-1\n";
	file << maxDistAng << " " << 0.8*minDist << "\n\n";

	//finally, dump the list into the file
	LI=L.begin();
	while (LI != L.end())
	{
		file << (*LI).a << " " << (*LI).b << "\n";

		LI++;
	}

	file.close();

	cout << "added " << count << " boundary points\n";
	cout << "radius min=" << minDist << " and max=" << maxDist << "\n";
}


int main(int argc, char **argv)
{
	 if (argc != 5)
	 {
	 	cout << "Extracts list of boundary points of a particular cell...\n\n";
		cout << "params: mask_file.ics cell_ID z_slice output_file.txt\n";
		cout << "If cell_ID <= 0 then any non-zero pixel is treated as cell mask pixel.\n";
		return(2);
	 }
    try
    {
		cerr << "Reading image Type" << endl;
		ImgVoxelType vt = ReadImageType(argv[1]);
		cerr << "Voxel=" << VoxelTypeToString(vt) << endl;

		int ID=atoi(argv[2]);
		int sl=atoi(argv[3]);

		switch (vt)
		{
		case Gray8Voxel:
			{
			Image3d <GRAY8> Input(argv[1]);
			Extract(Input,ID,sl,argv[4]);
			}
			break;
		case Gray16Voxel:
			{
			Image3d <GRAY16> Input(argv[1]);
			Extract(Input,ID,sl,argv[4]);
			}
			break;
		case FloatVoxel:
			{
			Image3d <float> Input(argv[1]);
			Extract(Input,ID,sl,argv[4]);
			}
			break;
		default:
			cerr << "Other voxel types not supported" << endl;
		}
    }
    catch(IOException & e)
    {
        cerr << e << endl;
        exit(1);
    }
    catch(InternalException & e)
    {
        cerr << e << endl;
        exit(1);
    }
    catch(bad_alloc &)
    {
        cerr << "Not enough memory." << endl;
        exit(1);
    }
    catch(...)
    {
        cerr << argv[0] << "Unknown exception." << endl;
        exit(1);
    }
    return 0;
}
