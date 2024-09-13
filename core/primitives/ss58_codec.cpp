/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/ss58_codec.hpp"

#include <libp2p/multi/multibase_codec/codecs/base58.hpp>

#include "crypto/hasher.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::primitives, Ss58Error, e) {
  using E = kagome::primitives::Ss58Error;

  switch (e) {
    case E::INVALID_LENGTH:
      return "Invalid SS58 address length; Only 35 byte addresses are "
             "supported";
    case E::INVALID_CHECKSUM:
      return "Invalid SS58 checksum";
  }
  return "Unknown SS58 codec error";
}

namespace kagome::primitives {

  static common::Buffer calculateChecksum(common::BufferView ss58_address,
                                          const crypto::Hasher &hasher) {
    constexpr auto PREFIX = "SS58PRE";
    auto preimage = common::Buffer{}.put(PREFIX).put(ss58_address);
    auto checksum = hasher.blake2b_512(preimage);
    return {std::span(checksum).first<kSs58ChecksumLength>()};
  }

  outcome::result<AccountId> decodeSs58(std::string_view account_address,
                                        const crypto::Hasher &hasher) {
    // decode SS58 address: base58(<address-type><address><checksum>)
    // https://github.com/paritytech/substrate/wiki/External-Address-Format-(SS58)
    OUTCOME_TRY(ss58_account_id,
                libp2p::multi::detail::decodeBase58(account_address));

    auto ss58_no_checksum =
        std::span(ss58_account_id)
            .first(ss58_account_id.size() - kSs58ChecksumLength);
    auto checksum = std::span(ss58_account_id).last<kSs58ChecksumLength>();

    auto calculated_checksum = calculateChecksum(ss58_no_checksum, hasher);

    if (common::BufferView(calculated_checksum)
        != common::BufferView(checksum)) {
      return Ss58Error::INVALID_CHECKSUM;
    }

    size_t type_size = (ss58_account_id[0] < 64) ? 1 : 2;

    if (ss58_account_id.size() - kSs58ChecksumLength - type_size
        != AccountId::size()) {
      return Ss58Error::INVALID_LENGTH;
    }

    primitives::AccountId account_id;
    std::copy_n(ss58_account_id.begin() + type_size,
                primitives::AccountId::size(),
                account_id.begin());

    return account_id;
  }

  std::string encodeSs58(uint8_t account_type,
                         const AccountId &id,
                         const crypto::Hasher &hasher) {
    common::Buffer ss58_bytes;
    ss58_bytes.reserve(2 + id.size() + kSs58ChecksumLength);
    if (account_type < 64) {
      ss58_bytes.putUint8(account_type).put(id);
    } else {
      // https://docs.substrate.io/fundamentals/accounts-addresses-keys/
      uint8_t _0 = (account_type & 0b0000000011111100) >> 2 | 0b01000000;
      uint8_t _1 = account_type >> 8 | (account_type & 0b0000000000000011) << 6;
      ss58_bytes.putUint8(_0).putUint8(_1).put(id);
    }
    auto checksum = calculateChecksum(ss58_bytes, hasher);
    ss58_bytes.put(checksum);
    return libp2p::multi::detail::encodeBase58(ss58_bytes.asVector());
  }

}  // namespace kagome::primitives
