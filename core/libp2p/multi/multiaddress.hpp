/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_MULTIADDRESS_HPP
#define KAGOME_MULTIADDRESS_HPP

#include <memory>
#include <string>
#include <string_view>

#include <spdlog/spdlog.h>
#include <boost/optional.hpp>
#include "common/buffer.hpp"
#include "common/result.hpp"

namespace libp2p::multi {
  /**
   * Address format, used by Libp2p
   */
  class Multiaddress {
   private:
    using ByteBuffer = kagome::common::Buffer;
    using FactoryResult =
        kagome::expected::Result<std::unique_ptr<Multiaddress>, std::string>;

   public:
    Multiaddress() = delete;

    Multiaddress(const Multiaddress &address) = default;
    Multiaddress &operator=(const Multiaddress &address) = default;

    Multiaddress(Multiaddress &&address) = default;
    Multiaddress &operator=(Multiaddress &&address) = default;

    /**
     * Construct a multiaddress instance from the string
     * @param address - string to be in that multiaddress
     */
    static FactoryResult createMultiaddress(std::string_view address);

    /**
     * Construct a multiaddress instance from the bytes
     * @param bytes to be in that multiaddress
     */
    static FactoryResult createMultiaddress(std::shared_ptr<ByteBuffer> bytes);

    /**
     * Put a new string to the right of this Multiaddress and recalculate the
     * bytes, such that:
     * '/ip4/192.168.0.1' after encapsulation with 'udp/138' becomes
     * '/ip4/192.168.0.1/udp/138'
     * @param part to be put
     * @return true, if address was successfully encapsulated, false otherwise
     */
    bool encapsulate(std::string_view part);

    /**
     * Remove the string from the right part of this Multiaddress, such that:
     * '/ip4/192.168.0.1/udp/138' after decapsulation with 'udp' becomes
     * '/ip4/192.168.0.1'
     * @param part to be removed
     * @return true, if address was successfully decapsulated, false otherwise
     */
    bool decapsulate(std::string_view part);

    /**
     * Families of protocols, supported by this multiaddress class; numbers
     * correspond to their byte encodings
     */
    enum class Family : uint8_t { kNone = 0, kIp4 = 4, kIp6 = 41 };
    /**
     * Get family of this multiaddress
     * @return enum of the family of this address
     */
    Family getFamily() const;

    /**
     * Check, if this address belongs to IP family
     * @return true, if address is from IP family, false otherwise
     */
    bool isIpAddress() const;

    /**
     * Get the textual representation of the whole address inside
     * @return stringified address
     */
    std::string_view getStringAddress() const;

    /**
     * Get the textual representation of particular family's address inside,
     * such that address '/ip4/192.168.0.1/tcp/1234' with passed kIp4 will lead
     * to '192.168.0.1/tcp/1234'
     * @param address_family of address to get
     * @return stringified part of address related to the family if supported,
     * none otherwise
     */
    boost::optional<std::string> getStringAddress(Family address_family) const;

    /**
     * Get port of this Multiaddress
     * @return port if exists
     */
    boost::optional<uint16_t> getPort() const;

    /**
     * Get peer id of this Multiaddress
     * @return peer id if exists
     */
    boost::optional<std::string> getPeerId() const;

   private:
    /**
     * Construct a multiaddress instance from both address and bytes
     * @param address to be in the multiaddress
     * @param bytes to be in the multiaddress
     * @throws invalid_argument, if could not retrieve address' family
     */
    Multiaddress(std::string &&address, std::shared_ptr<ByteBuffer> bytes);

    /**
     * Recalculate the port inside this address (if exists)
     */
    void calculatePort();

    /**
     * Recalculate peer_id inside this address (if exists)
     */
    void calculatePeerId();

    std::shared_ptr<ByteBuffer> bytes_;
    Family family_;
    std::string stringified_address_;

    boost::optional<uint16_t> port_;
    boost::optional<std::string> peer_id_;
  };
}  // namespace libp2p::multi

#endif  // KAGOME_MULTIADDRESS_HPP
