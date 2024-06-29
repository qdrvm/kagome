/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "primitives/version.hpp"

namespace kagome::runtime {
  // https://github.com/paritytech/polkadot-sdk/blob/929a273ae1ba647628c4ba6e2f8737e58b596d6a/substrate/client/executor/src/wasm_runtime.rs#L355
  outcome::result<std::optional<primitives::Version>> readEmbeddedVersion(
      BufferView wasm);
}  // namespace kagome::runtime
