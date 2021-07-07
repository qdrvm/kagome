/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/binaryen/core_api_factory.hpp"

#include "runtime/binaryen/runtime_api/core_impl.hpp"
#include "runtime/common/constant_code_provider.hpp"

namespace kagome::runtime::binaryen {

  BinaryenCoreApiFactory::BinaryenCoreApiFactory(
      std::shared_ptr<storage::changes_trie::ChangesTracker> changes_tracker,
      std::shared_ptr<blockchain::BlockHeaderRepository> header_repo)
      : changes_tracker_{std::move(changes_tracker)},
        header_repo_{std::move(header_repo)} {
    BOOST_ASSERT(changes_tracker_ != nullptr);
    BOOST_ASSERT(header_repo_ != nullptr);
  }

  std::unique_ptr<Core> BinaryenCoreApiFactory::make(
      std::shared_ptr<const crypto::Hasher> hasher,
      gsl::span<const uint8_t> runtime_code) const {
    return std::make_unique<CoreImpl>(
        runtime_env_factory_,
        std::make_shared<ConstantCodeProvider>(common::Buffer{runtime_code}),
        changes_tracker_,
        header_repo_);
  }

}  // namespace kagome::runtime::binaryen
