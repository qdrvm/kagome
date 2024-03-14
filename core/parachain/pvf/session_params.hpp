/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "runtime/runtime_api/parachain_host.hpp"
#include "runtime/runtime_context.hpp"

namespace kagome::parachain {
  inline outcome::result<runtime::RuntimeContext::ContextParams> sessionParams(
      runtime::ParachainHost &api, const primitives::BlockHash &relay_parent) {
    OUTCOME_TRY(session_index, api.session_index_for_child(relay_parent));
    OUTCOME_TRY(session_params,
                api.session_executor_params(relay_parent, session_index));
    runtime::RuntimeContext::ContextParams config;
    config.memory_limits.max_stack_values_num =
        runtime::RuntimeContext::DEFAULT_STACK_MAX;
    if (session_params) {
      for (auto &param : *session_params) {
        if (auto *stack_max = get_if<runtime::StackLogicalMax>(&param)) {
          config.memory_limits.max_stack_values_num = stack_max->max_values_num;
        } else if (auto *pages_max = get_if<runtime::MaxMemoryPages>(&param)) {
          config.memory_limits.max_memory_pages_num = pages_max->limit;
        }
      }
    }
    return config;
  }
}  // namespace kagome::parachain
