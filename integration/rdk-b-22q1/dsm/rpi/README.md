*Add DSM and Build a Bootable Raspberry Pi (Model 3 B++) Image and Flash to an SD Card 

** Obtain the RDKB source code (ie for 4th quarter of 2021)

   mkdir <workspace dir>
   cd <workspace dir>
 
   # initialize the repo tool
   repo init -u https://code.rdkcentral.com/r/rdkcmf/manifests -m rdkb-extsrc.xml -b rdkb-2021q4-dunfell
  
   # obtain the source code using the "repo" tool 
   repo sync -j `nproc` --no-clone-bundle --no-tags
 
   # configure to build a Raspberry Pi image
   MACHINE=raspberrypi-rdk-broadband source meta-cmf-raspberrypi/setup-environment

** Patch/overlay to RDKB workspace files to configure DSM in the build

   cd <workspace dir>

   # obtain the patches/new files and DSM installation script
   git clone git@github.com:rdkcentral/DSM.git

   # overlay/add  files to configure a DSM build for Raspberry Pi 
   ./DSM/integration/rdk-b-22q1/dsm/rpi/scripts/apply_patches_overlays.sh 

** Build the Raspberry Pi Image

   cd <workspace dir>
	 
   # build the Raspberry Pi image (NB: this will use as many CPUs as possible)
   MACHINE=raspberrypi-rdk-broadband source meta-cmf-raspberrypi/setup-environment
   bitbake rdk-generic-broadband-image

** Decompress the "bz2" de-ployable image file created by the build step.

   NB: The correct image file is in a compressed format and is named  <workspace dir>/build-raspberrypi-rdk-broadband/tmp/deploy/images/raspberrypi-rdk-broadband/rdk-generic-broadband-image-raspberrypi-rdk-broadband.wic.bz2

   cd <workspace dir>

   # copy the bz2 image file somewhere
   cp -p ./build-raspberrypi-rdk-broadband/tmp/deploy/images/raspberrypi-rdk-broadband/rdk-generic-broadband-image-raspberrypi-rdk-broadband.wic.bz2 ../images/.

   # decompress the bz2 image file into and uncompressed form with file extension .wic
   bzip2 -d ../images/rdk-generic-broadband-image-raspberrypi-rdk-broadband.wic.bz2

   # notice that the bz2 compressed file has been removed and replaces by a file with a wic image file.
   ls -l ../images/rdk-generic-broadband-image-raspberrypi-rdk-broadband.wic

** Flash the ".wic" image file which was decompressed from the ".bz2" file to an SD card using command "DD"
   cd <workspace dir>

   # flash the image file
   sudo dd if=../images/rdk-generic-broadband-image-raspberrypi-rdk-broadband.wic of=/dev/sdb1 bs=4M status=progress && sync

* RdKCentral Documentation

   DSM on RDK-B 2022q1 : RPi Reference Integration
     https://wiki.rdkcentral.com/display/ASP/DSM+on+RDK-B+2022q1+%3A+RPi+Reference+Integration 

   Testing on the Raspberry Pi Model 3 B++ Target (DSM) 
     https://wiki.rdkcentral.com/pages/viewpage.action?pageId=213586819 

   USP-PA on RDK-B 2022q1 : RPi Reference Integration
     https://wiki.rdkcentral.com/display/ASP/USP-PA+on+RDK-B+2022q1+%3A+RPi+Reference+Integration 
