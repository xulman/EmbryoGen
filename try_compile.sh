#! /bin/bash

set -e

if [ -z "$1" ]; then
    echo "Enter build directory as first argument please ( ./try_compile BUILD_DIR )"
    exit 1
fi

CURR_D="`pwd`"

if ! cd "$1"; then
    exit 1;
fi

BUILD_D="`pwd`"
cd - >/dev/null

cp "$CURR_D/src/config.hpp" "$CURR_D/src/config.hpp.old"
cp "$BUILD_D/CMakeCache.txt" "$BUILD_D/CMakeCache.txt.old"

# Config
CHANGE_PREC=T
CHANGE_DBG=T
CHANGE_DIST=T
CHANGE_DIST_DBG=F
CHANGE_STATIC=T

if [ "$2" = "all" ]; then
    CHANGE_PREC=T
    CHANGE_DBG=T
    CHANGE_DIST=T
    CHANGE_DIST_DBG=T
    CHANGE_STATIC=T
fi

if [ "$2" = "prec" ]; then
    CHANGE_PREC=T
    CHANGE_DBG=F
    CHANGE_DIST=F
    CHANGE_DIST_DBG=F
    CHANGE_STATIC=F
fi

if [ "$2" = "dbg" ]; then
    CHANGE_PREC=F
    CHANGE_DBG=T
    CHANGE_DIST=F
    CHANGE_DIST_DBG=F
    CHANGE_STATIC=F
fi

# float/double
change_prec()
{
    echo "Changing precision to $1"
    sed -i "s/using precision_t.*$/using precision_t = $1;/" "${CURR_D}/src/config.hpp"
    return $?
}

# OPTION VALUE
change_cmake()
{
    echo "Changing $1 to $2"
    if ! grep -q "^$1=" "${BUILD_D}/CMakeCache.txt"; then
        echo ERROR: $1 not found 
        exit 1
    fi
    sed -i "s/^$1=.*$/$1=$2/" "${BUILD_D}/CMakeCache.txt"
    return $? 
}

# ON/OFF
change_dbg()
{
    change_cmake "DEBUG_VERSION:BOOL" "$1"
    return $? 
}

# ON/OFF
change_dist()
{
    change_cmake "RUN_DISTRIBUTED:BOOL" "$1"
    return $?
}

# ON/OFF
change_dist_dbg()
{
    change_cmake "DEBUG_DIST_VERSION:BOOL" "$1"
    return $?
}

# ON/OFF
change_static()
{
    change_cmake "BUILD_STATIC:BOOL" "$1"
    return $?
}

if [ $CHANGE_PREC = T ]; then
    PREC_OPTIONS="float double"
else
    PREC_OPTIONS="NONE"
fi

if [ $CHANGE_DBG = T ]; then
    DBG_OPTIONS="ON OFF"
else
    DBG_OPTIONS="NONE"
fi

if [ $CHANGE_DIST = T ]; then
    DIST_OPTIONS="ON OFF"
else
    DIST_OPTIONS="NONE"
fi

if [ $CHANGE_DIST_DBG = T ]; then
    DIST_DBG_OPTIONS="ON OFF"
else
    DIST_DBG_OPTIONS="NONE"
fi


if [ $CHANGE_STATIC = T ]; then
    STATIC_OPTIONS="ON OFF"
else
    STATIC_OPTIONS="NONE"
fi

for PREC_OPT in $PREC_OPTIONS; do
    if ! [ $PREC_OPT = "NONE" ]; then
        change_prec $PREC_OPT
    fi
    
    for DBG_OPT in $DBG_OPTIONS; do
        if ! [ $DBG_OPT = "NONE" ]; then
            change_dbg $DBG_OPT
        fi

        for DIST_OPT in $DIST_OPTIONS; do
            if ! [ $DIST_OPT = "NONE" ]; then
                change_dist $DIST_OPT
            fi

            for DIST_DBG_OPT in $DIST_DBG_OPTIONS; do
                if ! [ $DIST_DBG_OPT = "NONE" ]; then
                    change_dist_dbg $DIST_DBG_OPT
                fi

                for STATIC_OPT in $STATIC_OPTIONS; do
                    if ! [ $STATIC_OPT = "NONE" ]; then
                        change_static $STATIC_OPT
                    fi

                    cd "$BUILD_D"

                    if ! make -j 8; then
                        exit 1;
                    fi

                    cd - > /dev/null

                done
            done
        done
    done
done


echo "Successfully finished"
mv "$CURR_D/src/config.hpp.old" "$CURR_D/src/config.hpp" 
mv "$BUILD_D/CMakeCache.txt.old" "$BUILD_D/CMakeCache.txt" 



