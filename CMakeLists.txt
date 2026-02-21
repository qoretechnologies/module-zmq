cmake_minimum_required(VERSION 3.14)

# Set macOS deployment target
if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")
endif()

project(qore-zmq-module)

set (VERSION_MAJOR 1)
set (VERSION_MINOR 2)
set (VERSION_PATCH 0)

set(PROJECT_VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

# Option to control zmq/czmq source
# AUTO (default): Try system libraries with draft API, fall back to building from source
# ON: Force use of system libraries (fails if draft API not available)
# OFF: Force building from source
set(USE_SYSTEM_ZMQ "AUTO" CACHE STRING "Use system zmq/czmq: AUTO, ON, or OFF")
set_property(CACHE USE_SYSTEM_ZMQ PROPERTY STRINGS "AUTO" "ON" "OFF")

find_package(Qore 0.9 REQUIRED)

# Check for C++11
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

# Determine whether to use system libraries or build from source
set(USE_SYSTEM_ZMQ_RESOLVED OFF)

if (USE_SYSTEM_ZMQ STREQUAL "ON" OR USE_SYSTEM_ZMQ STREQUAL "AUTO")
    # Try to find system zmq/czmq
    find_package(ZMQ QUIET)

    if (ZMQ_FOUND AND CZMQ_FOUND)
        message(STATUS "Found system ZMQ: ${ZMQ_LIBRARY}")
        message(STATUS "Found system CZMQ: ${CZMQ_LIBRARY}")

        # Check for ZMQ draft API support by testing if zmq_socket(ZMQ_SERVER) works at link time
        # We test actual function calls, not just header macros
        include(CheckCSourceCompiles)
        set(CMAKE_REQUIRED_INCLUDES ${ZMQ_INCLUDE_DIRS})
        set(CMAKE_REQUIRED_LIBRARIES ${ZMQ_LIBRARIES})
        set(CMAKE_REQUIRED_DEFINITIONS -DZMQ_BUILD_DRAFT_API=1)
        check_c_source_compiles("
        #include <zmq.h>
        int main() {
            void* ctx = zmq_ctx_new();
            // ZMQ_SERVER (value 13) is a draft socket type
            // This will link but fail at runtime if draft not compiled in
            void* s = zmq_socket(ctx, 13);
            if (s) zmq_close(s);
            zmq_ctx_term(ctx);
            return 0;
        }
        " ZMQ_COMPILES_WITH_DRAFT)

        # Also try to run the test to verify draft API actually works
        # We need to actually USE the socket, not just create it, because
        # zmq_socket() may return a socket that will crash when used
        if (ZMQ_COMPILES_WITH_DRAFT)
            include(CheckCSourceRuns)
            check_c_source_runs("
            #include <zmq.h>
            #include <string.h>
            int main() {
                void* ctx = zmq_ctx_new();
                if (!ctx) return 1;
                // ZMQ_SERVER = 13 (draft socket type)
                void* server = zmq_socket(ctx, 13);
                if (!server) {
                    zmq_ctx_term(ctx);
                    return 1;  // Draft API not available at runtime
                }
                // Set receive timeout to prevent hanging (2 seconds)
                int timeout = 2000;
                zmq_setsockopt(server, ZMQ_RCVTIMEO, &timeout, sizeof(timeout));
                // Try to bind - this will crash if draft socket types aren't properly supported
                int rc = zmq_bind(server, \"tcp://127.0.0.1:*\");
                if (rc != 0) {
                    zmq_close(server);
                    zmq_ctx_term(ctx);
                    return 1;
                }
                // Create a CLIENT socket and try to connect
                void* client = zmq_socket(ctx, 14);  // ZMQ_CLIENT = 14
                if (!client) {
                    zmq_close(server);
                    zmq_ctx_term(ctx);
                    return 1;
                }
                // Set send timeout to prevent hanging
                zmq_setsockopt(client, ZMQ_SNDTIMEO, &timeout, sizeof(timeout));
                // Get the bound endpoint
                char endpoint[256];
                size_t endpoint_len = sizeof(endpoint);
                rc = zmq_getsockopt(server, ZMQ_LAST_ENDPOINT, endpoint, &endpoint_len);
                if (rc != 0) {
                    zmq_close(client);
                    zmq_close(server);
                    zmq_ctx_term(ctx);
                    return 1;
                }
                rc = zmq_connect(client, endpoint);
                if (rc != 0) {
                    zmq_close(client);
                    zmq_close(server);
                    zmq_ctx_term(ctx);
                    return 1;
                }
                // Send a test message
                const char* test_msg = \"test\";
                rc = zmq_send(client, test_msg, strlen(test_msg), 0);
                if (rc != (int)strlen(test_msg)) {
                    zmq_close(client);
                    zmq_close(server);
                    zmq_ctx_term(ctx);
                    return 1;
                }
                // Receive it (will timeout after 2 seconds if no message arrives)
                char buf[256];
                rc = zmq_recv(server, buf, sizeof(buf), 0);
                if (rc != (int)strlen(test_msg)) {
                    zmq_close(client);
                    zmq_close(server);
                    zmq_ctx_term(ctx);
                    return 1;
                }
                zmq_close(client);
                zmq_close(server);
                zmq_ctx_term(ctx);
                return 0;
            }
            " ZMQ_DRAFT_API_WORKS)
        endif()

        if (ZMQ_DRAFT_API_WORKS)
            message(STATUS "System ZMQ has draft API support")
            set(USE_SYSTEM_ZMQ_RESOLVED ON)
        else()
            if (USE_SYSTEM_ZMQ STREQUAL "ON")
                message(FATAL_ERROR "System ZMQ does not have draft API support. "
                    "Please install libzmq 4.2+ built with draft API enabled "
                    "(--enable-drafts or -DENABLE_DRAFTS=ON), or use -DUSE_SYSTEM_ZMQ=AUTO or OFF.")
            else()
                message(STATUS "System ZMQ does not have draft API support, will build from source")
            endif()
        endif()
    else()
        if (USE_SYSTEM_ZMQ STREQUAL "ON")
            message(FATAL_ERROR "System ZMQ/CZMQ not found. Please install libzmq and czmq, "
                "or use -DUSE_SYSTEM_ZMQ=AUTO or OFF to build from source.")
        else()
            message(STATUS "System ZMQ/CZMQ not found, will build from source")
        endif()
    endif()
endif()

if (USE_SYSTEM_ZMQ_RESOLVED)
    # Use system-installed zmq/czmq with verified draft API support
    include_directories(${CZMQ_INCLUDE_DIRS})
    include_directories(${ZMQ_INCLUDE_DIRS})

    # Check for CZMQ draft API support
    set(CMAKE_REQUIRED_INCLUDES ${CZMQ_INCLUDE_DIRS})
    set(CMAKE_REQUIRED_LIBRARIES ${CZMQ_LIBRARIES} ${ZMQ_LIBRARIES})
    set(CMAKE_REQUIRED_DEFINITIONS -DZMQ_BUILD_DRAFT_API=1)
    include(CheckCXXSymbolExists)
    check_cxx_symbol_exists(zmsg_set_group "czmq.h" HAVE_CZMQ_DRAFT_API)
    if (HAVE_CZMQ_DRAFT_API)
        add_definitions(-DHAVE_CZMQ_DRAFT_API=1)
    endif()

    # Enable draft API in compilation
    add_definitions(-DZMQ_BUILD_DRAFT_API=1)

    set(ZMQ_LINK_LIBRARIES ${ZMQ_LIBRARIES} ${CZMQ_LIBRARIES})
else()
    # Build libzmq and czmq from source with draft API enabled
    include(FetchContent)

    message(STATUS "Building libzmq and czmq from source with draft API enabled")

    # Fetch libzmq
    FetchContent_Declare(
        libzmq
        GIT_REPOSITORY https://github.com/zeromq/libzmq.git
        GIT_TAG        v4.3.5
        GIT_SHALLOW    TRUE
        PATCH_COMMAND  ${CMAKE_COMMAND} -Dlibzmq_src_dir=<SOURCE_DIR>
                       -P ${CMAKE_SOURCE_DIR}/cmake/patch-libzmq-cmake-min.cmake
    )

    # Configure libzmq build options
    set(ENABLE_DRAFTS ON CACHE BOOL "Enable draft APIs" FORCE)
    set(ENABLE_CURVE ON CACHE BOOL "Enable CURVE security" FORCE)
    set(WITH_LIBSODIUM ON CACHE BOOL "Use libsodium for CURVE" FORCE)
    set(BUILD_TESTS OFF CACHE BOOL "Disable libzmq tests" FORCE)
    set(BUILD_SHARED OFF CACHE BOOL "Build static library" FORCE)
    set(BUILD_STATIC ON CACHE BOOL "Build static library" FORCE)
    set(WITH_DOCS OFF CACHE BOOL "Disable docs" FORCE)
    set(WITH_PERF_TOOL OFF CACHE BOOL "Disable perf tool" FORCE)

    FetchContent_MakeAvailable(libzmq)

    # Fetch czmq - using master for macOS compatibility
    FetchContent_Declare(
        czmq
        GIT_REPOSITORY https://github.com/zeromq/czmq.git
        GIT_TAG        master
    )

    # Configure czmq build options
    set(CZMQ_BUILD_SHARED OFF CACHE BOOL "Build shared library" FORCE)
    set(CZMQ_BUILD_STATIC ON CACHE BOOL "Build static library" FORCE)
    set(ENABLE_DRAFTS ON CACHE BOOL "Enable draft APIs" FORCE)
    set(CZMQ_WITH_DOCS OFF CACHE BOOL "Disable docs" FORCE)

    # On macOS, prevent czmq from finding Kernel.framework's uuid.h
    # by disabling UUID support (macOS has built-in UUID via CFUUIDCreate)
    if(APPLE)
        set(CZMQ_WITH_UUID OFF CACHE BOOL "Disable uuid dependency" FORCE)
        set(uuid_FOUND FALSE CACHE BOOL "uuid not found" FORCE)
    endif()

    # Ensure ZMQ draft API is enabled for czmq compilation
    add_compile_definitions(ZMQ_BUILD_DRAFT_API=1)

    # On Linux, enable GNU extensions for pthread_setname_np
    if(NOT APPLE)
        add_compile_definitions(_GNU_SOURCE)
    endif()

    # Tell czmq to use our FetchContent-built libzmq instead of finding the system one
    set(libzmq_FOUND TRUE)
    set(libzmq_INCLUDE_DIRS "${libzmq_SOURCE_DIR}/include;${libzmq_BINARY_DIR}")
    set(libzmq_LIBRARIES libzmq-static)

    # Use FetchContent_Populate + add_subdirectory with EXCLUDE_FROM_ALL
    # This prevents czmq from installing its executables which we don't build
    FetchContent_GetProperties(czmq)
    if(NOT czmq_POPULATED)
        FetchContent_Populate(czmq)
        add_subdirectory(${czmq_SOURCE_DIR} ${czmq_BINARY_DIR} EXCLUDE_FROM_ALL)
    endif()

    # Set include directories for fetched libraries
    include_directories(${libzmq_SOURCE_DIR}/include)
    include_directories(${czmq_SOURCE_DIR}/include)
    include_directories(${libzmq_BINARY_DIR})
    include_directories(${czmq_BINARY_DIR})

    # CZMQ draft API is always available when building from source
    add_definitions(-DHAVE_CZMQ_DRAFT_API=1)

    # Modern czmq uses zframe-based encode/decode API
    add_definitions(-DHAVE_ZMSG_ENCODE_TO_ZFRAME=1)

    # Check if zframe_meta is available (might not be in all czmq versions)
    # Note: We check header declaration since linking against static libs during cmake check is complex
    include(CheckCXXSourceCompiles)
    set(CMAKE_REQUIRED_INCLUDES ${czmq_SOURCE_DIR}/include ${libzmq_SOURCE_DIR}/include ${libzmq_BINARY_DIR})
    set(CMAKE_REQUIRED_DEFINITIONS -DZMQ_BUILD_DRAFT_API=1)
    check_cxx_source_compiles("
    #include <czmq.h>
    // Check that zframe_meta is declared in the header
    typedef const char* (*zframe_meta_func_t)(zframe_t*, const char*);
    int main() {
        zframe_meta_func_t fn = &zframe_meta;
        (void)fn;
        return 0;
    }
    " HAVE_ZFRAME_META_FUNC)
    if (HAVE_ZFRAME_META_FUNC)
        add_definitions(-DHAVE_ZFRAME_META=1)
    endif()

    set(ZMQ_LINK_LIBRARIES czmq-static libzmq-static)

    message(STATUS "Building ZMQ from source with draft API enabled")
endif()

# Enable draft API definitions
add_definitions(-DQORE_BUILD_ZMQ_DRAFT=1)

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
    src/QC_ZSocketServer.qpp
    src/QC_ZSocketClient.qpp
    src/QC_ZSocketRadio.qpp
    src/QC_ZSocketDish.qpp
    src/QC_ZFrame.qpp
    src/QC_ZMsg.qpp
    src/qc_zmq.qpp
    src/ql_zmq.qpp
)

set(CPP_SRC
    src/zmq-module.cpp
    src/QoreZSock.cpp
)

qore_wrap_qpp_value(QPP_SOURCES METALIST QPP_META_SRC ${QPP_SRC})

foreach (it ${QPP_SOURCES})
    GET_FILENAME_COMPONENT(_outfile ${it} NAME_WE)
    set(QPP_DOX ${QPP_DOX} ${CMAKE_CURRENT_BINARY_DIR}/${_outfile}.dox.h)
endforeach()

set(module_name "zmq")

set(QORE_DOX_TMPL_SRC
  docs/mainpage.dox.tmpl
)

add_library(${module_name} MODULE ${CPP_SRC} ${QPP_SOURCES})

# Include src directory first for any local headers
include_directories(BEFORE ${CMAKE_SOURCE_DIR}/src)
target_include_directories(${module_name} PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>)

target_link_libraries(${module_name} ${ZMQ_LINK_LIBRARIES})

set(MODULE_DOX_INPUT ${CMAKE_CURRENT_BINARY_DIR}/mainpage.dox ${QPP_DOX})
string(REPLACE ";" " " MODULE_DOX_INPUT "${MODULE_DOX_INPUT}")

if (DEFINED ENV{DOXYGEN_EXECUTABLE})
    set(DOXYGEN_EXECUTABLE $ENV{DOXYGEN_EXECUTABLE})
endif()

qore_external_binary_module(${module_name} ${PROJECT_VERSION})

qore_dist(${PROJECT_VERSION})

# install qpp metadata files
if (COMMAND qore_install_qpp_metadata)
    qore_install_qpp_metadata(${module_name} ${QPP_META_SRC})
else()
    install(FILES ${QPP_META_SRC}
            DESTINATION ${CMAKE_INSTALL_DATADIR}/qore/metadata)
endif()

qore_config_info()

if (DOXYGEN_FOUND)
  qore_wrap_dox(QORE_DOX_SRC ${QORE_DOX_TMPL_SRC})
  add_custom_target(QORE_MOD_DOX_FILES DEPENDS ${QORE_DOX_SRC})
  add_dependencies(docs-module QORE_MOD_DOX_FILES)
endif()
