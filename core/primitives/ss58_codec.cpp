/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/ss58_codec.hpp"

#include "crypto/hasher.hpp"
#include "primitives/base58_codec.hpp"

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

  static common::Buffer calculateChecksum(
      gsl::span<uint8_t, kSs58Length - kSs58ChecksumLength> ss58_address,
      const crypto::Hasher &hasher) {
    constexpr auto PREFIX = "SS58PRE";
    auto preimage = common::Buffer{}.put(PREFIX).put(ss58_address);
    auto checksum = hasher.blake2b_256(preimage);
    return common::Buffer{
        gsl::make_span(checksum).subspan(0, kSs58ChecksumLength)};
  }

  outcome::result<AccountId> decodeSs58(std::string_view account_address,
                                        const crypto::Hasher &hasher) noexcept {
    // decode SS58 address: base58(<address-type><address><checksum>)
    // https://github.com/paritytech/substrate/wiki/External-Address-Format-(SS58)
    OUTCOME_TRY(ss58_account_id, primitives::decodeBase58(account_address));
    primitives::AccountId account_id;

    // first byte in SS58 is account type, then 32 bytes of the actual account
    // id, then 2 bytes of checksum
    if (ss58_account_id.size() != kSs58Length) {
      return Ss58Error::INVALID_LENGTH;
    }

    auto ss58_no_checksum = gsl::make_span(
        ss58_account_id.data(), ss58_account_id.size() - kSs58ChecksumLength);
    auto checksum = gsl::make_span<const uint8_t>(
        ss58_account_id.data() + ss58_no_checksum.size(), kSs58ChecksumLength);
    auto calculated_checksum = calculateChecksum(ss58_no_checksum, hasher);
    if (gsl::span<const uint8_t>(calculated_checksum)
        != checksum) {
      return Ss58Error::INVALID_CHECKSUM;
    }

    std::copy_n(ss58_account_id.begin() + 1,
                primitives::AccountId::size(),
                account_id.begin());

    return account_id;
  }

  std::string encodeSs58(uint8_t account_type,
                         const AccountId &id,
                         const crypto::Hasher &hasher) noexcept {
    common::Buffer ss58_bytes;
    ss58_bytes.reserve(kSs58Length);
    ss58_bytes.putUint8(account_type).put(id);
    auto checksum = calculateChecksum(ss58_bytes, hasher);
    ss58_bytes.put(checksum);

    return encodeBase58(ss58_bytes);
  }

}  // namespace kagome::primitives
