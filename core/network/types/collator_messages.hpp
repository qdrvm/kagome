/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_COLLATOR_DECLARE_HPP
#define KAGOME_COLLATOR_DECLARE_HPP

#include <boost/variant.hpp>
#include <type_traits>
#include <vector>

#include "common/blob.hpp"
#include "consensus/grandpa/common.hpp"
#include "primitives/common.hpp"
#include "primitives/compact_integer.hpp"
#include "primitives/digest.hpp"
#include "storage/trie/types.hpp"

namespace kagome::network {
  /**
   * Possible collation message contents.
   * [X,Y] - possible values.
   * [0] --- 0 ---> [0, 1, 4] -+- 0 ---> struct
   * CollatorDeclaration(collator_pubkey, para_id, collator_signature)
   *                           |- 1 ---> struct CollatorAdvertisement(para_hash)
   *                           |- 4 ---> struct CollatorSeconded(relay_hash,
   * CollatorStatement)
   */

  /**
   * Collator -> Validator message.
   * Declaration of the intent to advertise a collation.
   */
  struct CollatorDeclaration {
    /*
     * Public key of the collator.
     */
    consensus::grandpa::Id collator_pubkey;

    /*
     * Parachain Id.
     */
    uint32_t para_id;

    /*
     * Signature of the collator using the PeerId of the collators node.
     */
    consensus::grandpa::Signature collator_signature;
  };

  /**
   * Collator -> Validator message.
   * Advertisement of a collation.
   */
  struct CollatorAdvertisement {
    /*
     * Hash of the parachain block.
     */
    primitives::BlockHash para_hash;
  };

  using CollationMessage =
      boost::variant<CollatorDeclaration,
                     CollatorAdvertisement,
                     Unused<2>,
                     Unused<3>,
                     Unused<4>>;  // 4 will be replaced to Statement
  using CollationProtocolData = boost::variant<CollationMessage>;

  struct CollationProtocolMessage {
    CollationProtocolData data;
  };

  inline bool operator==(const CollationProtocolMessage &lhs,
                         const CollationProtocolMessage &rhs) {
    return lhs.data == rhs.data;
  }
  inline bool operator!=(const CollationProtocolMessage &lhs,
                         const CollationProtocolMessage &rhs) {
    return !(lhs == rhs);
  }

  /**
   * @brief compares two CollatorDeclaration instances
   */
  inline bool operator==(const CollatorDeclaration &lhs,
                         const CollatorDeclaration &rhs) {
    return lhs.collator_pubkey == rhs.collator_pubkey
           && lhs.para_id == rhs.para_id
           && lhs.collator_signature == rhs.collator_signature;
  }
  inline bool operator!=(const CollatorDeclaration &lhs,
                         const CollatorDeclaration &rhs) {
    return !(lhs == rhs);
  }

  /**
   * @brief compares two CollatorAdvertisement instances
   */
  inline bool operator==(const CollatorAdvertisement &lhs,
                         const CollatorAdvertisement &rhs) {
    return lhs.para_hash == rhs.para_hash;
  }
  inline bool operator!=(const CollatorAdvertisement &lhs,
                         const CollatorAdvertisement &rhs) {
    return !(lhs == rhs);
  }

  template <class Stream,
            typename = std::enable_if_t<Stream::is_encoder_stream>>
  Stream &operator<<(Stream &s, const CollationProtocolMessage &v) {
    return s << v.data;
  }
  template <class Stream,
            typename = std::enable_if_t<Stream::is_decoder_stream>>
  Stream &operator>>(Stream &s, CollationProtocolMessage &v) {
    return s >> v.data;
  }

  /**
   * @brief serialization of CollatorDeclaration into stream
   */
  template <class Stream,
            typename = std::enable_if_t<Stream::is_encoder_stream>>
  Stream &operator<<(Stream &s, const CollatorDeclaration &v) {
    return s << v.collator_pubkey << v.para_id << v.collator_signature;
  }
  template <class Stream,
            typename = std::enable_if_t<Stream::is_decoder_stream>>
  Stream &operator>>(Stream &s, CollatorDeclaration &v) {
    return s >> v.collator_pubkey >> v.para_id >> v.collator_signature;
  }

  /**
   * @brief serialization of CollatorDeclaration into stream
   */
  template <class Stream,
            typename = std::enable_if_t<Stream::is_encoder_stream>>
  Stream &operator<<(Stream &s, const CollatorAdvertisement &v) {
    return s << v.para_hash;
  }
  template <class Stream,
            typename = std::enable_if_t<Stream::is_decoder_stream>>
  Stream &operator>>(Stream &s, CollatorAdvertisement &v) {
    return s >> v.para_hash;
  }
}  // namespace kagome::network

#endif  // KAGOME_COLLATOR_DECLARE_HPP
