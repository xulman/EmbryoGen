#include <iostream>
#include <i3d/image3d.h>
#include <i3d/regions.h>

using namespace std;
using namespace i3d;

template <class VOXEL>
void Label(const Image3d<VOXEL>& imgB,Image3d<VOXEL>& imgL)
{
	i3d::LabeledImage3d<VOXEL,VOXEL> tmpL;
	tmpL.CreateForegroundRegionsFF(imgB);
	if (tmpL.Convert(imgL) == false)
	{
		cerr << "output image voxel type cannot store this many detected components ("
		     << tmpL.NumberOfComponents() << ")\n";
		throw;
	}
}


int main(int argc, char **argv)
{
	 if (argc != 3)
	 {
	 	cout << "Creates a labeled copy of input binary mask image...\n\n";
		cout << "params: input_binary_mask.ics output_labeled_mask.ics\n";
		cout << "All non-zero pixels in the input image are considered.\n";
		return(2);
	 }
    try
    {
		cerr << "Reading mask image Type" << endl;
		ImgVoxelType vtL = ReadImageType(argv[1]);
		cerr << "Voxel=" << VoxelTypeToString(vtL) << endl;

		if (vtL == Gray8Voxel)
		{
			Image3d<GRAY8> inMask(argv[1]);
			Image3d<GRAY8> outMask(argv[2]);
			Label(inMask,outMask);
		}
		else if (vtL == Gray16Voxel)
		{
			Image3d<GRAY16> inMask(argv[1]);
			Image3d<GRAY16> outMask(argv[2]);
			Label(inMask,outMask);
		}
		else
		{
			cerr << "Other voxel types are not supported" << endl;
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
