/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_PRIMITIVES_RPC_METHODS_HPP
#define KAGOME_CORE_PRIMITIVES_RPC_METHODS_HPP

#include <string>
#include <vector>

#include "scale/tie.hpp"

namespace kagome::primitives {

  // The version number is just hardcoded in substrate implementation
  // https://github.com/paritytech/substrate/blob/8060a437dc01cc247a757fb318a46f81c8e40d5c/client/rpc-servers/src/lib.rs#L59
  static constexpr uint32_t kRpcMethodsVersion = 1;

  /**
   * A descriptor for a set of methods supported via RPC
   */
  struct RpcMethods {
    SCALE_TIE(2);

    using Version = uint32_t;
    using Methods = std::vector<std::string>;

    /// Descriptor version
    Version version = kRpcMethodsVersion;

    /// A set of methods names as strings
    Methods methods;
  };
}  // namespace kagome::primitives

#endif  // KAGOME_CORE_PRIMITIVES_RPC_METHODS_HPP
