/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_ADAPTERS_HPP
#define KAGOME_ADAPTERS_HPP

#include <functional>
#include <memory>
#include <gsl/span>

namespace kagome::network {

  template<typename T> struct ProtobufMessageAdapter {
    static size_t size(const T &t) {
      assert(!"No implementation");
      return 0ull;
    }
    static std::vector<uint8_t>::iterator write(const T &/*t*/, std::vector<uint8_t> &out, std::vector<uint8_t>::iterator /*loaded*/) {
      assert(!"No implementation");
      return out.end();
    }
  };

}  // namespace kagome::network

#endif  // KAGOME_ADAPTERS_HPP
