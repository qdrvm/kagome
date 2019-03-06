/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_MULTIBASE_IMPL_HPP
#define KAGOME_MULTIBASE_IMPL_HPP

#include "libp2p/multi/multibase.hpp"

namespace libp2p::multi {
  /**
   * Implementation of the Multibase with fair codecs
   */
  class MultibaseImpl : public Multibase {
   public:
    std::string encode(const kagome::common::Buffer &bytes,
                       Encoding encoding) const override;

    kagome::expected::Result<kagome::common::Buffer, std::string> decode(
        std::string_view string) const override;
  };
}  // namespace libp2p::multi

#endif  // KAGOME_MULTIBASE_IMPL_HPP
