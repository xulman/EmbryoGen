# EmbryoGen
![Example: TRAgen3D (C++) sends images to scenerygraphics/scenery (Java + Kotlin)](https://www.fi.muni.cz/~xulman/files/TRAgen-demo/EmbryoGen_earlyVersion.png)

# About
This is [my](http://www.fi.muni.cz/~xulman) next-gen simulator, in which I am
trying to implement the best of from my previous and current work in the field
of simulators/generators for producing realistically looking images for bechmarking
of the image processing, namely cell/nuclei segmentation and tracking.

The simulator can display its progress online via various tools, e.g.
[ZeroMQ](https://github.com/zeromq/libzmq)-ish sends display commands
to a [SimViewer](https://github.com/xulman/SimViewer).

# Installing and starting the simulator
First, resolve all dependecies --- make sure this is installed on your system:
- [CMake](https://cmake.org/)
- [gsl](https://www.gnu.org/software/gsl/)
- [libzmq](https://github.com/zeromq/libzmq) & [cppzmq](https://github.com/zeromq/cppzmq)
- [i3dlibs](https://cbia.fi.muni.cz/software/i3d-library.html)

Second, download, configure, build and run:
```
git clone https://github.com/xulman/EmbryoGen.git
cd EmbryoGen/
mkdir BUILD
cd BUILD/
ccmake ..
make -j4
ln -s ../2013-07-25_1_1_9_0_2_0_0_1_0_0_0_0_9_12.ics
./embryogen randomDrosophila
```
