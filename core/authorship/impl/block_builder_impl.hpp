/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef KAGOME_CORE_AUTHORSHIP_IMPL_BLOCK_BUILDER_IMPL_HPP
#define KAGOME_CORE_AUTHORSHIP_IMPL_BLOCK_BUILDER_IMPL_HPP

#include "authorship/block_builder.hpp"

#include "common/logger.hpp"
#include "primitives/block_id.hpp"
#include "runtime/block_builder.hpp"
#include "runtime/core.hpp"

namespace kagome::authorship {

  class BlockBuilderImpl : public BlockBuilder {
   public:
    ~BlockBuilderImpl() override = default;

    BlockBuilderImpl(primitives::BlockHeader block_header,
                     std::shared_ptr<runtime::BlockBuilder> r_block_builder);

    outcome::result<void> pushExtrinsic(
        const primitives::Extrinsic &extrinsic) override;

    outcome::result<primitives::Block> bake() const override;

   private:
    primitives::BlockHeader block_header_;
    std::shared_ptr<runtime::BlockBuilder> r_block_builder_;
    common::Logger logger_;

    std::vector<primitives::Extrinsic> extrinsics_{};
  };

}  // namespace kagome::authorship

#endif  // KAGOME_CORE_AUTHORSHIP_IMPL_BLOCK_BUILDER_IMPL_HPP
