/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/multi/multiaddress.hpp"
#include "libp2p/multi/multiaddress/c-utils/protoutils.h"

namespace libp2p::multi {

  Multiaddress::FactoryResult Multiaddress::createMultiaddress(
      std::string_view address) {
    std::vector<uint8_t> bytes(address.size());
    auto *bytes_ptr = bytes.data();
    auto bytes_size = bytes.size();

    // convert string address to bytes and make sure they represent valid
    // address
    auto conversion_error = string_to_bytes(
        &bytes_ptr, &bytes_size, address.data(), address.size());
    if (conversion_error != nullptr) {
      return kagome::expected::Error{"Could not create a multiaddress object: "
                                     + std::string{conversion_error}};
    }

    return kagome::expected::Value{
        Multiaddress{std::string{address}, ByteBuffer{bytes}}};
  }

  Multiaddress::FactoryResult Multiaddress::createMultiaddress(
      const ByteBuffer &bytes) {
    std::string address(bytes.size(), '0');
    auto *address_ptr = address.data();

    // convert bytes address to string and make sure it represents valid address
    auto conversion_error =
        bytes_to_string(&address_ptr, bytes.to_bytes(), bytes.size());
    if (conversion_error != nullptr) {
      return kagome::expected::Error{"Could not create a multiaddress object: "
                                     + std::string{conversion_error}};
    }

    return kagome::expected::Value{
        Multiaddress{std::move(address), ByteBuffer{bytes}}};
  }

  Multiaddress::Multiaddress(std::string &&address, ByteBuffer &&bytes)
      : stringified_address_{std::move(address)}, bytes_{std::move(bytes)} {}

}  // namespace libp2p::multi
