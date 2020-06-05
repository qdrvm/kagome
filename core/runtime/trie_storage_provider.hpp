/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_TRIE_STORAGE_PROVIDER
#define KAGOME_CORE_RUNTIME_TRIE_STORAGE_PROVIDER

#include "outcome/outcome.hpp"

namespace kagome::runtime {

  class StorageProvider {
   public:
    outcome::result setToEphemeral();
    outcome::result setToEphemeral();

   private:

  };

}

#endif  // KAGOME_CORE_RUNTIME_TRIE_STORAGE_PROVIDER
