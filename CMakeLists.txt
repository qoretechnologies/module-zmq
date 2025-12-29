cmake_minimum_required(VERSION 2.8.12)

project(qore-zmq-module)

set (VERSION_MAJOR 1)
set (VERSION_MINOR 1)
set (VERSION_PATCH 0)

set(PROJECT_VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

find_package(Qore 0.9 REQUIRED)
find_package(ZMQ REQUIRED)

include_directories(${CZMQ_INCLUDE_DIRS})
include_directories(${ZMQ_INCLUDE_DIRS})

# Check for C++11.
include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
if(COMPILER_SUPPORTS_CXX11)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
else()
    message(FATAL_ERROR "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
endif()

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "release")
endif (NOT CMAKE_BUILD_TYPE)
string(TOLOWER ${CMAKE_BUILD_TYPE} QORE_BUILD_TYPE_LWR)
if (${QORE_BUILD_TYPE_LWR} MATCHES "debug")
    add_definitions(-DDEBUG)
else ()
    add_definitions(-DNDEBUG)
endif ()

set(QPP_SRC
    src/QC_ZContext.qpp
    src/QC_ZSocket.qpp
    src/QC_ZSocketPub.qpp
    src/QC_ZSocketSub.qpp
    src/QC_ZSocketReq.qpp
    src/QC_ZSocketRep.qpp
    src/QC_ZSocketDealer.qpp
    src/QC_ZSocketRouter.qpp
    src/QC_ZSocketPush.qpp
    src/QC_ZSocketPull.qpp
    src/QC_ZSocketXPub.qpp
    src/QC_ZSocketXSub.qpp
    src/QC_ZSocketPair.qpp
    src/QC_ZSocketStream.qpp
    src/QC_ZFrame.qpp
    src/QC_ZMsg.qpp
    src/qc_zmq.qpp
    src/ql_zmq.qpp
)

if (QORE_BUILD_ZMQ_DRAFT)
    set(QPP_SRC ${QPP_SRC}
        src/QC_ZSocketServer.qpp
        src/QC_ZSocketClient.qpp
        src/QC_ZSocketRadio.qpp
        src/QC_ZSocketDish.qpp
    )
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DQORE_BUILD_ZMQ_DRAFT=1")
endif (QORE_BUILD_ZMQ_DRAFT)

set(CPP_SRC
    src/zmq-module.cpp
    src/QoreZSock.cpp
)

qore_wrap_qpp_value(QPP_SOURCES ${QPP_SRC})

foreach (it ${QPP_SOURCES})
    GET_FILENAME_COMPONENT(_outfile ${it} NAME_WE)
    set(QPP_DOX ${QPP_DOX} ${CMAKE_CURRENT_BINARY_DIR}/${_outfile}.dox.h)
endforeach()

set(module_name "zmq")

set(QORE_DOX_TMPL_SRC
  docs/mainpage.dox.tmpl
)

add_library(${module_name} MODULE ${CPP_SRC} ${QPP_SOURCES})
include_directories(${ZMQ_INCLUDE_DIRS})
include_directories(${CMAKE_SOURCE_DIR}/src)
target_include_directories(${module_name} PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>)

target_link_libraries(${module_name} ${ZMQ_LIBRARIES} ${CZMQ_LIBRARIES})

set(MODULE_DOX_INPUT ${CMAKE_CURRENT_BINARY_DIR}/mainpage.dox ${QPP_DOX})
string(REPLACE ";" " " MODULE_DOX_INPUT "${MODULE_DOX_INPUT}")

if (DEFINED ENV{DOXYGEN_EXECUTABLE})
    set(DOXYGEN_EXECUTABLE $ENV{DOXYGEN_EXECUTABLE})
endif()

qore_external_binary_module(${module_name} ${PROJECT_VERSION})

qore_dist(${PROJECT_VERSION})

qore_config_info()

if (DOXYGEN_FOUND)
  qore_wrap_dox(QORE_DOX_SRC ${QORE_DOX_TMPL_SRC})
  add_custom_target(QORE_MOD_DOX_FILES DEPENDS ${QORE_DOX_SRC})
  add_dependencies(docs-module QORE_MOD_DOX_FILES)
endif()
