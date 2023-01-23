SRC_URI = "git://git@gitlab.consult.red/dave.chapman/rdk-smart-router.git;protocol=ssh;branch=main \
           file://0001-static-cast.patch"
SRCREV = "${AUTOREV}"
S = "${WORKDIR}/git/internal/develop/librbusprovider"
inherit pkgconfig cmake

SUMMARY = "RBUS datamodel provider lib"
LICENSE += ""
LIC_FILES_CHKSUM += "file://CMakeLists.txt;beginline=1;endline=18;md5=018a818c805c6b2ae3a3bb7e78bcf292" 

OECMAKE_CXX_FLAGS += "-I${STAGING_INCDIR}/rbus"
 
COMPONENT = "librbusprovider"

DEPENDS += "rbus"

RDEPENDS_${PN} += "rbus"
RDEPENDS_${PN} += "rtmessage"
RDEPENDS_${PN} += "rbus-core"

FILES_${PN} += "/usr/lib/librbusprovider.so" 