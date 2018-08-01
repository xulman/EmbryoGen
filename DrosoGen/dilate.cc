/*
compile command: g++ -Wall dilate.cc -o dilate -li3dalgo -li3dcore
of_transpose mask000.tif mask000_T__.tif yz
./dilate, one stage after another one
of_transpose mask000_T_dilated_done2.tif DrosophilaYolk_mask.tif yz
*/

#include <i3d/image3d.h>
#include <i3d/morphology.h>
#include <i3d/draw.h>

int main(void)
{
	/*
	std::cout << "reading\n";
	i3d::Image3d<i3d::GRAY8> a("/Users/ulman/devel/EmbryoGen/DrosoGen/BUILD/mask000_T__.tif"),b;
	std::cout << "dilation\n";
	i3d::DilationO(a,b,10);
	std::cout << "writing\n";
	b.SaveImage("/Users/ulman/devel/EmbryoGen/DrosoGen/BUILD/mask000_T_dilated.tif");
	*/


	/*
	std::cout << "reading\n";
	i3d::Image3d<i3d::GRAY8> b("/Users/ulman/devel/EmbryoGen/DrosoGen/BUILD/mask000_T_dilated.tif"),a;
	i3d::DilationO(b,a,2);

	std::cout << "filling\n";
	i3d::FloodFill(a,b,(i3d::GRAY8)10,0);
	//b.SaveImage("/Users/ulman/devel/EmbryoGen/DrosoGen/BUILD/mask000_T_dilated_flooded.tif");

	std::cout << "thresholding\n";
	//now extract only interior
	i3d::GRAY8* p = b.GetFirstVoxelAddr();
	i3d::GRAY8* const pS = p + b.GetImageSize();
	while (p != pS)
	{
		*p = *p < 10 ? 255 : 0;
		++p;
	}

	std::cout << "postprocessing\n";
	i3d::OpeningO(b,a,15);

	std::cout << "saving\n";
	a.SaveImage("/Users/ulman/devel/EmbryoGen/DrosoGen/BUILD/mask000_T_dilated_done.tif");
	*/


	i3d::Image3d<i3d::GRAY8> a("/Users/ulman/devel/EmbryoGen/DrosoGen/BUILD/mask000_T_dilated_done.tif"),b;
	i3d::DilationO(a,b,15);
	b.SaveImage("/Users/ulman/devel/EmbryoGen/DrosoGen/BUILD/mask000_T_dilated_done2.tif");

	return (0);
}
