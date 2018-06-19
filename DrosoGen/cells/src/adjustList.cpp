#include <iostream>
#include <fstream>
#include <list>
#include <stdlib.h>
#include "../../src/params.h"

using namespace std;

#define DEG_TO_RAD(x) ((x)*M_PI/180.0f)
#define PI 3.14159265
#define dPI		6.28318


//implementation of a function declared in params.h
///returns |a-b| in radians, function takes care of 2PI periodicity
float AngularDifference(const float& a, const float& b)
{
	return acos(cos(a-b));
}


int main(int argc, char **argv)
{
	 if (argc != 5)
	 {
	 	cout << "Resizes list of boundary points of a particular cell...\n\n";
		cout << "params: cell_in.txt cell_out.txt final_radius shorter_radius\n\n";
		cout << "Restretches boundary points distances from the centre such that\n";
		cout << "the furtherst one is at distance final_radius. It is also does\n";
		cout << "kind of elongation such that distance of point at azimuth perpendicular\n";
		cout << "to the main cell orientation will be shorter_radius.\n";
		cout << "If some radius is -1, then no relevant stretch occurs.\n";
		return(2);
	 }
    try
    {
	 	//cell centre
		Vector3d<float> pos;

		///vector showing from the centre towards "head" of the cell
		PolarPair orientation;

		///list of bp (boundary points) -- shape of the cell
		std::list<PolarPair> bp;

		//search for the maximum radius
		float outerRadius=0.f;

		std::ifstream fileI(argv[1]);
		float ignore1, ignore2;
		fileI >> pos.x >> pos.y >> ignore1;
		fileI >> ignore2;
		fileI >> orientation.ang >> orientation.dist;

		float ang,dist;
		while (fileI >> ang >> dist)
		{
			  bp.push_back(PolarPair(ang,dist));

			  //update the search for maximum radius
			  if (dist > outerRadius) outerRadius=dist;
		}

		fileI.close();

		//overall resize
		float stretch=atof(argv[3]);
		if (stretch > 0.f)
		{
			stretch/= outerRadius;

			//now, re-adjust to become a cell with radius of final_radius
			std::list<PolarPair>::iterator p=bp.begin();
			while (p != bp.end())
			{
				p->dist *= stretch;
				++p;
			}
			outerRadius*=stretch;
			orientation.dist*=stretch;
		}

		//elongation
		stretch=atof(argv[4]);
		if (stretch > 0.f)
		{
			std::list<PolarPair>::iterator p,s=bp.begin();

			//so far the best solution:
			p=s;
			float bestAngle=AngularDifference(s->ang, orientation.ang - PI/2.f);

			//scan all BPs to find better
			while (s != bp.end())
			{
				float curAngle=AngularDifference(s->ang, orientation.ang - PI/2.f);
				if (curAngle < bestAngle)
				{
					p=s;
					bestAngle=curAngle;
				}

				++s;
			}

			//how much dist we are missing
			stretch=stretch - p->dist;

			//slightly more than 180deg should span 6*sigma
			const float spread=DEG_TO_RAD(200.f) / 6.f;
			const float sigma=spread*spread;

			//okay, we need to elongate by PeakElongation at the reference pole
			p=bp.begin();
			while (p != bp.end())
			{
				float lAngle=AngularDifference(p->ang,orientation.ang - PI/2.f);
				float rAngle=AngularDifference(p->ang,orientation.ang - PI/2.f +PI);

				p->dist += stretch*
					( expf(-0.5f*lAngle*lAngle/sigma) + expf(-0.5f*rAngle*rAngle/sigma) );

				++p;
			}
		}


		//can we write the output file?
		ofstream file(argv[2]);

		//write "header" about the cell
		file << pos.x << " " << pos.y << " " << ignore1 << "\n";
		file << ignore2 << "\n";
		file << orientation.ang << " " << orientation.dist << "\n\n";

		//finally, dump the list into the file
		std::list<PolarPair>::iterator p=bp.begin();
		while (p != bp.end())
		{
			file << p->ang << " " << p->dist << "\n";

			p++;
		}

		file.close();
    }
    catch(...)
    {
        cerr << argv[0] << "Some exception." << endl;
        exit(1);
    }
    return 0;
}
