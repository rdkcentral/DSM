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

SUMMARY = "Downloadable Software Modules"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=50e2d278b66b3b7b20bc165c146a2b58"

SRC_URI = "git://git@gitlab.consult.red/dave.chapman/rdk-smart-router.git;protocol=ssh;branch=main"
SRC_URI += "file://0001-static-cast.patch"
SRC_URI += "file://dsm.config"

S = "${WORKDIR}/git"

EXTRA_OECMAKE =  " -DENABLE_RBUS_PROVIDER=ON"

SRCREV = "${AUTOREV}"
inherit pkgconfig cmake

DEPENDS += "rbus"

RDEPENDS_${PN} += "rbus"
RDEPENDS_${PN} += "rtmessage"
RDEPENDS_${PN} += "rbus-core"

OECMAKE_CXX_FLAGS += "-I${STAGING_INCDIR}/rbus"

do_install_append() {
    install -D -m 644 ${WORKDIR}/dsm.config ${D}/root/home/dsm.config
    ln -s ${D}/home/root/command.socket ${D}/command.socket
}