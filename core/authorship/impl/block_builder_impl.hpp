/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_AUTHORSHIP_IMPL_BLOCK_BUILDER_IMPL_HPP
#define KAGOME_CORE_AUTHORSHIP_IMPL_BLOCK_BUILDER_IMPL_HPP

#include "authorship/block_builder.hpp"

#include "log/logger.hpp"
#include "primitives/block_id.hpp"
#include "primitives/event_types.hpp"
#include "runtime/runtime_api/block_builder.hpp"
#include "runtime/runtime_api/core.hpp"

namespace kagome::authorship {

  class BlockBuilderImpl : public BlockBuilder {
   public:
    ~BlockBuilderImpl() override = default;

    BlockBuilderImpl(primitives::BlockHeader block_header,
                     const storage::trie::RootHash &storage_state,
                     std::shared_ptr<runtime::BlockBuilder> block_builder_api);

    outcome::result<std::vector<primitives::Extrinsic>> getInherentExtrinsics(
        const primitives::InherentData &data) const override;

    outcome::result<primitives::ExtrinsicIndex> pushExtrinsic(
        const primitives::Extrinsic &extrinsic) override;

    outcome::result<primitives::Block> bake() const override;

    size_t estimateBlockSize() const override;

   private:
    size_t estimatedBlockHeaderSize() const;

    primitives::BlockHeader block_header_;
    std::shared_ptr<runtime::BlockBuilder> block_builder_api_;
    storage::trie::RootHash storage_state_;
    log::Logger logger_;

    std::vector<primitives::Extrinsic> extrinsics_{};
  };

}  // namespace kagome::authorship

#endif  // KAGOME_CORE_AUTHORSHIP_IMPL_BLOCK_BUILDER_IMPL_HPP
