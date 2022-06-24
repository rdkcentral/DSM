SUMMARY = "Downloadable Software Modules"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=c466d4ab8a68655eb1edf0bf8c1a8fb8"

S = "${WORKDIR}/git/dsm"
SRC_URI= "git://git@gitlab.consult.red:/dave.chapman/rdk-smart-router.git;protocol=ssh;branch=sprint-002"

SRCREV = "c42806ea1734768ab6e3ec8e1e6a68e86de02255"
inherit pkgconfig cmake
