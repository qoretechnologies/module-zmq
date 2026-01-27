if(NOT DEFINED libzmq_src_dir)
    message(FATAL_ERROR "libzmq_src_dir is not set")
endif()

set(libzmq_cmake "${libzmq_src_dir}/CMakeLists.txt")
if(NOT EXISTS "${libzmq_cmake}")
    message(FATAL_ERROR "libzmq CMakeLists.txt not found at ${libzmq_cmake}")
endif()

file(READ "${libzmq_cmake}" content)
string(REPLACE "cmake_minimum_required(VERSION 3.0.2)" "cmake_minimum_required(VERSION 3.5)" content "${content}")
string(REPLACE "cmake_minimum_required(VERSION 2.8.12)" "cmake_minimum_required(VERSION 3.5)" content "${content}")
file(WRITE "${libzmq_cmake}" "${content}")
