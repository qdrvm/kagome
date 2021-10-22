/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_OFFCHAIN_OFFCHAINPERSISTENTSTORAGEIMPL
#define KAGOME_OFFCHAIN_OFFCHAINPERSISTENTSTORAGEIMPL

#include "offchain/impl/offchain_storage_impl.hpp"
#include "offchain/offchain_persistent_storage.hpp"

namespace kagome::offchain {

  class OffchainPersistentStorageImpl final : public OffchainPersistentStorage,
                                              public OffchainStorageImpl {
   public:
    using OffchainStorageImpl::OffchainStorageImpl;
  };

}  // namespace kagome::offchain

#endif  // KAGOME_OFFCHAIN_OFFCHAINPERSISTENTSTORAGEIMPL
