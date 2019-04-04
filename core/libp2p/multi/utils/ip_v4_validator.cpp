/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/multi/utils/ip_v4_validator.hpp"

#include <cctype>
#include <string>

namespace libp2p::multi {

  bool IPv4Validator::isValidIp(std::string_view ip_str) {
    int num, dots = 0;

    auto curr_pos = ip_str.find(kDelimeter);
    decltype(curr_pos) prev_pos = 0;

    if (curr_pos == std::string_view::npos) {
      return false;
    }

    dots++;
    while (prev_pos != ip_str.size() + 1) {
      auto substr = ip_str.substr(prev_pos, curr_pos - prev_pos);
      /* after parsing string, it must contain only digits */
      if (!isNumber(substr)) {
        return false;
      }
      try {
        num = std::stoi(std::string(substr));
      } catch (std::exception&) {
        return false;
      }

      /* check for valid IP */
      if (num >= 0 && num <= 255) {
        /* parse remaining string */
        prev_pos = curr_pos + 1;
        curr_pos = ip_str.find(kDelimeter, prev_pos);

        if (curr_pos != std::string_view::npos) {
          ++dots;
        } else {
          curr_pos = ip_str.size();
        }
      } else {
        return false;
      }
    }

    /* valid IP string must contain 3 dots */
    return dots == 3;
  }



  bool IPv4Validator::isNumber(std::string_view str) {
    for(char c: str) {
      if (!std::isdigit(c)) {
        return false;
      }
    }
    return true;
  }


}


