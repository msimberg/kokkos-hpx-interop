# Copyright (c) 2019-2020 ETH Zurich
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

cmake_minimum_required(VERSION 3.13)

project(HPXKokkos CXX)

# Dependencies
find_package(HPX 1.6.0 REQUIRED)
find_package(Kokkos 3.2.0 REQUIRED)

# Check that Kokkos and HPX options are consistent.
if(Kokkos_ENABLE_CUDA)
  kokkos_check(OPTIONS CUDA_LAMBDA)
  if(NOT HPX_WITH_CUDA)
    message(FATAL_ERROR "Kokkos was built with CUDA support, HPX was not")
  endif()
else()
  if(HPX_WITH_CUDA)
    message(FATAL_ERROR "HPX was built with CUDA support, Kokkos was not")
  endif()
endif()

kokkos_check(DEVICES HPX)
kokkos_check(OPTIONS HPX_ASYNC_DISPATCH)

# Main targets provided by this project
add_library(hpx_kokkos INTERFACE)
target_include_directories(hpx_kokkos
  INTERFACE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
  $<INSTALL_INTERFACE:include>)
target_link_libraries(hpx_kokkos INTERFACE HPX::hpx Kokkos::kokkos)

set(HPX_KOKKOS_CUDA_FUTURE_TYPE "event" CACHE STRING
  "Type of CUDA futures to use by default (\"event\" or \"callback\").")
if(HPX_KOKKOS_CUDA_FUTURE_TYPE STREQUAL "event")
  target_compile_definitions(hpx_kokkos INTERFACE "HPX_KOKKOS_CUDA_FUTURE_TYPE=0")
elseif(HPX_KOKKOS_CUDA_FUTURE_TYPE STREQUAL "callback")
  target_compile_definitions(hpx_kokkos INTERFACE "HPX_KOKKOS_CUDA_FUTURE_TYPE=1")
else()
  message(FATAL_ERROR "Invalid HPX_KOKKOS_CUDA_FUTURE_TYPE=\"${HPX_KOKKOS_CUDA_FUTURE_TYPE}\" (allowed values are \"event\" and \"callback\")")
endif()

include(GNUInstallDirs)
install(
  TARGETS hpx_kokkos
  EXPORT HPXKokkos)
install(
  EXPORT HPXKokkos
  FILE HPXKokkosTargets.cmake
  NAMESPACE HPXKokkos::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/HPXKokkos)
install(
  DIRECTORY src/
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  FILES_MATCHING PATTERN "*.hpp")
install(
  FILES cmake/HPXKokkosConfig.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/HPXKokkos)

# Handle tests
option(HPX_KOKKOS_ENABLE_TESTS "Enable tests." OFF)
if(HPX_KOKKOS_ENABLE_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

option(HPX_KOKKOS_ENABLE_BENCHMARKS "Enable benchmarks." OFF)
if(HPX_KOKKOS_ENABLE_BENCHMARKS)
  enable_testing()
  add_subdirectory(benchmarks)
endif()
