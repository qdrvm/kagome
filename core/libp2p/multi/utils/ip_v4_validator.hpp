/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_IPV4VALIDATOR_HPP
#define KAGOME_IPV4VALIDATOR_HPP

#include <string_view>



namespace libp2p::multi {

  /**
   * Checks if given string is a valid IPv4 address (e.g. 127.0.0.1)
   */
  class IPv4Validator {
  public:

    /*
     * The delimeter between parts of an address
     */
    static constexpr std::string_view kDelimeter = ".";

    /**
     * Accepts a string only if it is in form of n.n.n.n, where n is an integer in range [0; 255]
     */
    static bool isValidIp(std::string_view ip_str);

  private:
    static bool isNumber(std::string_view str);
  };

}

#endif //KAGOME_IPV4VALIDATOR_HPP
