/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/system/error_code.hpp>
#include <functional>
#include <memory>
#include <span>
#include <vector>

#include "network/adapters/adapter_errors.hpp"
#include "outcome/outcome.hpp"

namespace kagome::network {

  template <typename T>
  struct ProtobufMessageAdapter {};

  template <typename T>
  inline std::vector<uint8_t>::iterator appendToVec(
      const T &msg,
      std::vector<uint8_t> &out,
      std::vector<uint8_t>::iterator loaded) {
    const size_t distance_was = std::distance(out.begin(), loaded);
    const size_t was_size = out.size();

    out.resize(was_size + msg.ByteSizeLong());
    msg.SerializeToArray(&out[was_size], msg.ByteSizeLong());

    auto res_it = out.begin();
    std::advance(res_it, std::min(distance_was, was_size));
    return res_it;
  }

}  // namespace kagome::network
