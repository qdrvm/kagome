/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "runtime/runtime_api/parachain_host.hpp"
#include "runtime/runtime_context.hpp"

namespace kagome::parachain {
  inline outcome::result<runtime::RuntimeContext::RuntimeParams> sessionParams(
      runtime::ParachainHost &api, const primitives::BlockHash &relay_parent) {
    // https://github.com/paritytech/polkadot-sdk/blob/e0c081dbd46c1e6edca1ce2c62298f5f3622afdd/polkadot/node/core/pvf/common/src/executor_interface.rs#L46-L47
    constexpr uint32_t kDefaultHeapPagesEstimate = 32;
    constexpr uint32_t kExtraHeapPages = 2048;
    OUTCOME_TRY(session_index, api.session_index_for_child(relay_parent));
    OUTCOME_TRY(session_params,
                api.session_executor_params(relay_parent, session_index));
    runtime::RuntimeContext::RuntimeParams config;
    auto& contextParams = config.context_params;
    contextParams.memory_limits.max_stack_values_num =
        runtime::RuntimeContext::DEFAULT_STACK_MAX;
    contextParams.memory_limits.heap_alloc_strategy =
        HeapAllocStrategyDynamic{kDefaultHeapPagesEstimate + kExtraHeapPages};
    if (session_params) {
      for (auto &param : *session_params) {
        if (auto *stack_max = get_if<runtime::StackLogicalMax>(&param)) {
          contextParams.memory_limits.max_stack_values_num = stack_max->max_values_num;
        } else if (auto *pages_max = get_if<runtime::MaxMemoryPages>(&param)) {
          contextParams.memory_limits.heap_alloc_strategy = HeapAllocStrategyDynamic{
              kDefaultHeapPagesEstimate + pages_max->limit};
        } else if (auto *pvfExecTimeout = get_if<runtime::PvfExecTimeout>(&param)) {
          if (pvfExecTimeout->kind == runtime::PvfExecTimeoutKind::Backing) {
            config.pvf_exec_timeout_backing_ms = pvfExecTimeout->msec;
          }
        }
      }
    }
    return config;
  }
}  // namespace kagome::parachain
