#!/bin/bash

# --------- config ---------
DEPLOYDIR=$PWD/DEPLOY
MAKEJOBS=8

prepare_ICS=YES
prepare_GSL=YES
prepare_LAPACKwithBLAS=YES
prepare_ZLIB=YES
prepare_TIFF=YES
prepare_FFTW=YES
prepare_F2C=YES
prepare_I3D=YES


mkdir -p $DEPLOYDIR

# this makes sure that all .a files end up in one folder
mkdir -p $DEPLOYDIR/lib64
test ! -e $DEPLOYDIR/lib && ln -vs $DEPLOYDIR/lib64  $DEPLOYDIR/lib

echo ========================= ICS =========================
if [ "_$prepare_ICS" == "_YES" ]; then
	ICS_archive=1.6.4.tar.gz
	wget https://github.com/svi-opensource/libics/archive/$ICS_archive
	tar -xzf $ICS_archive
	cd libics-${ICS_archive:0:5}
	mkdir -p BUILD
	cd BUILD
	cmake -D CMAKE_INSTALL_PREFIX=$DEPLOYDIR ..
	make -j${MAKEJOBS}
	make install
	cd ../..
else
	echo "not now"
fi

echo ========================= GSL =========================
if [ "_$prepare_GSL" == "_YES" ]; then
	GSL_archive=gsl-2.6.tar.gz
	wget ftp://ftp.gnu.org/gnu/gsl/$GSL_archive
	tar -xzf $GSL_archive
	cd ${GSL_archive:0:7}
	./configure  --prefix=$DEPLOYDIR
	make -j${MAKEJOBS}
	make install
	cd ..
else
	echo "not now"
fi

echo ========================= LAPACK + BLAS =========================
if [ "_$prepare_LAPACKwithBLAS" == "_YES" ]; then
	LAPACK_archive=lapack-3.8.0.zip
	wget https://github.com/Reference-LAPACK/lapack-release/archive/$LAPACK_archive
	unzip $LAPACK_archive
	cd lapack-release-lapack-${LAPACK_archive:7:5}
	mkdir -p BUILD
	cd BUILD
	cmake -D CBLAS=ON -D LAPACKE=ON -D CMAKE_INSTALL_PREFIX=$DEPLOYDIR ..
	make -j${MAKEJOBS}
	make install
	cd ../..
else
	echo "not now"
fi

echo ========================= ZLIB =========================
if [ "_$prepare_ZLIB" == "_YES" ]; then
	ZLIB_archive=zlib-1.2.11.tar.gz
	wget https://www.zlib.net/$ZLIB_archive
	tar -xzf $ZLIB_archive
	cd ${ZLIB_archive:0:11}
	mkdir -p BUILD
	cd BUILD
	cmake -D CMAKE_INSTALL_PREFIX=$DEPLOYDIR ..
	make -j${MAKEJOBS}
	make install
	cd ../..
else
	echo "not now"
fi

echo ========================= TIFF =========================
if [ "_$prepare_TIFF" == "_YES" ]; then
	TIFF_archive=tiff-4.0.10.tar.gz
	wget http://download.osgeo.org/libtiff/$TIFF_archive
	tar -xzf $TIFF_archive
	cd ${TIFF_archive:0:11}
	mkdir -p BUILD
	cd BUILD
	cmake -D BUILD_SHARED_LIBS=OFF -D jbig=OFF -D jpeg=OFF -D jpeg12=OFF -D webp=OFF -D zstd=OFF -D CMAKE_INSTALL_PREFIX=$DEPLOYDIR ..
	make -j${MAKEJOBS}
	make install
	cd ../..
else
	echo "not now"
fi

echo ========================= FFTW =========================
if [ "_$prepare_FFTW" == "_YES" ]; then
	FFTW_archive=fftw-3.3.8.tar.gz
	wget http://www.fftw.org/$FFTW_archive
	tar -xzf $FFTW_archive
	cd ${FFTW_archive:0:10}
	mkdir -p BUILD
	cd BUILD
	cmake -D BUILD_SHARED_LIBS=OFF -D DISABLE_FORTRAN=ON -D ENABLE_FLOAT=ON  -D ENABLE_LONG_DOUBLE=OFF -D ENABLE_THREADS=ON -D CMAKE_INSTALL_PREFIX=$DEPLOYDIR ..
	make -j${MAKEJOBS} install
	cmake -D BUILD_SHARED_LIBS=OFF -D DISABLE_FORTRAN=ON -D ENABLE_FLOAT=OFF -D ENABLE_LONG_DOUBLE=OFF -D ENABLE_THREADS=ON -D CMAKE_INSTALL_PREFIX=$DEPLOYDIR ..
	make -j${MAKEJOBS} install
	cmake -D BUILD_SHARED_LIBS=OFF -D DISABLE_FORTRAN=ON -D ENABLE_FLOAT=OFF -D ENABLE_LONG_DOUBLE=ON  -D ENABLE_THREADS=ON -D CMAKE_INSTALL_PREFIX=$DEPLOYDIR ..
	make -j${MAKEJOBS} install
	cd ../..
else
	echo "not now"
fi

echo ========================= F2C =========================
if [ "_$prepare_F2C" == "_YES" ]; then
	F2C_archive=libf2c.zip
	wget https://www.netlib.org/f2c/$F2C_archive
	mkdir -p ${F2C_archive:0:6}
	cd ${F2C_archive:0:6}
	unzip ../$F2C_archive
	cp -i makefile.u makefile
	make hadd
	make
	cp -v libf2c.a $DEPLOYDIR/lib64
	cd ..
else
	echo "not now"
fi

echo ========================= I3D =========================
if [ "_$prepare_I3D" == "_YES" ]; then
	git clone git@gitlab.fi.muni.cz:cbia/I3DLIB.git i3dlibs
	cd i3dlibs
	mkdir -p BUILD
	cd BUILD
	cmake -D ALGO_WITH_BLAS=ON -D ALGO_WITH_LAPACK=ON -D CORE_WITH_BIOFORMATS=OFF -D CORE_WITH_DCM=OFF \
	      -D CORE_WITH_HDF5=OFF -D CORE_WITH_JPEG=OFF -D CORE_WITH_METAIO=OFF -D CORE_WITH_PNG=OFF \
	      -D CORE_WITH_TARGA=OFF -D GLOBAL_BUILD_SHARED_LIBS=OFF \
	      -D CORE_ICS_HEADERS_REL=$DEPLOYDIR/include \
	      -D CORE_ICS_LIB_REL=$DEPLOYDIR/lib/libics.a \
	      -D CORE_TIFF_HEADERS_REL=$DEPLOYDIR/include \
	      -D CORE_TIFF_LIB_REL=$DEPLOYDIR/lib64/libtiff.a \
	      -D CORE_ZLIB_HEADERS_REL=$DEPLOYDIR/include \
	      -D CORE_ZLIB_LIB_REL=$DEPLOYDIR/lib/libz.a \
	      -D ALGO_FFTW_HEADERS_REL=$DEPLOYDIR/include \
	      -D ALGO_FFTW_LIB_REL=$DEPLOYDIR/lib64/libfftw3.a \
	      -D ALGO_FFTW_THREADS_LIB_REL=$DEPLOYDIR/lib64/libfftw3_threads.a \
	      -D ALGO_FFTWF_LIB_REL=$DEPLOYDIR/lib64/libfftw3f.a \
	      -D ALGO_FFTWF_THREADS_LIB_REL=$DEPLOYDIR/lib64/libfftw3f_threads.a \
	      -D ALGO_FFTWL_LIB_REL=$DEPLOYDIR/lib64/libfftw3l.a \
	      -D ALGO_FFTWL_THREADS_LIB_REL=$DEPLOYDIR/lib64/libfftw3l_threads.a \
	      -D ALGO_F2C_LIB_REL=$DEPLOYDIR/lib64/libf2c.a \
	      -D ALGO_BLAS_LIB_REL=$DEPLOYDIR/lib64/libblas.a \
	      -D ALGO_LAPACK_LIB_REL=$DEPLOYDIR/lib64/liblapack.a \
	      -D GLOBAL_INSTALL_PATH=$DEPLOYDIR ..

	make -j${MAKEJOBS}
	make install
	cd ../..
else
	echo "not now"
fi
