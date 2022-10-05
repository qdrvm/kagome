/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "parachain/availability/bitfield/store_impl.hpp"

namespace kagome::parachain {
  void BitfieldStoreImpl::putBitfield(const BlockHash &relay_parent,
                                      const SignedBitfield &bitfield) {
    bitfields_[relay_parent].push_back(bitfield);
  }

  std::vector<BitfieldStore::SignedBitfield> BitfieldStoreImpl::getBitfields(
      const BlockHash &relay_parent) const {
    std::vector<SignedBitfield> bitfields;
    auto it = bitfields_.find(relay_parent);
    if (it != bitfields_.end()) {
      bitfields = it->second;
    }
    return bitfields;
  }
}  // namespace kagome::parachain
