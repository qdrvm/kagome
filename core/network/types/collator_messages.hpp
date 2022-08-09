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
#include "scale/tie.hpp"
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
    SCALE_TIE(3);
    SCALE_TIE_EQ(CollatorDeclaration);

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
    SCALE_TIE(1);
    SCALE_TIE_EQ(CollatorAdvertisement);

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
    SCALE_TIE(1);
    SCALE_TIE_EQ(CollationProtocolMessage);

    CollationProtocolData data;
  };
}  // namespace kagome::network

#endif  // KAGOME_COLLATOR_DECLARE_HPP
