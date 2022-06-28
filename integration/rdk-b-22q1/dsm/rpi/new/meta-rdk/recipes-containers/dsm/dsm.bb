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
LIC_FILES_CHKSUM = "file://LICENSE;md5=c466d4ab8a68655eb1edf0bf8c1a8fb8"

S = "${WORKDIR}/git"
SRC_URI= "git://git@github.com:rdkcentral/DSM.git;protocol=ssh;branch=main"

SRCREV = "c459281ab4b0a89fc79dc6334a2e1152e40c18c7"
inherit pkgconfig cmake