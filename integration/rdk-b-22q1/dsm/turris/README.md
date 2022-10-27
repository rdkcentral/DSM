*Add DSM and Build a Bootable Turris Omnia Image and Flash to an SD Card

** Obtain the RDKB source code

   mkdir <workspace dir>
   cd <workspace dir>
 
   # initialize the repo tool
   repo init -u https://code.rdkcentral.com/r/rdkcmf/manifests -m rdkb-turris-extsrc.xml -b dunfell
  
   # obtain the source code using the "repo" tool 
   repo sync -j `nproc` --no-clone-bundle
 
   # delete the zilker-sdk dependency
   --- a/recipes-core/packagegroups/packagegroup-rdk-oss-broadband.bbappend
   +++ b/recipes-core/packagegroups/packagegroup-rdk-oss-broadband.bbappend
   @@ -1,2 +1 @@
    RDEPENDS_packagegroup-rdk-oss-broadband_remove = "alljoyn"
   -RDEPENDS_packagegroup-rdk-oss-broadband_append_dunfell = " zilker-sdk"

   # configure
   MACHINE=turris source meta-turris/setup-environment

** Patch/overlay to RDKB workspace files to configure DSM in the build

   cd <workspace dir>

   # obtain the patches/new files and DSM installation script
   git clone git@github.com:rdkcentral/DSM.git

   # overlay/add  files to configure a DSM build
   ./DSM/integration/rdk-b-22q1/dsm/turris/scripts/apply_patches_overlays.sh

** Build Turris image

   cd <workspace dir>
	 
   # configure and build
   MACHINE=turris source meta-turris/setup-environment
   bitbake rdk-generic-broadband-image

** Flash the ".wic" image file
   https://wiki.rdkcentral.com/pages/viewpage.action?pageId=114986683

* RdKCentral Documentation

   DSM on RDK-B 2022q1 : RPi Reference Integration
     https://wiki.rdkcentral.com/display/ASP/DSM+on+RDK-B+2022q1+%3A+RPi+Reference+Integration

