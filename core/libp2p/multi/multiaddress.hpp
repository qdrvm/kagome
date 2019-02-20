/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_MULTIADDRESS_HPP
#define KAGOME_MULTIADDRESS_HPP

#include <string>

#include <boost/optional.hpp>

namespace libp2p::multi {
  /**
   * Address format, used by Libp2p
   */
  class Multiaddress {
    /**
     * Construct a multiaddress instance from the string
     * @param address - string to be in that multiaddress
     */
    explicit Multiaddress(const std::string &address);

    /**
     * Put a new string to the right of this Multiaddress and recalculate the
     * bytes, such that:
     * '/ip4/192.168.0.1' after encapsulation with 'udp/138' becomes
     * '/ip4/192.168.0.1/udp/138'
     * @param part to be put
     * @return true, if address was successfully encapsulated, false otherwise
     */
    bool encapsulate(const std::string &part);

    /**
     * Remove the string from the right part of this Multiaddress, such that:
     * '/ip4/192.168.0.1/udp/138' after decapsulation with 'udp' becomes
     * '/ip4/192.168.0.1'
     * @param part to be removed
     * @return true, if address was successfully decapsulated, false otherwise
     */
    bool decapsulate(const std::string &part);

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
  };
}  // namespace libp2p::multi

#endif  // KAGOME_MULTIADDRESS_HPP
