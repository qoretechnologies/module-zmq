# - Try to find ZMQ
# Once done this will define
# ZMQ_FOUND - System has ZMQ
# ZMQ_INCLUDE_DIR - The ZMQ include directories
# ZMQ_INCLUDE_DIRS - The ZMQ include directories
# ZMQ_LIBRARY - The libraries needed to use ZMQ
# ZMQ_LIBRARIES - The libraries needed to use ZMQ
# ZMQ_DEFINITIONS - Compiler switches required for using ZMQ

find_path(ZMQ_INCLUDE_DIR zmq.h HINTS $ENV{ZMQ_DIR}/include)
if (ZMQ_INCLUDE_DIR)
    message(STATUS "Found ZMQ includes: ${ZMQ_INCLUDE_DIR}")
else()
    message(FATAL_ERROR "ZMQ includes not found (set ZMQ_DIR to the ZMQ installation location and try again)")
endif(ZMQ_INCLUDE_DIR)

find_library(ZMQ_LIBRARY NAMES zmq HINTS $ENV{ZMQ_DIR}/lib)

set(ZMQ_INCLUDE_DIRS ${ZMQ_INCLUDE_DIR})
set(ZMQ_LIBRARIES ${ZMQ_LIBRARY})

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set ZMQ_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(ZMQ DEFAULT_MSG ZMQ_LIBRARY ZMQ_INCLUDE_DIR)

# - Try to find CZMQ
# Once done this will define
# CZMQ_FOUND - System has CZMQ
# CZMQ_INCLUDE_DIR - The CZMQ include directories
# CZMQ_INCLUDE_DIRS - The CZMQ include directories
# CZMQ_LIBRARY - The libraries needed to use CZMQ
# CZMQ_LIBRARIES - The libraries needed to use CZMQ
# CZMQ_DEFINITIONS - Compiler switches required for using CZMQ

include(CheckFunctionExists)
include(CheckCXXSourceCompiles)

find_path(CZMQ_INCLUDE_DIR czmq.h HINTS $ENV{ZMQ_DIR}/include)
if (CZMQ_INCLUDE_DIR)
  message(STATUS "Found CZMQ includes: ${CZMQ_INCLUDE_DIR}")
else()
  message(FATAL_ERROR "CZMQ includes not found (set CZMQ_DIR to the CZMQ installation location and try again)")
endif(CZMQ_INCLUDE_DIR)

find_library(CZMQ_LIBRARY NAMES czmq HINTS $ENV{ZMQ_DIR}/lib)

set(CZMQ_INCLUDE_DIRS ${CZMQ_INCLUDE_DIR})
set(CZMQ_LIBRARIES ${CZMQ_LIBRARY})

set(CMAKE_REQUIRED_INCLUDES ${CZMQ_INCLUDE_DIR} ${ZMQ_INCLUDE_DIRS})
set(CMAKE_REQUIRED_LIBRARIES ${CZMQ_LIBRARY} ${ZMQ_LIBRARIES})

check_function_exists(zframe_meta HAVE_ZFRAME_META)
if(HAVE_ZFRAME_META)
    add_definitions(-DHAVE_ZFRAME_META)
endif(HAVE_ZFRAME_META)

check_function_exists(zmq_proxy_steerable HAVE_ZMQ_PROXY_STEERABLE)
if(HAVE_ZMQ_PROXY_STEERABLE)
    add_definitions(-DHAVE_ZMQ_PROXY_STEERABLE)
endif(HAVE_ZMQ_PROXY_STEERABLE)

# check signature of zmsg_encode()
check_cxx_source_compiles("
#include <zmq.h>
#include <czmq.h>
#include <zmsg.h>
#include <zframe.h>
int main() {
    zframe_t* f = zmsg_encode((zmsg_t*)0);
}
" HAVE_ZMSG_ENCODE_TO_ZFRAME)

if(HAVE_ZMSG_ENCODE_TO_ZFRAME)
    add_definitions(-DHAVE_ZMSG_ENCODE_TO_ZFRAME)
else(HAVE_ZMSG_ENCODE_TO_ZFRAME)
    check_cxx_source_compiles("
#include <czmq.h>
#include <zmsg.h>
int main() {
    byte* p;
    size_t i = zmsg_encode((zmsg_t*)0, &p);
}
" HAVE_ZMSG_ENCODE_TO_BUFFER)

    if(HAVE_ZMSG_ENCODE_TO_BUFFER)
        add_definitions(-DHAVE_ZMSG_ENCODE_TO_BUFFER)
    else(HAVE_ZMSG_ENCODE_TO_BUFFER)
        message(FATAL_ERROR "The found CZMQ library does not support zmsg_encode() function")
    endif(HAVE_ZMSG_ENCODE_TO_BUFFER)
endif(HAVE_ZMSG_ENCODE_TO_ZFRAME)

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set CZMQ_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(CZMQ DEFAULT_MSG CZMQ_LIBRARY CZMQ_INCLUDE_DIR)
