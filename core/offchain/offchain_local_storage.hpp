/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_OFFCHAIN_OFFCHAINLOCALSTORAGE
#define KAGOME_OFFCHAIN_OFFCHAINLOCALSTORAGE

#include "offchain/offchain_storage.hpp"

namespace kagome::offchain {

  class OffchainLocalStorage : public virtual OffchainStorage {};

}  // namespace kagome::offchain

#endif  // KAGOME_OFFCHAIN_OFFCHAINLOCALSTORAGE
