/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_MULTIADDRESS_HPP
#define KAGOME_MULTIADDRESS_HPP

#include <memory>
#include <optional>
#include <outcome/outcome.hpp>
#include <string>
#include <string_view>
#include <unordered_map>

#include "common/buffer.hpp"

namespace libp2p::multi {

  /**
   * Address format, used by Libp2p
   */
  class Multiaddress {
   private:
    using ByteBuffer = kagome::common::Buffer;
    using FactoryResult = outcome::result<Multiaddress>;

   public:
    Multiaddress() = delete;

    Multiaddress(const Multiaddress &address) = default;
    Multiaddress &operator=(const Multiaddress &address) = default;

    Multiaddress(Multiaddress &&address) = default;
    Multiaddress &operator=(Multiaddress &&address) = default;

    enum class Error {
      InvalidInput = 1,     ///< input contains invalid multiaddress
      ProtocolNotFound,     ///< given protocol can not be found
      InvalidProtocolValue  ///< protocol value can not be casted to T
    };

    /**
     * Construct a multiaddress instance from the string
     * @param address - string to be in that multiaddress
     * @return pointer to Multiaddress, if creation is successful, error
     * otherwise
     */
    static FactoryResult create(std::string_view address);

    /**
     * Construct a multiaddress instance from the bytes
     * @param bytes to be in that multiaddress
     * @return pointer to Multiaddress, if creation is successful, error
     * otherwise
     */
    static FactoryResult create(const ByteBuffer &bytes);

    /**
     * Encapsulate a multiaddress to this one, such that:
     * '/ip4/192.168.0.1' after encapsulation with '/udp/138' becomes
     * '/ip4/192.168.0.1/udp/138'
     * @param address - another address to be encapsulated into this one
     */
    void encapsulate(const Multiaddress &address);

    /**
     * Encapsulate a multiaddress from this one, such that:
     * '/ip4/192.168.0.1/udp/138' after decapsulation with '/udp/' becomes
     * '/ip4/192.168.0.1'
     * @param address - another address to be decapsulated from this one
     * @return true, if such address was found and removed, false otherwise
     */
    bool decapsulate(const Multiaddress &address);

    /**
     * Get the textual representation of the address inside
     * @return stringified address
     */
    std::string_view getStringAddress() const;

    /**
     * Get the byte representation of the address inside
     * @return bytes address
     */
    const ByteBuffer &getBytesAddress() const;

    /**
     * Get peer id of this Multiaddress
     * @return peer id if exists
     */
    std::optional<std::string> getPeerId() const;

    /**
     * List of protocols, supported by this Multiaddress
     */
    enum class Protocol {
      kIp4,
      kIp6,
      kIpfs,
      kTcp,
      kUdp,
      kDccp,
      kSctp,
      kUdt,
      kUtp,
      kHttp,
      kHttps,
      kWs,
      kOnion,
      kWebrtc
    };
    /**
     * Get all values, which are under that protocol in this multiaddress
     * @param proto to be searched for
     * @return empty vector if no protocols found, or vector with values
     * otherwise
     */
    std::vector<std::string> getValuesForProtocol(Protocol proto) const;

    /**
     * Get first value for protocol
     * @param proto to be searched for
     * @return value (string) if protocol is found, none otherwise
     */
    outcome::result<std::string> getFirstValueForProtocol(Protocol proto) const;

    bool operator==(const Multiaddress &other) const;

    template <typename T>
    outcome::result<T> getFirstValueForProtocol(
        Protocol protocol, std::function<T(const std::string &)> caster) const {
      OUTCOME_TRY(val, getFirstValueForProtocol(protocol));

      try {
        return caster(val);
      } catch (...) {
        return Error::InvalidProtocolValue;
      }
    }

   private:
    /**
     * Construct a multiaddress instance from both address and bytes
     * @param address to be in the multiaddress
     * @param bytes to be in the multiaddress
     */
    Multiaddress(std::string &&address, ByteBuffer &&bytes);

    /**
     * Recalculate peer_id inside this address (the first one, if exists)
     */
    void calculatePeerId();

    /**
     * Convert Protocol enum into a string
     * @param proto to be converted
     * @return related string
     */
    std::string_view protocolToString(Protocol proto) const;

    ByteBuffer bytes_;
    std::string stringified_address_;

    std::optional<std::string> peer_id_;
  };
}  // namespace libp2p::multi

OUTCOME_HPP_DECLARE_ERROR(libp2p::multi, Multiaddress::Error);

#endif  // KAGOME_MULTIADDRESS_HPP
