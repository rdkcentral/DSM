#!/bin/bash

#@description   Applies a patches/new files to add Dobby+DSM to an RDKB Raspberry Pi Model 3 B++ build for 22q1 (ie RDKB source code from the 1st quarter of 2022)
#@param[in]     $1      target file directory path [NB: default is "."]
#@param[in]     $2      patch/new files base directory path containing folders "./patches" and "./new"  [NB: default is "./DSM/integration/rdk-b-22q1/dsm/rpi"]


function apply_patch(){
#@description   Applies a patch file, using the 'patch' command given a relative path file name, to the corresponding file in the target directory path
#@param[in]     $1      relative file path name
#@param[in]     $2      target file directory path
#@param[in]     $3      patch files directory path [NB: default is "."]

patch_dir=.
if [ ! "$3" == "" ]; then
        patch_dir=$3/$(dirname $1)
    if [ ! -d $patch_dir ]; then
        mkdir -p $patch_dir
    fi
fi

echo "====>>>"
echo "patch file : $patch_dir/$(ls $patch_dir)"

if  [ -f $patch_dir/$(basename $1).patch ]; then
	echo "patching $2/$1 from $patch_file"

elif [ -f $patch_dir/$(basename $1) ]; then
	echo "copy $patch_dir/$1 to $2/$1"

else
	echo "**** Error ! **** : patch/new file does not exist for $2/$1"
	exit 1
fi

file=$1
if [ -f $patch_dir/$(basename $1).patch ]; then

    # apply a patch
    patch -u -b $2/$1 -i $patch_dir/$(basename $1).patch
    ls -l $2/$1
    ls -l $patch_dir/$(basename $1).patch

else

    target_file="$2/$1"
    if [ -f $target_file ]; then
        echo "**** Error ! **** : attempting to overwite a file which already exists. Suggest creating a patch for $target_file"
        exit 2
    fi

    if [ ! -f $patch_dir/$(basename $1) ]; then
	echo "**** Error ! **** : expected patch file directory to exist $patch_dir/$(basename $1)"
        exit 3
    fi

    if [ ! -d $2/$(basename $1) ]; then
        mkdir -p $2/$(basename $1)
        echo "**** Status ! **** : created target directory $2/$(basename $1)"
    fi

    #copy the file which does not yet exist in the target directory
    cp $patch_dir/$(basename $1) $target_file
    echo "New: $(ls ${patch_dir}/$(basename $1))"
    echo "New added to target: $target_file"

    if [ ! -f $target_file ]; then
        echo "**** Error ! **** : expected new target file to exist $target_file"
        exit 4
    fi

fi

echo "====<<<"
}


#defaults
target=.
patch_new_files_dir=./DSM/integration/rdk-b-22q1/dsm/rpi

#parameters
if [ ! "$1" == "" ]; then
    target=$1
fi
if [ ! "$2" == "" ]; then
    patch_new_files_dir=$2
fi

#checks
if [ ! -d "$target" ]; then
    echo "**** Error ! **** : expected target directory to exist $target"
    exit 5
fi
if [ ! -d "$patch_new_files_dir" ]; then
    echo "**** Error ! **** : expected patches/new files directory to exist $patch_new_files_dir"
    exit 6
fi

#apply the patches to add dobby+dsm to a 22q1 RDKB build for Rpi
patches=$patch_new_files_dir/patches
newbies=$patch_new_files_dir/new
echo "patches=$patches"
echo "newbies=$newbies"
echo "using targets in $target "
echo "using patches in $patches "
echo "using newbies in $newbies "

   # dobby patches
apply_patch meta-cmf-raspberrypi/conf/machine/raspberrypi-rdk-broadband.conf $target $patches
apply_patch meta-raspberrypi/recipes-kernel/linux/linux-raspberrypi_5.4.bb $target $patches
apply_patch meta-raspberrypi/recipes-kernel/linux/linux-raspberrypi.inc $target $patches
apply_patch meta-rdk/recipes-containers/dobby/dobby.bb $target $patches
    # dobby+dsm patches
apply_patch meta-rdk-broadband/recipes-core/packagegroups/packagegroup-rdk-oss-broadband.bbappend $target $patches
    # dobby+dsm new files
apply_patch meta-raspberrypi/recipes-kernel/linux/files/container.cfg $target $newbies
apply_patch meta-rdk/recipes-containers/dsm/dsm.bb $target $newbies
