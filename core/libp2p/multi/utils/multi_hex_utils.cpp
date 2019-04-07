/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/multi/utils/multi_hex_utils.hpp"

#include <cmath>
#include <ios>
#include <iosfwd>
#include <sstream>
#include <string>

namespace libp2p::multi {

  std::string intToHex(uint64_t n) {
    std::stringstream result;
    result.width(2);
    result.fill('0');
    result << std::hex << std::uppercase << n;
    auto str = result.str();
    if (str.length() % 2 != 0) {
      str.push_back('\0');
      for (int i = str.length() - 2; i >= 0; --i) {
        str[i + 1] = str[i];
      }
      str[0] = '0';
    }
    return str;
  }

  uint64_t hexToInt(std::string_view hex) {
    uint64_t val = 0;
    while (!hex.empty()) {
      // get current character then increment
      uint8_t byte = hex.front();
      hex.remove_prefix(1);
      // transform hex character to the 4bit equivalent number, using the ascii
      // table indexes
      if (byte >= '0' && byte <= '9') {
        byte = byte - '0';
      } else if (byte >= 'a' && byte <= 'f') {
        byte = byte - 'a' + 10;
      } else if (byte >= 'A' && byte <= 'F') {
        byte = byte - 'A' + 10;
      }
      // shift 4 to make space for new digit, and add the 4 bits of the new
      // digit
      val = (val << 4) | (byte & 0xF);
    }
    return val;
  }

  std::vector<uint8_t> hexToBytes(std::string_view hex_str) {
    std::vector<uint8_t> res;
    size_t incoming_size = hex_str.size();
    res.resize(incoming_size / 2);

    std::string code(2, ' ');
    code[2] = '\0';
    int pos = 0;
    for (int i = 0; i < incoming_size; i += 2) {
      code[0] = hex_str[i];
      code[1] = hex_str[i + 1];
      int64_t lu = std::stol(code);
      res[pos] = static_cast<unsigned char>(lu);
      pos++;
    }
    return res;
  }


  uint64_t ipToInt(std::string_view r) {
    uint64_t final_result = 0;
    int ipat = 0;
    size_t c = 0;
    std::string::size_type prev = 0;
    auto pos = r.find(".");
    while (c < 4 && prev != std::string::npos) {
      ipat = std::stoi(std::string(r.substr(prev, pos)));
      prev = (pos != std::string::npos) ? pos + 1 : pos;
      pos = r.find(".", prev);
      final_result += ipat * pow(2, (3 - c) * 8);
      c++;
    }
    return final_result;
  }


  std::string intToIp(uint64_t anInt) {
    uint32_t ipint = anInt;
    std::stringstream xxx_int2ip_result;
    uint32_t ipint0 = (ipint >> 8 * 3) % 256;
    uint32_t ipint1 = (ipint >> 8 * 2) % 256;
    uint32_t ipint2 = (ipint >> 8 * 1) % 256;
    uint32_t ipint3 = (ipint >> 8 * 0) % 256;
    xxx_int2ip_result << ipint0 << "." << ipint1 << "." << ipint2 << "." << ipint3;
    return xxx_int2ip_result.str();
  }
}