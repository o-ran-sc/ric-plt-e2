# Copyright 2019 AT&T Intellectual Property
# Copyright 2019 Nokia
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cmake_minimum_required(VERSION 3.13)
project(e2)

set(CMAKE_VERBOSE_MAKEFILE on)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DDEV_PKG=1")

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DASN_DISABLE_OER_SUPPORT -DASN_PDU_COLLECTION -L. -LRIC-E2-TERMINATION/tracelibcpp/build -ggdb3 -Wall -W -Wpedantic")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DASN_DISABLE_OER_SUPPORT -DASN_PDU_COLLECTION -L. -O3 -L/usr/lib -Wall -Wpedantic")
#only c code with -O3
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DASN_DISABLE_OER_SUPPORT -DASN_PDU_COLLECTION -L. -O3 -L/usr/lib -Wall -W -Wpedantic")

include_directories(RIC-E2-TERMINATION
        RIC-E2-TERMINATION/asn1cFiles
        RIC-E2-TERMINATION/3rdparty)

file(GLOB ASN_MODULE_SRCS "RIC-E2-TERMINATION/asn1cFiles/*.c")
file(GLOB ASN_MODULE_HDRS "RIC-E2-TERMINATION/asn1cFiles/*.h")

add_library(asn1codec ${ASN_MODULE_SRCS} ${ASN_MODULE_HDRS})

install(TARGETS asn1codec DESTINATION /usr/lib)
install(FILES ${ASN_MODULE_HDRS} DESTINATION /usr/include/asn1c)


#include_directories(RIC-E2-TERMINATION/TEST/asn1c)
#
#file(GLOB ASNE2SMC "RIC-E2-TERMINATION/TEST/asn1c/*.c")
#file(GLOB ASNE2SMH "RIC-E2-TERMINATION/TEST/asn1c/*.h")
#RIC-E2-TERMINATION/
#add_library(asne2smcodec ${ASNE2SMC})


include_directories(RIC-E2-TERMINATION/TEST)
include_directories(RIC-E2-TERMINATION/TEST/e2smTest)
include_directories(RIC-E2-TERMINATION/TEST/T1)
include_directories(RIC-E2-TERMINATION/TEST/T2)
include_directories(RIC-E2-TERMINATION/base64/include)
include_directories(RIC-E2-TERMINATION/base64/lib)

add_library(e2sm
        RIC-E2-TERMINATION/TEST/e2sm.c
        RIC-E2-TERMINATION/TEST/e2sm.h
        ${ASN_MODULE_HDRS})

add_definitions(-DBOOST_LOG_DYN_LINK)

link_libraries(nsl
        sctp
        c
        m
        dl
        mdclog
        rmr_nng
        nng
        e2sm
        asn1codec
        boost_system
        boost_log_setup
        boost_log
        boost_date_time
        boost_thread
        boost_system
        rt
        boost_filesystem
        cgreen
        pthread)

add_executable(e2 RIC-E2-TERMINATION/sctpThread.cpp
        RIC-E2-TERMINATION/sctpThread.h
        RIC-E2-TERMINATION/openTracing.h
        RIC-E2-TERMINATION/mapWrapper.h
        RIC-E2-TERMINATION/base64.h
        RIC-E2-TERMINATION/base64.cpp
        RIC-E2-TERMINATION/ReadConfigFile.h
        )

add_executable(e2smtest
        RIC-E2-TERMINATION/TEST/e2smTest/e2smtest.cpp)

add_executable(testConfigFile
        RIC-E2-TERMINATION/ReadConfigFile.h
        RIC-E2-TERMINATION/TEST/ConfigurationFileTest/testConfigFile.cpp)
