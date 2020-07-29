//  Copyright (c) 2020 ETH Zurich
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file
/// Contains HPX executors that forward to a Kokkos backend.

#pragma once

#include <hpx/kokkos/deep_copy.hpp>
#include <hpx/kokkos/detail/logging.hpp>
#include <hpx/kokkos/kokkos_algorithms.hpp>

#include <hpx/algorithm.hpp>
#include <hpx/numeric.hpp>

#include <Kokkos_Core.hpp>

#include <type_traits>

namespace hpx {
namespace kokkos {
namespace detail {
template <std::size_t... Is, typename F, typename A, typename Tuple>
HPX_HOST_DEVICE void invoke_helper(hpx::util::index_pack<Is...>, F &&f, A &&a,
                                   Tuple &&t) {
  hpx::util::invoke_r<void>(std::forward<F>(f), std::forward<A>(a),
                            hpx::util::get<Is>(std::forward<Tuple>(t))...);
}
} // namespace detail

/// \brief HPX executor wrapping a Kokkos execution space.
template <typename ExecutionSpace = Kokkos::DefaultExecutionSpace>
class executor {
public:
  using execution_space = ExecutionSpace;
  using execution_category = hpx::parallel::execution::parallel_execution_tag;

  explicit executor(execution_space const &instance = {}) : inst(instance) {}

  execution_space instance() const { return inst; }

  template <typename F, typename... Ts> void post(F &&f, Ts &&... ts) {
    auto ts_pack = hpx::util::make_tuple(std::forward<Ts>(ts)...);
    parallel_for_async(
        Kokkos::Experimental::require(
            Kokkos::RangePolicy<execution_space>(inst, 0, 1),
            Kokkos::Experimental::WorkItemProperty::HintLightWeight),
        KOKKOS_LAMBDA(int) { hpx::util::invoke_fused_r<void>(f, ts_pack); });
  }

  template <typename F, typename... Ts>
  hpx::shared_future<void> async_execute(F &&f, Ts &&... ts) {
    auto ts_pack = hpx::util::make_tuple(std::forward<Ts>(ts)...);
    return parallel_for_async(
        Kokkos::Experimental::require(
            Kokkos::RangePolicy<execution_space>(inst, 0, 1),
            Kokkos::Experimental::WorkItemProperty::HintLightWeight),
        KOKKOS_LAMBDA(int) { hpx::util::invoke_fused_r<void>(f, ts_pack); });
  }

  template <typename F, typename S, typename... Ts>
  std::vector<hpx::future<void>> bulk_async_execute(F &&f, S const &s,
                                                    Ts &&... ts) {
    HPX_KOKKOS_DETAIL_LOG("bulk_async_execute");
    auto ts_pack = hpx::util::make_tuple(std::forward<Ts>(ts)...);
    auto size = hpx::util::size(s);
    auto b = hpx::util::begin(s);

    auto fut = parallel_for_async(
        Kokkos::Experimental::require(
            Kokkos::RangePolicy<ExecutionSpace>(inst, 0, size),
            Kokkos::Experimental::WorkItemProperty::HintLightWeight),
        KOKKOS_LAMBDA(int i) {
          HPX_KOKKOS_DETAIL_LOG("bulk_async_execute i = %d", i);
          using index_pack_type =
              typename hpx::util::detail::fused_index_pack<decltype(
                  ts_pack)>::type;
          detail::invoke_helper(index_pack_type{}, f, *(b + i), ts_pack);
        });

    std::vector<hpx::future<void>> result;
    // TODO: The empty continuation is only used to get a future from a shared
    // future.
    result.push_back(fut.then(hpx::launch::sync, [](auto &&) {}));
    return result;
  }

  template <typename Parameters, typename F>
  constexpr std::size_t get_chunk_size(Parameters &&params, F &&f,
                                       std::size_t cores,
                                       std::size_t count) const {
    return std::size_t(-1);
  }

private:
  execution_space inst{};
};

// Define type aliases
using default_executor = executor<Kokkos::DefaultExecutionSpace>;
using default_host_executor = executor<Kokkos::DefaultHostExecutionSpace>;

#if defined(KOKKOS_ENABLE_CUDA)
using cuda_executor = executor<Kokkos::Cuda>;
#endif

#if defined(KOKKOS_ENABLE_HIP)
using hip_executor = executor<Kokkos::Experimental::HIP>;
#endif

#if defined(KOKKOS_ENABLE_HPX)
using hpx_executor = executor<Kokkos::Experimental::HPX>;
#endif

#if defined(KOKKOS_ENABLE_OPENMP)
using openmp_executor = executor<Kokkos::OpenMP>;
#endif

#if defined(KOKKOS_ENABLE_ROCM)
using rocm_executor = executor<Kokkos::ROCm>;
#endif

#if defined(KOKKOS_ENABLE_SERIAL)
using serial_executor = executor<Kokkos::Serial>;
#endif

template <typename Executor> struct is_kokkos_executor : std::false_type {};

template <typename ExecutionSpace>
struct is_kokkos_executor<executor<ExecutionSpace>> : std::true_type {};

template <typename ExecutionSpace = Kokkos::DefaultExecutionSpace>
hpx::kokkos::executor<typename std::decay<ExecutionSpace>::type>
make_executor() {
  return {hpx::kokkos::make_execution_space<
      typename std::decay<ExecutionSpace>::type>};
}
} // namespace kokkos
} // namespace hpx

namespace hpx {
namespace parallel {
namespace execution {
template <typename ExecutionSpace>
struct is_one_way_executor<hpx::kokkos::executor<ExecutionSpace>>
    : std::true_type {};

template <typename ExecutionSpace>
struct is_two_way_executor<hpx::kokkos::executor<ExecutionSpace>>
    : std::true_type {};

template <typename ExecutionSpace>
struct is_bulk_two_way_executor<hpx::kokkos::executor<ExecutionSpace>>
    : std::true_type {};
} // namespace execution
} // namespace parallel
} // namespace hpx
