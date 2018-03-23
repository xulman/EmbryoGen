#include <iostream>
#include <fstream>
#include <list>
#include <i3d/image3d.h>
#include <i3d/morphology.h>

using namespace std;
using namespace i3d;

#define dPI		6.28318f

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


template <class VOXEL, class LABEL>
void Extract(const Image3d<VOXEL>& imgI,
             const Image3d<LABEL>& imgM,
				 const LABEL ID,
				 const int slice,
				 char* output,
				 char* outputT)
{
	//consistency test
	if ((slice < 0) || (slice >= imgI.GetSizeZ()))
	{
		cerr << "texture image has few slices\n";
		throw;
	}

	if (imgI.GetSize() != imgM.GetSize())
	{
		cerr << "texture and mask images have different size\n";
		throw;
	}

	//can we write the output file?
	ofstream file(output);

	//make a "copy out" of the mask into a separate image ...
	Image3d<bool> tmpI,tmpE; //Initial and Eroded images
	tmpI.MakeRoom(imgM.GetSizeX(),imgM.GetSizeY(),1);
	tmpI.GetVoxelData()=false;

	//... and retrieve the bounding box params
	VOI<PIXELS> voi;
	voi.offset.x=(int)imgM.GetSizeX(); //offset would search for "min" coordinate
	voi.offset.y=(int)imgM.GetSizeY();
	voi.size.x=0;                 //size (temporarily) would search for "max"
	voi.size.y=0;

	const LABEL* rI=imgM.GetVoxelAddr(0,0,slice);
	bool* rW=tmpI.GetFirstVoxelAddr();
	bool found=false;
	if (ID <= 0)
	{
		for (size_t i=0; i < tmpI.GetImageSize(); ++i, ++rI, ++rW)
			if (*rI > 0)
			{
				*rW=true; found=true;

				//look for extremal coordinates
				size_t xx=tmpI.GetX(i);
				size_t yy=tmpI.GetY(i);
				if (xx < (size_t)voi.offset.x) voi.offset.x=(int)xx;
				if (xx >         voi.size.x)   voi.size.x=xx;
				if (yy < (size_t)voi.offset.y) voi.offset.y=(int)yy;
				if (yy >         voi.size.y)   voi.size.y=yy;
			}
	}
	else
	{
		for (size_t i=0; i < tmpI.GetImageSize(); ++i, ++rI, ++rW)
			if (*rI == ID)
			{
				*rW=true; found=true;

				//look for extremal coordinates
				size_t xx=tmpI.GetX(i);
				size_t yy=tmpI.GetY(i);
				if (xx < (size_t)voi.offset.x) voi.offset.x=(int)xx;
				if (xx >         voi.size.x)   voi.size.x=xx;
				if (yy < (size_t)voi.offset.y) voi.offset.y=(int)yy;
				if (yy >         voi.size.y)   voi.size.y=yy;
			}
	}

	if (!found) {
		cerr << "mask not found in the image\n";
		throw;
	}

	//finish setting up the voi
	voi.offset.z=slice;
	voi.size.x-=voi.offset.x-1;
	voi.size.y-=voi.offset.y-1;
	voi.size.z=1;
	//and the local copy of the texture as well
	Image3d<GRAY16> textureOut_tmp;
	textureOut_tmp.MakeRoom(voi.size);
	textureOut_tmp.GetVoxelData()=0;
	//resolution is considered irrelevant from now, as OpenGL stretches it arbitrarily
	textureOut_tmp.SetResolution(Resolution(1,1,1));

	std::cout << "bbox of the cell: offset=" << voi.offset << ", size=" << voi.size << "\n";


	//erode the copy mask copy
	Erosion<bool>(tmpI,tmpE,nb2D_8);

	//here's the list of boundary points
	list<Pair> L;

	//time-savers:
	const float resx=imgM.GetResolution().GetX();
	const float resy=imgM.GetResolution().GetY();
	const float offx=imgM.GetOffset().x;
	const float offy=imgM.GetOffset().y;

	//geom centre
	double cX=0., cY=0.f;
	long int count=0;

	//see what's been eroded -- these are at the boundary
	bool* pB=tmpI.GetFirstVoxelAddr();
	rW=tmpE.GetFirstVoxelAddr();
	for (size_t i=0; i < tmpI.GetImageSize(); ++i, ++rW, ++pB)
	{
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

		if (*pB)
		{
			//found any point in the mask, let's copy out the texture
			size_t xx=tmpI.GetX(i);
			size_t yy=tmpI.GetY(i);

			const VOXEL tval=imgI.GetVoxel(xx,yy,slice);
			textureOut_tmp.SetVoxel(xx-voi.offset.x,yy-voi.offset.y,0, GRAY16(tval));
		}
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
		(*LI).a -= (float)cX;
		(*LI).b -= (float)cY;

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
	file << outputT << "\n";
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



	//textureOut_tmp.SaveImage(outputT); //TODO

	//note: angles in the point list are within 0 to 2PI interval
	
	//calculate image coordinate of the cell's centre point
	int oCx=int( (cX-offx)*resx ) -voi.offset.x;
	int oCy=int( (cY-offy)*resy ) -voi.offset.y;

	std::cout << "_tmp image size is: " << textureOut_tmp.GetSize() << "\n";
	std::cout << "cell centre within _tmp is: (" << oCx << "," << oCy << ")\n";

	//final texture image
	Image3d<GRAY16> textureOut;
	textureOut.MakeRoom(100,100,1);
	textureOut.GetVoxelData()=0;
	//resolution is considered irrelevant from now, as OpenGL stretches it arbitrarily
	textureOut.SetResolution(Resolution(1,1,1));

	int delta_Cx=(int)(textureOut_tmp.GetSizeX()/2) -oCx;
	int delta_Cy=(int)(textureOut_tmp.GetSizeY()/2) -oCy;
	std::cout << "cell centre should move by: (" << delta_Cx << "," << delta_Cy << ")\n";

	int newSizeX=(int)textureOut_tmp.GetSizeX() + 2*abs(delta_Cx);
	int newSizeY=(int)textureOut_tmp.GetSizeY() + 2*abs(delta_Cy);
	std::cout << "this suggests to resize the image to: " << newSizeX << " x " << newSizeY << "\n";

	if (delta_Cx < 0) delta_Cx=0;
	if (delta_Cy < 0) delta_Cy=0;
	delta_Cx*=2; //is now the offset of the original texture image in the new one
	delta_Cy*=2;
	std::cout << "and offset the orig _tmp at (" << delta_Cx << "," << delta_Cy << ")\n";

	float scaleX=1.f, scaleY=1.f;
	float edgeSize;
	if (newSizeX > newSizeY)
	{
		scaleY*=float(newSizeX) / float(newSizeY); //is wider...
		edgeSize=float(newSizeX);
	}
	else
	{
		scaleX*=float(newSizeY) / float(newSizeX); //is taller...
		edgeSize=float(newSizeY);
	}
	std::cout << "scaleX=" << scaleX << ", scaleY=" << scaleY << ", edgeSize=" << edgeSize << "\n";

	for (int yy=0; yy < 100; ++yy)
	for (int xx=0; xx < 100; ++xx)
	{
		float dx=float(xx);  //- 50.f;
		float dy=float(yy);  //- 50.f;

		dx*=edgeSize/100.f;
		dy*=edgeSize/100.f;

		dx/=scaleX;
		dy/=scaleY;

		dx-=float(delta_Cx);
		dy-=float(delta_Cy);

		if (textureOut_tmp.Include(int(dx),int(dy),0))
			textureOut.SetVoxel(xx,yy,0, textureOut_tmp.GetVoxel(size_t(dx),size_t(dy),0));
		else
			textureOut.SetVoxel(xx,yy,0, 0);
	}


	//this tells offset of the textureOut image within the tmpI which the same the both input images, except it is 2D
	voi.offset.x-=delta_Cx;
	voi.offset.y-=delta_Cy;

	oCx=(int)((( (cX-offx)*resx ) -voi.offset.x) * scaleX*100.f/edgeSize);
	oCy=(int)((( (cY-offy)*resy ) -voi.offset.y) * scaleY*100.f/edgeSize);
	if ((oCx != 50) || (oCy != 50))
	{
		cout << "Projected cell centre is not in the image centre!!\n";
		cout << "It is at (" << oCx << "," << oCy << ") instead of at (50,50).\n";
		cout << "It is, in fact, at ("
		     << (( (cX-offx)*resx ) -voi.offset.x) * scaleX*100.f/edgeSize
			  << ","
			  << (( (cY-offy)*resy ) -voi.offset.y) * scaleY*100.f/edgeSize
			  << ").\n";
		//throw;
	}


	//create list of boundary points in the texture image coordinates
	list<Pair> LT;
	LI=L.begin();
	while (LI != L.end())
	{
		float dx=( ((float)cX +LI->b*cos(LI->a) -offx)*resx ) -voi.offset.x;
		float dy=( ((float)cY +LI->b*sin(LI->a) -offy)*resy ) -voi.offset.y;
		dx*=scaleX*100.f/edgeSize;
		dy*=scaleY*100.f/edgeSize;
		if (textureOut.Include(int(dx),int(dy),0))
		{
			dx-=(float)oCx;
			dy-=(float)oCy;

			//calc its polar coordinate
			float ang=atan2(dy,dx);
			if (ang < 0) ang+=dPI;
			float rr=sqrt(dx*dx + dy*dy);

			//save the point on the list
			Pair p;
			p.a=ang;
			p.b=rr;
			LT.push_back(p);
		}
		else std::cout << "some point is outside...\n";

		LI++;
	}

/*
	//just draws the cell centre into the final(*) texture
	textureOut.SetVoxel(oCx,oCy,0,100);

	//just draws an outline into the final(*) texture
	LI=LT.begin();
	while (LI != LT.end())
	{
		int x=oCx + (int)round(LI->b * cos(LI->a));
		int y=oCy + (int)round(LI->b * sin(LI->a));
		if (textureOut.Include(x,y,0)) textureOut.SetVoxel(x,y,0,90);
		else std::cout << "(2) some point is outside...\n";

		LI++;
	}

	//(*): the texture that will be used in TRAgen but needs to
	//be further stretched beforethat so that the OpenGL mapping
	//will produce the expected result (something very very similar
	//to what the textureOut_tmp image holds now)
	//
	//basically, we are now in the middle of preprocessing of the
	//textureOut_tmp for the needs of the OpenGL mapper; during which
	//this routine above have injected some visual clues of boundary points
	textureOut.SaveImage("testOutline.tif");
*/


	//creates a copy of the image to hold the stretched image
	Image3d<GRAY16> textureOutB(textureOut);

	//now, sweep along boundaries and stretch/interpolate in
	//radial directions

	//top horizontal boundary
	for (int x=0; x < 100; ++x)
	{
		//texture image boundary coordinate relative to the image centre
		const float bdx=float(x)-50.f;
		const float bdy=-50.f;

		//now convert into polar angle
		float ang=atan2(bdy,bdx);
		if (ang < 0) ang+=dPI;

		//find the closest texture boundary point at this angle
		float bestAng=dPI;
		list<Pair>::iterator bestBP=LT.begin();

		LI=LT.begin();
		while (LI != LT.end())
		{
			float currAng=acos(cos(ang - LI->a));
			if (currAng < bestAng)
			{
				bestAng=currAng;
				bestBP=LI;
			}
			LI++;
		}

		//image coordinate of the boundary point closest
		//(in the azimuth perspective) to the current boundary point
		//
		//cell boundary point coordinate relative to the image centre
		float dx=bestBP->b * cos(bestBP->a);
		float dy=bestBP->b * sin(bestBP->a);

		//determine "optimal" stepping to travel from the centre towards the boundary
		//int maxStep=(fabs(bdx) > fabs(bdy))?  (int)floor(fabs(bdx)) : (int)floor(fabs(bdy));
		int maxStep=50;
		for (int qq=0; qq <= maxStep; ++qq)
		{
			//current relative position within the image
			int cdx=(int)(float(qq)/float(maxStep) *bdx);
			int cdy=(int)(float(qq)/float(maxStep) *bdy);

			//ditto with respect to the original cell boundary
			int sdx=(int)(float(qq)/float(maxStep) *dx);
			int sdy=(int)(float(qq)/float(maxStep) *dy);

			//get back the absolute coordinates
			cdx+=50; cdy+=50;
			sdx+=50; sdy+=50;
			if (textureOutB.Include(cdx,cdy,0))
			//if (textureOutB.Include(cdx,cdy,0) && textureOut.Include(sdx,sdy,0))
				textureOutB.SetVoxel(cdx,cdy,0, textureOut.GetVoxel(sdx,sdy,0));
		}
	}

	//right vertical boundary
	for (int y=0; y < 100; ++y)
	{
		const float bdx=49.f;
		const float bdy=float(y)-50.f;

		//now convert into polar angle
		float ang=atan2(bdy,bdx);
		if (ang < 0) ang+=dPI;

		//find the closest texture boundary point at this angle
		float bestAng=dPI;
		list<Pair>::iterator bestBP=LT.begin();

		LI=LT.begin();
		while (LI != LT.end())
		{
			float currAng=acos(cos(ang - LI->a));
			if (currAng < bestAng)
			{
				bestAng=currAng;
				bestBP=LI;
			}
			LI++;
		}

		//image coordinate of the boundary point closest
		//(in the azimuth perspective) to the current boundary point
		//
		//cell boundary point coordinate relative to the image centre
		float dx=bestBP->b * cos(bestBP->a);
		float dy=bestBP->b * sin(bestBP->a);

		//determine "optimal" stepping to travel from the centre towards the boundary
		//int maxStep=(fabs(bdx) > fabs(bdy))?  (int)floor(fabs(bdx)) : (int)floor(fabs(bdy));
		int maxStep=50;
		for (int qq=0; qq <= maxStep; ++qq)
		{
			//current relative position within the image
			int cdx=(int)(float(qq)/float(maxStep) *bdx);
			int cdy=(int)(float(qq)/float(maxStep) *bdy);

			//ditto with respect to the original cell boundary
			int sdx=(int)(float(qq)/float(maxStep) *dx);
			int sdy=(int)(float(qq)/float(maxStep) *dy);

			//get back the absolute coordinates
			cdx+=50; cdy+=50;
			sdx+=50; sdy+=50;
			if (textureOutB.Include(cdx,cdy,0))
			//if (textureOutB.Include(cdx,cdy,0) && textureOut.Include(sdx,sdy,0))
				textureOutB.SetVoxel(cdx,cdy,0, textureOut.GetVoxel(sdx,sdy,0));
		}
	}

	//bottom horizontal boundary
	for (int x=0; x < 100; ++x)
	{
		const float bdx=49.f-float(x);
		const float bdy=49.f;
		/*
		const float bdy=50.f; //would expect here 49.f but then the integer
		                      //stepping below manages to ignore one pixel line
		                      //yielding an unpleasant artifact in the image
		*/

		//now convert into polar angle
		float ang=atan2(bdy,bdx);
		if (ang < 0) ang+=dPI;

		//find the closest texture boundary point at this angle
		float bestAng=dPI;
		list<Pair>::iterator bestBP=LT.begin();

		LI=LT.begin();
		while (LI != LT.end())
		{
			float currAng=acos(cos(ang - LI->a));
			if (currAng < bestAng)
			{
				bestAng=currAng;
				bestBP=LI;
			}
			LI++;
		}

		//image coordinate of the boundary point closest
		//(in the azimuth perspective) to the current boundary point
		//
		//cell boundary point coordinate relative to the image centre
		float dx=bestBP->b * cos(bestBP->a);
		float dy=bestBP->b * sin(bestBP->a);

		//determine "optimal" stepping to travel from the centre towards the boundary
		//int maxStep=(fabs(bdx) > fabs(bdy))?  (int)floor(fabs(bdx)) : (int)floor(fabs(bdy));
		int maxStep=50;
		for (int qq=0; qq <= maxStep; ++qq)
		{
			//current relative position within the image
			int cdx=(int)(float(qq)/float(maxStep) *bdx);
			int cdy=(int)(float(qq)/float(maxStep) *bdy);

			//ditto with respect to the original cell boundary
			int sdx=(int)(float(qq)/float(maxStep) *dx);
			int sdy=(int)(float(qq)/float(maxStep) *dy);

			//get back the absolute coordinates
			cdx+=50; cdy+=50;
			sdx+=50; sdy+=50;
			if (textureOutB.Include(cdx,cdy,0))
			//if (textureOutB.Include(cdx,cdy,0) && textureOut.Include(sdx,sdy,0))
				textureOutB.SetVoxel(cdx,cdy,0, textureOut.GetVoxel(sdx,sdy,0));
		}
	}

	//left vertical boundary
	for (int y=0; y < 100; ++y)
	{
		const float bdx=-50.f;
		const float bdy=49.f-float(y);

		//now convert into polar angle
		float ang=atan2(bdy,bdx);
		if (ang < 0) ang+=dPI;

		//find the closest texture boundary point at this angle
		float bestAng=dPI;
		list<Pair>::iterator bestBP=LT.begin();

		LI=LT.begin();
		while (LI != LT.end())
		{
			float currAng=acos(cos(ang - LI->a));
			if (currAng < bestAng)
			{
				bestAng=currAng;
				bestBP=LI;
			}
			LI++;
		}

		//image coordinate of the boundary point closest
		//(in the azimuth perspective) to the current boundary point
		//
		//cell boundary point coordinate relative to the image centre
		float dx=bestBP->b * cos(bestBP->a);
		float dy=bestBP->b * sin(bestBP->a);

		//determine "optimal" stepping to travel from the centre towards the boundary
		//int maxStep=(fabs(bdx) > fabs(bdy))?  (int)floor(fabs(bdx)) : (int)floor(fabs(bdy));
		int maxStep=50;
		for (int qq=0; qq <= maxStep; ++qq)
		{
			//current relative position within the image
			int cdx=(int)(float(qq)/float(maxStep) *bdx);
			int cdy=(int)(float(qq)/float(maxStep) *bdy);

			//ditto with respect to the original cell boundary
			int sdx=(int)(float(qq)/float(maxStep) *dx);
			int sdy=(int)(float(qq)/float(maxStep) *dy);

			//get back the absolute coordinates
			cdx+=50; cdy+=50;
			sdx+=50; sdy+=50;
			if (textureOutB.Include(cdx,cdy,0))
			//if (textureOutB.Include(cdx,cdy,0) && textureOut.Include(sdx,sdy,0))
				textureOutB.SetVoxel(cdx,cdy,0, textureOut.GetVoxel(sdx,sdy,0));
		}
	}

	//textureOutB.SaveImage("test.tif");
	textureOutB.SaveImage(outputT);
}


int main(int argc, char **argv)
{
	 if (argc != 7)
	 {
	 	cout << "Extracts list of boundary points of a particular cell...\n\n";
		cout << "params: texture_file.ics mask_file.ics cell_ID z_slice output_file.txt output_texture.ics\n";
		cout << "If cell_ID <= 0 then any non-zero pixel is treated as cell mask pixel.\n";
		return(2);
	 }
    try
    {
		cerr << "Reading texture image Type" << endl;
		ImgVoxelType vtI = ReadImageType(argv[1]);
		cerr << "Voxel=" << VoxelTypeToString(vtI) << endl;

		cerr << "Reading mask image Type" << endl;
		ImgVoxelType vtL = ReadImageType(argv[2]);
		cerr << "Voxel=" << VoxelTypeToString(vtL) << endl;

		int ID=atoi(argv[3]);
		int sl=atoi(argv[4]);

		if ((vtI == Gray8Voxel) && (vtL == Gray8Voxel))
		{
			Image3d<GRAY8> inTexture(argv[1]);
			Image3d<GRAY8> inMask(argv[2]);
			Extract(inTexture,inMask,(GRAY8)ID,sl,argv[5],argv[6]);
		}
		else if ((vtI == Gray8Voxel) && (vtL == Gray16Voxel))
		{
			Image3d<GRAY8> inTexture(argv[1]);
			Image3d<GRAY16> inMask(argv[2]);
			Extract(inTexture,inMask,(GRAY16)ID,sl,argv[5],argv[6]);
		}
		else if ((vtI == Gray16Voxel) && (vtL == Gray8Voxel))
		{
			Image3d<GRAY16> inTexture(argv[1]);
			Image3d<GRAY8> inMask(argv[2]);
			Extract(inTexture,inMask,(GRAY8)ID,sl,argv[5],argv[6]);
		}
		else if ((vtI == Gray16Voxel) && (vtL == Gray16Voxel))
		{
			Image3d<GRAY16> inTexture(argv[1]);
			Image3d<GRAY16> inMask(argv[2]);
			Extract(inTexture,inMask,(GRAY16)ID,sl,argv[5],argv[6]);
		}
		else if ((vtI == FloatVoxel) && (vtL == Gray8Voxel))
		{
			Image3d<float> inTexture(argv[1]);
			Image3d<GRAY8> inMask(argv[2]);
			Extract(inTexture,inMask,(GRAY8)ID,sl,argv[5],argv[6]);
		}
		else if ((vtI == FloatVoxel) && (vtL == Gray16Voxel))
		{
			Image3d<float> inTexture(argv[1]);
			Image3d<GRAY16> inMask(argv[2]);
			Extract(inTexture,inMask,(GRAY16)ID,sl,argv[5],argv[6]);
		}
		else
		{
			cerr << "Other voxel types or this combination is not supported" << endl;
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
