/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <algorithm>
#include <numeric>
#include <stdexcept>

#include "libp2p/multi/multiaddress.hpp"
extern "C" {
#include "libp2p/multi/multiaddress/c-utils/protoutils.h"
}

namespace {
  // string representations of protocols
  constexpr std::string_view kIp4 = "/ip4";
  constexpr std::string_view kIp6 = "/ip6";
  constexpr std::string_view kIpfs = "/ipfs";
  constexpr std::string_view kTcp = "/tcp";
  constexpr std::string_view kUdp = "/udp";
  constexpr std::string_view kDccp = "/dccp";
  constexpr std::string_view kSctp = "/sctp";
  constexpr std::string_view kUdt = "/udt";
  constexpr std::string_view kUtp = "/utp";
  constexpr std::string_view kHttp = "/http";
  constexpr std::string_view kHttps = "/https";
  constexpr std::string_view kWs = "/ws";
  constexpr std::string_view kOnion = "/onion";
  constexpr std::string_view kWebrtc = "/libp2p-webrtc-star";

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

  /**
   * Find all occurrences of the string in other string
   * @param string to search in
   * @param substring to be searched for
   * @return vector with positions of all occurrences of that substring
   */
  std::vector<size_t> findSubstringOccurrences(std::string_view string,
                                               std::string_view substring) {
    std::vector<size_t> occurrences;
    auto occurrence = string.find_first_of(substring);
    while (occurrence != std::string_view::npos) {
      occurrences.push_back(occurrence);
      occurrence = string.find(substring, occurrence + substring.size());
    }
    return occurrences;
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

    Multiaddress res{std::string{address},
                     std::make_shared<ByteBuffer>(std::move(bytes))};
    return kagome::expected::Value{
        std::make_unique<Multiaddress>(std::move(res))};
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

    Multiaddress res{std::string{address}, std::move(bytes)};
    return kagome::expected::Value{
        std::make_unique<Multiaddress>(std::move(res))};
  }

  Multiaddress::Multiaddress(std::string &&address,
                             std::shared_ptr<ByteBuffer> bytes)
      : stringified_address_{std::move(address)}, bytes_{std::move(bytes)} {
    calculatePeerId();
  }

  void Multiaddress::encapsulate(const Multiaddress &address) {
    stringified_address_ += address.stringified_address_;
    bytes_->put(address.bytes_->to_vector());

    calculatePeerId();
  }

  bool Multiaddress::decapsulate(const Multiaddress &address) {
    auto str_pos =
        stringified_address_.find_last_of(address.stringified_address_);
    if (str_pos == std::string::npos) {
      return false;
    }
    stringified_address_.erase(str_pos);

    const auto &this_bytes = bytes_->to_vector();
    const auto &other_bytes = address.bytes_->to_vector();
    auto bytes_pos = std::search(this_bytes.begin(),
                                 this_bytes.end(),
                                 other_bytes.begin(),
                                 other_bytes.end());
    bytes_ = std::make_shared<ByteBuffer>(
        std::vector<uint8_t>{this_bytes.begin(), bytes_pos - 1});

    calculatePeerId();
    return true;
  }

  std::string_view Multiaddress::getStringAddress() const {
    return stringified_address_;
  }

  const std::vector<uint8_t> &Multiaddress::getBytesAddress() const {
    return bytes_->to_vector();
  }

  boost::optional<std::string> Multiaddress::getPeerId() const {
    return peer_id_;
  }

  boost::optional<std::vector<std::string>> Multiaddress::getValuesForProtocol(
      Protocol proto) const {
    std::vector<std::string> values;

    auto proto_str = protocolToString(proto);
    auto proto_positions =
        findSubstringOccurrences(stringified_address_, proto_str);
    if (proto_positions.empty()) {
      return boost::none;
    }

    for (auto proto_pos : proto_positions) {
      auto value_pos = stringified_address_.find_first_of('/', proto_pos + 1);
      auto value_end = stringified_address_.find_first_of('/', value_pos + 1);
      values.push_back(stringified_address_.substr(value_pos, value_end));
    }

    return values;
  }

  void Multiaddress::calculatePeerId() {
    auto ipfs_beginning = findOneOfSubstrings(stringified_address_, {kIpfs});
    if (ipfs_beginning == std::string_view::npos) {
      return;
    }

    auto id_beginning = ipfs_beginning + 6;
    auto id_end = stringified_address_.find_first_of('/', id_beginning + 1);

    peer_id_ = stringified_address_.substr(id_beginning, id_end);
  }

  std::string_view Multiaddress::protocolToString(Protocol proto) const {
    switch (proto) {
      case Protocol::kIp4:
        return kIp4;
      case Protocol::kIp6:
        return kIp6;
      case Protocol::kIpfs:
        return kIpfs;
      case Protocol::kTcp:
        return kTcp;
      case Protocol::kUdp:
        return kUdp;
      case Protocol::kDccp:
        return kDccp;
      case Protocol::kSctp:
        return kSctp;
      case Protocol::kUdt:
        return kUdt;
      case Protocol::kUtp:
        return kUtp;
      case Protocol::kHttp:
        return kHttp;
      case Protocol::kHttps:
        return kHttps;
      case Protocol::kWs:
        return kWs;
      case Protocol::kOnion:
        return kOnion;
      case Protocol::kWebrtc:
        return kWebrtc;
    }
  }

}  // namespace libp2p::multi
