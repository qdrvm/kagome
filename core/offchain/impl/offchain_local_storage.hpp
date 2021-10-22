/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_OFFCHAIN_OFFCHAINLOCALSTORAGEIMPL
#define KAGOME_OFFCHAIN_OFFCHAINLOCALSTORAGEIMPL

#include "offchain/impl/offchain_storage_impl.hpp"
#include "offchain/offchain_local_storage.hpp"

namespace kagome::offchain {

  class OffchainLocalStorageImpl final : public OffchainStorageImpl,
                                         public OffchainLocalStorage {
   public:
    using OffchainStorageImpl::OffchainStorageImpl;
  };

}  // namespace kagome::offchain

#endif  // KAGOME_OFFCHAIN_OFFCHAINLOCALSTORAGEIMPL
