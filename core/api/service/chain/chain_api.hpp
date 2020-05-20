/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_API_CHAIN_API_HPP
#define KAGOME_API_CHAIN_API_HPP

#include <boost/variant.hpp>
#include "common/buffer.hpp"
#include "outcome/outcome.hpp"
#include "primitives/common.hpp"

namespace kagome::api {

  class ChainApi {
   public:
    virtual ~ChainApi() = default;
    using BlockHash = kagome::primitives::BlockHash;
    using ValueType = boost::variant<uint32_t, std::string>;

    virtual outcome::result<BlockHash> getBlockHash() const = 0;

    virtual outcome::result<BlockHash> getBlockHash(uint32_t value) const = 0;

    virtual outcome::result<BlockHash> getBlockHash(
        std::string_view value) const = 0;

    virtual outcome::result<std::vector<BlockHash>> getBlockHash(
        gsl::span<const ValueType> values) const = 0;
  };

}  // namespace kagome::api

#endif  // KAGOME_API_CHAIN_API_HPP
