cmake_minimum_required(VERSION 2.6)

project(affinity-test)

set(Boost_USE_STATIC_LIBS ON)

find_package(Boost 1.65.1 COMPONENTS system filesystem REQUIRED)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wextra")

include_directories(..)
include_directories(../..)

add_executable(irq-affinity-test IrqAffinityTest.cpp)
add_executable(cpu-affinity-test CpuAffinityTest.cpp)

target_link_libraries(irq-affinity-test ${Boost_FILESYSTEM_LIBRARY} ${Boost_SYSTEM_LIBRARY})
target_link_libraries(cpu-affinity-test pthread)
