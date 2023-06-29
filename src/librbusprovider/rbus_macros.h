// If not stated otherwise in this file or this component's license file the
// following copyright and licenses apply:
//
// Copyright 2022 Consult Red
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef _DEVICESM_RBUS_MACROS_H
#define _DEVICESM_RBUS_MACROS_H


//if the c++ standard version of attributes are supported then use them, otherwise use the gcc attributes
#if defined (__GNUC__) && __GNUC__ >= 7
	#define UNUSED_CHECK [[maybe_unused]]
	#define UNUSED_RESULT_CHECK [[nodiscard]]
#else
	#define UNUSED_CHECK  __attribute__((unused))
	#define UNUSED_RESULT_CHECK __attribute__((warn_unused_result))
#endif

#endif