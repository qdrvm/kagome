#pragma once

#include "runtime/runtime_context.hpp"

namespace kagome::parachain {
  static constexpr uint64_t DEFAULT_BACKING_EXECUTION_TIMEOUT = 2000;
  static constexpr uint64_t DEFAULT_APPROVAL_EXECUTION_TIMEOUT = 12000;

  struct RuntimeParams {
    kagome::runtime::RuntimeContext::ContextParams context_params;
    uint64_t pvf_exec_timeout_approval_ms = DEFAULT_APPROVAL_EXECUTION_TIMEOUT;
    uint64_t pvf_exec_timeout_backing_ms = DEFAULT_BACKING_EXECUTION_TIMEOUT;
  };

}  // namespace kagome::parachain

SCALE_TIE_HASH_STD(kagome::parachain::RuntimeParams);
