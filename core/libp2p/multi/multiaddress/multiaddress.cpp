/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <numeric>
#include <stdexcept>

#include "libp2p/multi/multiaddress.hpp"
#include "libp2p/multi/multiaddress/c-utils/protoutils.h"

namespace {
  /**
   * Find beginning of at least one of the provided substrings in the string
   * @param string to search in
   * @param substrings to be searched for
   * @return position of one of the substrings inside of the string or npos, if
   * none of them is found
   */
  size_t findOneOfSubstrings(
      std::string_view string,
      std::initializer_list<std::string_view> substrings) {
    auto pos = std::string_view::npos;
    std::any_of(std::begin(substrings),
                std::end(substrings),
                [&pos, &string](auto substring) {
                  pos = string.find(substring);
                  return pos != std::string_view::npos;
                });
    return pos;
  }
}  // namespace

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

    try {
      Multiaddress res{std::string{address},
                       std::make_shared<ByteBuffer>(std::move(bytes))};
      return kagome::expected::Value{
          std::make_unique<Multiaddress>(std::move(res))};
    } catch (const std::invalid_argument &ex) {
      return kagome::expected::Error{"Could not create a multiaddress object: "
                                     + std::string{ex.what()}};
    }
  }

  Multiaddress::FactoryResult Multiaddress::createMultiaddress(
      std::shared_ptr<ByteBuffer> bytes) {
    std::string address(bytes->size(), '0');
    auto *address_ptr = address.data();

    // convert bytes address to string and make sure it represents valid address
    auto conversion_error =
        bytes_to_string(&address_ptr, bytes->to_bytes(), bytes->size());
    if (conversion_error != nullptr) {
      return kagome::expected::Error{"Could not create a multiaddress object: "
                                     + std::string{conversion_error}};
    }

    try {
      Multiaddress res{std::string{address}, std::move(bytes)};
      return kagome::expected::Value{
          std::make_unique<Multiaddress>(std::move(res))};
    } catch (const std::invalid_argument &ex) {
      return kagome::expected::Error{"Could not create a multiaddress object: "
                                     + std::string{ex.what()}};
    }
  }

  Multiaddress::Multiaddress(std::string &&address,
                             std::shared_ptr<ByteBuffer> bytes)
      : stringified_address_{std::move(address)}, bytes_{std::move(bytes)} {
    // set family of this multiaddress based on the bytes
    switch (bytes_->operator[](0)) {
      case static_cast<uint8_t>(Family::kIp4):
        family_ = Family::kIp4;
        break;
      case static_cast<uint8_t>(Family::kIp6):
        family_ = Family::kIp6;
        break;
      default:
        throw std::invalid_argument(
            "family of the provided address is either unsupported or invalid: "
            + stringified_address_);
    }
    calculatePort();
    calculatePeerId();
  }

  bool Multiaddress::encapsulate(std::string_view part);

  bool Multiaddress::decapsulate(std::string_view part);

  Multiaddress::Family Multiaddress::getFamily() const {
    return family_;
  }

  bool Multiaddress::isIpAddress() const {
    return getFamily() == Family::kIp4 || getFamily() == Family::kIp6;
  }

  std::string_view Multiaddress::getStringAddress() const {
    return stringified_address_;
  }

  boost::optional<std::string> Multiaddress::getStringAddress(
      Family address_family) const {
    // currently, only IP addresses are supported
    if (!isIpAddress()
        || (address_family != Family::kIp4 && address_family != Family::kIp6)) {
      return boost::none;
    }

    auto family_beginning =
        findOneOfSubstrings(stringified_address_, {"/ip4/", "/ip6/"});
    if (family_beginning == std::string_view::npos) {
      // maybe, the address got such decapsulated that is does not contain
      // family prefix
      return boost::none;
    }

    auto address_beginning = family_beginning + 5;
    if (address_beginning == std::string::npos) {
      // address does not have anything but the family prefix
      return boost::none;
    }

    return stringified_address_.substr(address_beginning);
  }

  boost::optional<uint16_t> Multiaddress::getPort() const {
    return port_;
  }

  boost::optional<std::string> Multiaddress::getPeerId() const {
    return peer_id_;
  }

  void Multiaddress::calculatePort() {
    size_t proto_beginning =
        findOneOfSubstrings(stringified_address_, {"/tcp/", "/udp/"});
    if (proto_beginning == std::string_view::npos) {
      return;
    }

    auto port_beginning = proto_beginning + 5;
    if (port_beginning == std::string::npos) {
      return;
    }

    // convert the tail of address to size_t port
    std::istringstream iss{stringified_address_.substr(port_beginning)};
    size_t port;
    iss >> port;
    if (iss.fail()) {
      return;
    }

    port_ = port;
  }

  void Multiaddress::calculatePeerId() {
    size_t ipfs_beginning =
        findOneOfSubstrings(stringified_address_, {"/ipfs/"});
    if (ipfs_beginning == std::string_view::npos) {
      return;
    }

    auto id_beginning = ipfs_beginning + 6;
    if (id_beginning == std::string::npos) {
      return;
    }

    peer_id_ = stringified_address_.substr(id_beginning);
  }

}  // namespace libp2p::multi
