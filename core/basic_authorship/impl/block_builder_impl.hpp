/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_BASIC_AUTHORSHIP_IMPL_BLOCK_BUILDER_IMPL_HPP
#define KAGOME_CORE_BASIC_AUTHORSHIP_IMPL_BLOCK_BUILDER_IMPL_HPP

#include "basic_authorship/block_builder.hpp"

#include "primitives/block_id.hpp"
#include "runtime/block_builder_api.hpp"
#include "runtime/core.hpp"

namespace kagome::basic_authorship {

  class BlockBuilderImpl : public BlockBuilder {
   public:
    ~BlockBuilderImpl() override = default;

    BlockBuilderImpl(primitives::BlockHeader block_header,
                     std::shared_ptr<runtime::BlockBuilderApi> r_block_builder);

    outcome::result<bool> pushExtrinsic(
        const primitives::Extrinsic &extrinsic) override;

    [[nodiscard]] primitives::Block bake() const override;

   private:
    primitives::BlockHeader block_header_;
    std::shared_ptr<runtime::BlockBuilderApi> r_block_builder_;

    std::vector<primitives::Extrinsic> extrinsics_{};
  };

}  // namespace kagome::basic_authorship

#endif  // KAGOME_CORE_BASIC_AUTHORSHIP_IMPL_BLOCK_BUILDER_IMPL_HPP
