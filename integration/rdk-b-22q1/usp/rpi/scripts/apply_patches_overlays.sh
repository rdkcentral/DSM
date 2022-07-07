#!/bin/bash

#@description   Applies a patches/new files to add USP-PA Agent to an RDKB Raspberry Pi Model 3 B++ build for 22q1 (ie RDKB source code from the 1st quarter of 2022)
#@param[in]     $1      target file directory path [NB: default is "."]
#@param[in]     $2      patch/new files base directory path containing folders "./patches" and "./new"  [NB: default is "./DSM/integration/rdk-b-22q1/dsm/rpi"]

# If not stated otherwise in this file or this component's license file the
# following copyright and licenses apply:
#
# Copyright 2022 Consult Red
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http:#www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

function remove_patch(){
#@description   Removes any patch which has been applied in the target directory path
#@param[in]     $1      relative file path name
#@param[in]     $2      target file directory path
#@param[in]     $3      patch files directory path [NB: default is "."]

patch_dir=.
if [ ! "$3" == "" ]; then
    patch_dir=$3/$(dirname $1)
fi

echo "====>>> ** REMOVE PATCH/NEW **"
echo "patch file : $patch_dir/$(ls $patch_dir)"
echo "target file : $2/$1"

if [ -f $patch_dir/$(basename $1).patch ]; then

    # strip off any leading ./
    file=$2/$1
    if [ "${file:0:1}" == "." ]; then
        file=$(echo "$file" | awk '{ print substr ($0, 3 ) }')
    fi

    # obtain the first leaf of the directory path
    git_file=${file#*/}
    git_repo=${file%"$git_file"}

    echo "git_repo='$git_repo'"
    echo "git_file='$git_file'"

    src=$(pwd)

    # remove a patch
    cd $git_repo
    git checkout $git_file

    if [ -f ${git_file}.orig ]; then
        rm ${git_file}.orig
    fi

    if [ -f ${git_file}.rej ]; then
        rm ${git_file}.rej
    fi

    git log -1 $git_file

    cd $src

    ls -l $2/$1*
    ls -l $patch_dir/$(basename $1).patch

elif [ -f $patch_dir/$(basename $1) ]; then

    target_file="$2/$1"
    if [ -f $target_file ]; then

        # remove a target file
        rm "$2/$1"

        echo "New: $(ls ${patch_dir}/$(basename $1))"
        echo "New removed from target: $target_file"

        if [ -f $target_file ]; then
            echo "**** Error ! **** : expected new target file to not exist $target_file"
            exit 6
        fi

    else
        echo "New: $(ls ${patch_dir}/$(basename $1))"
        echo "New already removed from target: $target_file"

    fi

else
    echo "**** Error ! **** : no patch/new file provided for $1 in directory $2"
    exit 7
fi

echo "====<<<"
}

function apply_patch(){
#@description   Applies a patch file, using the 'patch' command give a relative path file name, to the corresponding file in the target directory path
#@param[in]     $1      relative file path name
#@param[in]     $2      target file directory path
#@param[in]     $3      patch files directory path [NB: default is "."]

if [ ! "$remove_patches" == "" ]; then
    remove_patch $1 $2 $3
    return
fi

patch_dir=.
if [ ! "$3" == "" ]; then
    patch_dir=$3/$(dirname $1)
    if [ ! -d $patch_dir ]; then
        mkdir -p $patch_dir
    fi
fi

echo "====>>>"
echo "patching : $2/$1"

if  [ -f $patch_dir/$(basename $1).patch ]; then
	echo "patching $2/$1 from $patch_file"

elif [ -f $patch_dir/$(basename $1) ]; then
	echo "copy $patch_dir/$1 to $2/$1"

else
	echo "**** Error ! **** : patch/new file does not exist for $2/$1"
	exit 3
fi

if [ -f $patch_dir/$(basename $1).patch ]; then

    # apply a patch
    patch -u -b $2/$1 -i $patch_dir/$(basename $1).patch

    ls -l $2/$1*
    ls -l $patch_dir/$(basename $1).patch

elif [ -f $patch_dir/$(basename $1) ]; then

    target_file="$2/$1"
    if [ -f $target_file ]; then
        echo "**** Error ! **** : attempting to overwite a file which already exists. Suggest creating a patch for $target_file"
        exit 2
    fi

    if [ ! -f $patch_dir/$(basename $1) ]; then
	echo "**** Error ! **** : expected patch file directory to exist $patch_dir/$(basename $1)"
        exit 3
    fi

    if [ ! -d $2/$(dirname $1) ]; then
        mkdir -p $2/$(dirname $1)
        echo "**** Status ! **** : created target directory $2/$(dirname $1)"
    fi

    #copy the file which does not yet exist in the target directory
    cp $patch_dir/$(basename $1) $target_file
    echo "New: $(ls ${patch_dir}/$(basename $1))"
    echo "New added to target: $target_file"

    if [ ! -f $target_file ]; then
        echo "**** Error ! **** : expected new target file to exist $target_file"
        exit 4
    fi

else
    echo "**** Error ! **** : no patch/new file provided for $1 in directory $2"
    exit 5

fi

echo "====<<<"
}


#defaults
target=.
patch_new_files_dir=./DSM/integration/rdk-b-22q1/usp/rpi

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

echo "====>>>"

#apply the patches to add USP-PA to a 22q1 RDKB build for Rpi
patches=$patch_new_files_dir/patches
newbies=$patch_new_files_dir/new
echo "patches=$patches"
echo "using targets in $target "
echo "using patches in $patches "
echo "using newbies in $newbies "

   # usp-pa patches
if [ "$3" == "" ]; then
    apply_patch meta-rdk-broadband/recipes-rdkb/usp-pa/usp-pa.bb $target $patches
fi
if [ "$3" == "new" ]; then
    apply_patch vendor/lcm_datamodel.c $target $newbies
fi
if [ "$3" == "vendor" ]; then
    apply_patch ./vendor.c $target $patch_new_files_dir
fi
