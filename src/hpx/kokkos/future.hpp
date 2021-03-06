///////////////////////////////////////////////////////////////////////////////
//  Copyright (c) 2020 ETH Zurich
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
///////////////////////////////////////////////////////////////////////////////

/// \file Contains utilities for dealing with Kokkos execution spaces and HPX
/// futures.

#pragma once

#include <hpx/kokkos/detail/logging.hpp>

#include <hpx/config.hpp>
#include <hpx/future.hpp>

#if defined(HPX_HAVE_COMPUTE)
#include <hpx/modules/async_cuda.hpp>
#endif

#include <Kokkos_Core.hpp>

namespace hpx {
namespace kokkos {
namespace detail {
template <typename ExecutionSpace = Kokkos::DefaultExecutionSpace>
struct get_future {
  template <typename E> static hpx::shared_future<void> call(E &&inst) {
    // The best we can do generically at the moment is to fence on the
    // instance and return a ready future. It would be nice to be able to
    // attach a callback to any execution space instance to trigger future
    // completion.
    inst.fence();
    HPX_KOKKOS_DETAIL_LOG("getting generic ready future after fencing");
    return hpx::make_ready_future();
  }
};

#if defined(KOKKOS_ENABLE_CUDA)
#if !defined(HPX_KOKKOS_CUDA_FUTURE_TYPE)
#define HPX_KOKKOS_CUDA_FUTURE_TYPE callback
#endif
template <> struct get_future<Kokkos::Cuda> {
  template <typename E> static hpx::shared_future<void> call(E &&inst) {
    HPX_KOKKOS_DETAIL_LOG("getting future from stream %p", inst.cuda_stream());
#if HPX_KOKKOS_CUDA_FUTURE_TYPE == 0
    return hpx::cuda::experimental::detail::get_future_with_event(
        inst.cuda_stream());
#elif HPX_KOKKOS_CUDA_FUTURE_TYPE == 1
    return hpx::cuda::experimental::detail::get_future_with_callback(
        inst.cuda_stream());
#else
#error "HPX_KOKKOS_CUDA_FUTURE_TYPE is invalid (must be callback or event)"
#endif
  }
};
#endif

#if defined(KOKKOS_ENABLE_HIP)
#if !defined(HPX_KOKKOS_CUDA_FUTURE_TYPE)
#define HPX_KOKKOS_CUDA_FUTURE_TYPE callback
#endif
template <> struct get_future<Kokkos::Experimental::HIP> {
  template <typename E> static hpx::shared_future<void> call(E &&inst) {
    HPX_KOKKOS_DETAIL_LOG("getting future from stream %p", inst.hip_stream());
#if HPX_KOKKOS_CUDA_FUTURE_TYPE == 0
    return hpx::cuda::experimental::detail::get_future_with_event(
        inst.hip_stream());
#elif HPX_KOKKOS_CUDA_FUTURE_TYPE == 1
    return hpx::cuda::experimental::detail::get_future_with_callback(
        inst.hip_stream());
#else
#error "HPX_KOKKOS_CUDA_FUTURE_TYPE is invalid (must be callback or event)"
#endif
  }
};
#endif

#if defined(KOKKOS_ENABLE_HPX) && KOKKOS_VERSION >= 30000 &&                   \
    defined(KOKKOS_ENABLE_HPX_ASYNC_DISPATCH)
template <> struct get_future<Kokkos::Experimental::HPX> {
  template <typename E> static hpx::shared_future<void> call(E &&inst) {
    HPX_KOKKOS_DETAIL_LOG("getting future from HPX instance %x",
                          inst.impl_instance_id());
    return inst.impl_get_future();
  }
};
#endif
} // namespace detail

/// Make a future for a particular execution space instance. This might be
/// useful for functions that don't have *_async overloads yet but take an
/// execution space instance for asynchronous execution.
template <typename ExecutionSpace>
hpx::shared_future<void> get_future(ExecutionSpace &&inst) {
  return detail::get_future<typename std::decay<ExecutionSpace>::type>::call(
      std::forward<ExecutionSpace>(inst));
}

/// Make a future for the default instance of an execution space. This might be
/// useful for functions that don't have *_async overloads yet but take an
/// execution space instance for asynchronous execution.
template <typename ExecutionSpace = Kokkos::DefaultExecutionSpace>
hpx::shared_future<void> get_future() {
  return detail::get_future<ExecutionSpace>::call(ExecutionSpace());
}
} // namespace kokkos
} // namespace hpx
