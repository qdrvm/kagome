/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONTENT_IDENTIFIER_HPP
#define KAGOME_CONTENT_IDENTIFIER_HPP

#include <vector>

#include "libp2p/multi/multibase_codec.hpp"
#include "libp2p/multi/multihash.hpp"
#include "libp2p/multi/uvarint.hpp"

namespace libp2p::multi {

  /**
   *
   *
   * @note multibase may be omitted in non text-based protocols and is generally
   * needed only for CIDs serialized to a string, so it is not present in this
   * structure
   */
  class ContentIdentifier {
   public:
    using MulticodecCode = UVarint;
    using MultibaseCode = MultibaseCodec::Encoding;

    enum class Version { V0, V1 };

    ContentIdentifier(Version version,
                      MulticodecCode content_type,
                      Multihash content_address);

    std::string toPrettyString(std::string_view base);

    bool operator==(const ContentIdentifier &c) const;
    bool operator!=(const ContentIdentifier &c) const;

    Version version;
    MulticodecCode content_type;
    Multihash content_address;
  };

}  // namespace libp2p::multi

#endif  // KAGOME_CONTENT_IDENTIFIER_HPP
