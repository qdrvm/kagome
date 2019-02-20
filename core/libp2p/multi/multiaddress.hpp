/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_MULTIADDRESS_HPP
#define KAGOME_MULTIADDRESS_HPP

#include <string>
#include <string_view>

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
    using FactoryResult = kagome::expected::Result<Multiaddress, std::string>;

   public:
    Multiaddress() = delete;

    /**
     * Construct a multiaddress instance from the string
     * @param address - string to be in that multiaddress
     */
    static FactoryResult createMultiaddress(std::string_view address);

    /**
     * Construct a multiaddress instance from the bytes
     * @param bytes to be in that multiaddress
     */
    static FactoryResult createMultiaddress(const ByteBuffer &bytes);

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
     * Families of protocols, supported by this multiaddress class
     */
    enum class Family { kIp4, kIp6 };
    /**
     * Get family of this multiaddress
     * @return enum of the family of this address
     */
    Family getFamily() const;

    /**
     * Get the textual representation of the address inside
     * @return
     */
    std::string getStringAddress() const;

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
     */
    Multiaddress(std::string &&address, ByteBuffer &&bytes);

    ByteBuffer bytes_;
    Family family_;
    std::string stringified_address_;
  };
}  // namespace libp2p::multi

#endif  // KAGOME_MULTIADDRESS_HPP
